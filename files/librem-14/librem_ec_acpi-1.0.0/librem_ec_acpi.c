// SPDX-License-Identifier: GPL-2.0
/*
 * Librem EC ACPI Driver
 *
 * Nicole Faerber
 * Copyright (C) 2021 Purism
 * based on System76 ACPI Driver
 * Jeremy Soller <jeremy@system76.com>
 * Copyright (C) 2019 System76
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/acpi.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/pci_ids.h>
#include <linux/power_supply.h>
#include <linux/types.h>

#include <acpi/battery.h>

struct librem_ec_data {
	struct acpi_device *acpi_dev;
	struct led_classdev ap_led;
	struct led_classdev notif_led_r;
	struct led_classdev notif_led_g;
	struct led_classdev notif_led_b;
	struct led_classdev kb_led;
	enum led_brightness kb_brightness;
	enum led_brightness kb_toggle_brightness;
	struct device *therm;
	union acpi_object *nfan;
	union acpi_object *ntmp;
	struct input_dev *input;
};

static const struct acpi_device_id device_ids[] = {
	{"316D4C14", 0},
	{"PURI4543", 0},
	{"", 0},
};
MODULE_DEVICE_TABLE(acpi, device_ids);

// Array of keyboard LED brightness levels
static const enum led_brightness kb_levels[] = {
	0,
	128,
	172,
	215,
	254,
	255
};


// Get a Librem EC ACPI device value by name
static int librem_ec_get(struct librem_ec_data *data, char *method)
{
	acpi_handle handle;
	acpi_status status;
	unsigned long long ret = 0;

	handle = acpi_device_handle(data->acpi_dev);
	status = acpi_evaluate_integer(handle, method, NULL, &ret);
	if (ACPI_SUCCESS(status))
		return (int)ret;
	else
		return -1;
}

// Get a Librem EC ACPI device value by name with index
static int librem_ec_get_index(struct librem_ec_data *data, char *method, int index)
{
	union acpi_object obj;
	struct acpi_object_list obj_list;
	acpi_handle handle;
	acpi_status status;
	unsigned long long ret = 0;

	obj.type = ACPI_TYPE_INTEGER;
	obj.integer.value = index;
	obj_list.count = 1;
	obj_list.pointer = &obj;
	handle = acpi_device_handle(data->acpi_dev);
	status = acpi_evaluate_integer(handle, method, &obj_list, &ret);
	if (ACPI_SUCCESS(status))
		return (int)ret;
	else
		return -1;
}

// Get a Librem ACPI device object by name
static int librem_ec_get_object(struct librem_ec_data *data, char *method, union acpi_object **obj)
{
	acpi_handle handle;
	acpi_status status;
	struct acpi_buffer buf = { ACPI_ALLOCATE_BUFFER, NULL };

	handle = acpi_device_handle(data->acpi_dev);
	status = acpi_evaluate_object(handle, method, NULL, &buf);
	if (ACPI_SUCCESS(status)) {
		*obj = (union acpi_object *)buf.pointer;
		return 0;
	} else {
		return -1;
	}
}

// Get a name from a Librem ACPI device object
static char * librem_ec_name(union acpi_object *obj, int index) {
	if (obj && obj->type == ACPI_TYPE_PACKAGE && index <= obj->package.count) {
		if (obj->package.elements[index].type == ACPI_TYPE_STRING) {
			return obj->package.elements[index].string.pointer;
		}
	}
	return NULL;
}

// Set a Librem ACPI device value by name
static int librem_ec_set(struct librem_ec_data *data, char *method, int value)
{
	union acpi_object obj;
	struct acpi_object_list obj_list;
	acpi_handle handle;
	acpi_status status;

	obj.type = ACPI_TYPE_INTEGER;
	obj.integer.value = value;
	obj_list.count = 1;
	obj_list.pointer = &obj;
	handle = acpi_device_handle(data->acpi_dev);
	status = acpi_evaluate_object(handle, method, &obj_list, NULL);
	if (ACPI_SUCCESS(status))
		return 0;
	else
		return -1;
}

/* Battery */

#define BATTERY_THRESHOLD_INVALID       0xFF

enum {
	THRESHOLD_START,
	THRESHOLD_END,
};

static ssize_t battery_get_threshold(int which, char *buf)
{
	struct acpi_object_list input;
	union acpi_object param;
	acpi_handle handle;
	acpi_status status;
	unsigned long long ret = BATTERY_THRESHOLD_INVALID;

	handle = ec_get_handle();
	if (!handle)
		return -ENODEV;

	input.count = 1;
	input.pointer = &param;
	// Start/stop selection
	param.type = ACPI_TYPE_INTEGER;
	param.integer.value = which;

	status = acpi_evaluate_integer(handle, "GBCT", &input, &ret);
	if (ACPI_FAILURE(status))
		return -EIO;
	if (ret == BATTERY_THRESHOLD_INVALID)
		return -EINVAL;

	return sprintf(buf, "%d\n", (int)ret);
}

static ssize_t battery_set_threshold(int which, const char *buf, size_t count)
{
	struct acpi_object_list input;
	union acpi_object params[2];
	acpi_handle handle;
	acpi_status status;
	unsigned int value;
	int ret;

	handle = ec_get_handle();
	if (!handle)
		return -ENODEV;

	ret = kstrtouint(buf, 10, &value);
	if (ret)
		return ret;

	if (value > 100)
		return -EINVAL;

	input.count = 2;
	input.pointer = params;
	// Start/stop selection
	params[0].type = ACPI_TYPE_INTEGER;
	params[0].integer.value = which;
	// Threshold value
	params[1].type = ACPI_TYPE_INTEGER;
	params[1].integer.value = value;

	status = acpi_evaluate_object(handle, "SBCT", &input, NULL);
	if (ACPI_FAILURE(status))
		return -EIO;

	return count;
}

static ssize_t charge_control_start_threshold_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	return battery_get_threshold(THRESHOLD_START, buf);
}

static ssize_t charge_control_start_threshold_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	return battery_set_threshold(THRESHOLD_START, buf, count);
}

static DEVICE_ATTR_RW(charge_control_start_threshold);

static ssize_t charge_control_end_threshold_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	return battery_get_threshold(THRESHOLD_END, buf);
}

static ssize_t charge_control_end_threshold_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	return battery_set_threshold(THRESHOLD_END, buf, count);
}

static DEVICE_ATTR_RW(charge_control_end_threshold);

static int librem_ec_battery_add(struct power_supply *battery)
{
	// Librem EC only supports 1 battery
	if (strcmp(battery->desc->name, "BAT0") != 0)
		return -ENODEV;

	device_create_file(&battery->dev, &dev_attr_charge_control_start_threshold);
	device_create_file(&battery->dev, &dev_attr_charge_control_end_threshold);

	return 0;
}

static int librem_ec_battery_remove(struct power_supply *battery)
{
	device_remove_file(&battery->dev, &dev_attr_charge_control_start_threshold);
	device_remove_file(&battery->dev, &dev_attr_charge_control_end_threshold);
	return 0;
}

static struct acpi_battery_hook librem_ec_battery_hook = {
	.add_battery = librem_ec_battery_add,
	.remove_battery = librem_ec_battery_remove,
	.name = "Librem EC Battery Extension",
};

static void librem_ec_battery_init(void)
{
	acpi_handle handle;

	handle = ec_get_handle();
	if (handle && acpi_has_method(handle, "GBCT"))
		battery_hook_register(&librem_ec_battery_hook);
}

static void librem_ec_battery_exit(void)
{
	acpi_handle handle;

	handle = ec_get_handle();
	if (handle && acpi_has_method(handle, "GBCT"))
		battery_hook_unregister(&librem_ec_battery_hook);
}

/* Keyboard */

// Get the airplane mode LED brightness
static enum led_brightness ap_led_get(struct led_classdev *led)
{
	struct librem_ec_data *data;
	int value;

	data = container_of(led, struct librem_ec_data, ap_led);
	value = librem_ec_get(data, "GAPL");
	if (value > 0)
		return (enum led_brightness)value;
	else
		return LED_OFF;
}

// Set the airplane mode LED brightness
static int ap_led_set(struct led_classdev *led, enum led_brightness value)
{
	struct librem_ec_data *data;

	data = container_of(led, struct librem_ec_data, ap_led);
	return librem_ec_set(data, "SAPL", value == LED_OFF ? 0 : 1);
}

//
// notification RGB LEDs
//
static enum led_brightness notification_led_r_get(struct led_classdev *led)
{
	struct librem_ec_data *data;
	int value;

	data = container_of(led, struct librem_ec_data, notif_led_r);
	value = librem_ec_get(data, "GNTR");
	if (value > 0)
		return (enum led_brightness)value;
	else
		return 0;
}

static int notification_led_r_set(struct led_classdev *led, enum led_brightness value)
{
	struct librem_ec_data *data;

	data = container_of(led, struct librem_ec_data, notif_led_r);
	return librem_ec_set(data, "SNTR", value);
}

static enum led_brightness notification_led_g_get(struct led_classdev *led)
{
	struct librem_ec_data *data;
	int value;

	data = container_of(led, struct librem_ec_data, notif_led_g);
	value = librem_ec_get(data, "GNTG");
	if (value > 0)
		return (enum led_brightness)value;
	else
		return 0;
}

static int notification_led_g_set(struct led_classdev *led, enum led_brightness value)
{
	struct librem_ec_data *data;

	data = container_of(led, struct librem_ec_data, notif_led_g);
	return librem_ec_set(data, "SNTG", value);
}

static enum led_brightness notification_led_b_get(struct led_classdev *led)
{
	struct librem_ec_data *data;
	int value;

	data = container_of(led, struct librem_ec_data, notif_led_b);
	value = librem_ec_get(data, "GNTB");
	if (value > 0)
		return (enum led_brightness)value;
	else
		return 0;
}

static int notification_led_b_set(struct led_classdev *led, enum led_brightness value)
{
	struct librem_ec_data *data;

	data = container_of(led, struct librem_ec_data, notif_led_b);
	return librem_ec_set(data, "SNTB", value);
}





// Get the last set keyboard LED brightness
static enum led_brightness kb_led_get(struct led_classdev *led)
{
	struct librem_ec_data *data;

	data = container_of(led, struct librem_ec_data, kb_led);
	return data->kb_brightness;
}

// Set the keyboard LED brightness
static int kb_led_set(struct led_classdev *led, enum led_brightness value)
{
	struct librem_ec_data *data;

	data = container_of(led, struct librem_ec_data, kb_led);
	data->kb_brightness = value;
	return librem_ec_set(data, "SKBL", (int)data->kb_brightness);
}

// Notify that the keyboard LED was changed by hardware
static void kb_led_notify(struct librem_ec_data *data)
{
	led_classdev_notify_brightness_hw_changed(
		&data->kb_led,
		data->kb_brightness
	);
}

// Read keyboard LED brightness as set by hardware
static void kb_led_hotkey_hardware(struct librem_ec_data *data)
{
	int value;

	value = librem_ec_get(data, "GKBL");
	if (value < 0)
		return;
	data->kb_brightness = value;
	kb_led_notify(data);
}

// Toggle the keyboard LED
static void kb_led_hotkey_toggle(struct librem_ec_data *data)
{
	if (data->kb_brightness > 0) {
		data->kb_toggle_brightness = data->kb_brightness;
		kb_led_set(&data->kb_led, 0);
	} else {
		kb_led_set(&data->kb_led, data->kb_toggle_brightness);
	}
	kb_led_notify(data);
}

// Decrease the keyboard LED brightness
static void kb_led_hotkey_down(struct librem_ec_data *data)
{
	int i;

	if (data->kb_brightness > 0) {
		for (i = ARRAY_SIZE(kb_levels); i > 0; i--) {
			if (kb_levels[i - 1] < data->kb_brightness) {
				kb_led_set(&data->kb_led, kb_levels[i - 1]);
				break;
			}
		}
	} else {
		kb_led_set(&data->kb_led, data->kb_toggle_brightness);
	}
	kb_led_notify(data);
}

// Increase the keyboard LED brightness
static void kb_led_hotkey_up(struct librem_ec_data *data)
{
	int i;

	if (data->kb_brightness > 0) {
		for (i = 0; i < ARRAY_SIZE(kb_levels); i++) {
			if (kb_levels[i] > data->kb_brightness) {
				kb_led_set(&data->kb_led, kb_levels[i]);
				break;
			}
		}
	} else {
		kb_led_set(&data->kb_led, data->kb_toggle_brightness);
	}
	kb_led_notify(data);
}

/* hwmon */

static umode_t thermal_is_visible(const void *drvdata, enum hwmon_sensor_types type, u32 attr, int channel) {
	const struct librem_ec_data *data = drvdata;

	if (type == hwmon_fan || type == hwmon_pwm) {
		if (librem_ec_name(data->nfan, channel)) {
			return S_IRUGO;
		}
	} else if (type == hwmon_temp) {
		if (librem_ec_name(data->ntmp, channel)) {
			return S_IRUGO;
		}
	}
	return 0;
}

static int thermal_read(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, long *val) {
	struct librem_ec_data *data = dev_get_drvdata(dev);
	int raw;

	if (type == hwmon_fan && attr == hwmon_fan_input) {
		raw = librem_ec_get_index(data, "GFAN", channel);
		if (raw >= 0) {
			*val = (long)((raw >> 8) & 0xFFFF);
			return 0;
		}
	} else if (type == hwmon_pwm && attr == hwmon_pwm_input) {
		raw = librem_ec_get_index(data, "GFAN", channel);
		if (raw >= 0) {
			*val = (long)(raw & 0xFF);
			return 0;
		}
	} else if (type == hwmon_temp && attr == hwmon_temp_input) {
		raw = librem_ec_get_index(data, "GTMP", channel);
		if (raw >= 0) {
			*val = (long)(raw * 1000);
			return 0;
		}
	}
	return -EINVAL;
}

static int thermal_read_string(struct device *dev, enum hwmon_sensor_types type, u32 attr, int channel, const char **str) {
	struct librem_ec_data *data = dev_get_drvdata(dev);

	if (type == hwmon_fan && attr == hwmon_fan_label) {
		*str = librem_ec_name(data->nfan, channel);
		if (*str)
			return 0;
	} else if (type == hwmon_temp && attr == hwmon_temp_label) {
		*str = librem_ec_name(data->ntmp, channel);
		if (*str)
			return 0;
	}
	return -EINVAL;
}

static const struct hwmon_ops thermal_ops = {
	.is_visible = thermal_is_visible,
	.read = thermal_read,
	.read_string = thermal_read_string,
};

// Allocate up to 8 fans and temperatures
static const struct hwmon_channel_info *thermal_channel_info[] = {
	HWMON_CHANNEL_INFO(fan,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL,
		HWMON_F_INPUT | HWMON_F_LABEL),
	HWMON_CHANNEL_INFO(pwm,
			HWMON_PWM_INPUT,
			HWMON_PWM_INPUT,
			HWMON_PWM_INPUT,
			HWMON_PWM_INPUT,
			HWMON_PWM_INPUT,
			HWMON_PWM_INPUT,
			HWMON_PWM_INPUT,
			HWMON_PWM_INPUT),
	HWMON_CHANNEL_INFO(temp,
			HWMON_T_INPUT | HWMON_T_LABEL,
			HWMON_T_INPUT | HWMON_T_LABEL,
			HWMON_T_INPUT | HWMON_T_LABEL,
			HWMON_T_INPUT | HWMON_T_LABEL,
			HWMON_T_INPUT | HWMON_T_LABEL,
			HWMON_T_INPUT | HWMON_T_LABEL,
			HWMON_T_INPUT | HWMON_T_LABEL,
			HWMON_T_INPUT | HWMON_T_LABEL),
	NULL
};

static const struct hwmon_chip_info thermal_chip_info = {
	.ops = &thermal_ops,
	.info = thermal_channel_info,
};

/* ACPI driver */

static void input_key(struct librem_ec_data *data, unsigned int code) {
	input_report_key(data->input, code, 1);
	input_sync(data->input);

	input_report_key(data->input, code, 0);
	input_sync(data->input);
}

// Handle ACPI notification
static void librem_ec_notify(struct acpi_device *acpi_dev, u32 event)
{
	struct librem_ec_data *data;

	data = acpi_driver_data(acpi_dev);

	switch (event) {
	case 0x80:
		kb_led_hotkey_hardware(data);
		break;
	case 0x81:
		kb_led_hotkey_toggle(data);
		break;
	case 0x82:
		kb_led_hotkey_down(data);
		break;
	case 0x83:
		kb_led_hotkey_up(data);
		break;
	case 0x85:
		input_key(data, KEY_SCREENLOCK);
		break;
	default:
		printk(KERN_INFO "librem_ec_acpi: unhandled notify event 0x%02x\n", event);
	}
}

// Add a Librem EC ACPI device
static int librem_ec_add(struct acpi_device *acpi_dev)
{
	struct librem_ec_data *data;
	int err;

	data = devm_kzalloc(&acpi_dev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	acpi_dev->driver_data = data;
	data->acpi_dev = acpi_dev;

	err = librem_ec_get(data, "INIT");
	if (err)
		return err;
	data->ap_led.name = "librem_ec:airplane";
	data->ap_led.flags = LED_CORE_SUSPENDRESUME;
	data->ap_led.brightness_get = ap_led_get;
	data->ap_led.brightness_set_blocking = ap_led_set;
	data->ap_led.max_brightness = 1;
	data->ap_led.default_trigger = "rfkill-none";
	err = devm_led_classdev_register(&acpi_dev->dev, &data->ap_led);
	if (err)
		return err;

	data->notif_led_r.name = "red:status";
	data->notif_led_r.flags = LED_CORE_SUSPENDRESUME;
	data->notif_led_r.brightness_get = notification_led_r_get;
	data->notif_led_r.brightness_set_blocking = notification_led_r_set;
	data->notif_led_r.max_brightness = 255;
	// data->notif_led_r.default_trigger = "rfkill-none";
	err = devm_led_classdev_register(&acpi_dev->dev, &data->notif_led_r);
	if (err)
		return err;

	data->notif_led_g.name = "green:status";
	data->notif_led_g.flags = LED_CORE_SUSPENDRESUME;
	data->notif_led_g.brightness_get = notification_led_g_get;
	data->notif_led_g.brightness_set_blocking = notification_led_g_set;
	data->notif_led_g.max_brightness = 255;
	// data->notif_led_g.default_trigger = "rfkill-none";
	err = devm_led_classdev_register(&acpi_dev->dev, &data->notif_led_g);
	if (err)
		return err;

	data->notif_led_b.name = "blue:status";
	data->notif_led_b.flags = LED_CORE_SUSPENDRESUME;
	data->notif_led_b.brightness_get = notification_led_b_get;
	data->notif_led_b.brightness_set_blocking = notification_led_b_set;
	data->notif_led_b.max_brightness = 255;
	// data->notif_led_b.default_trigger = "rfkill-none";
	err = devm_led_classdev_register(&acpi_dev->dev, &data->notif_led_b);
	if (err)
		return err;

	data->kb_led.name = "librem_ec:kbd_backlight";
	data->kb_led.flags = LED_BRIGHT_HW_CHANGED | LED_CORE_SUSPENDRESUME;
	data->kb_led.brightness_get = kb_led_get;
	data->kb_led.brightness_set_blocking = kb_led_set;
	data->kb_led.max_brightness = 255;
	err = devm_led_classdev_register(&acpi_dev->dev, &data->kb_led);
	if (err)
		return err;

	librem_ec_get_object(data, "NFAN", &data->nfan);
	librem_ec_get_object(data, "NTMP", &data->ntmp);
	data->therm = devm_hwmon_device_register_with_info(&acpi_dev->dev, "librem_ec_acpi", data, &thermal_chip_info, NULL);
	if (IS_ERR(data->therm))
		return PTR_ERR(data->therm);

	data->input = devm_input_allocate_device(&acpi_dev->dev);
	if (!data->input)
		return -ENOMEM;
	data->input->name = "Librem EC ACPI Hotkeys";
	data->input->phys = "librem_ec_acpi/input0";
	data->input->id.bustype = BUS_HOST;
	data->input->dev.parent = &acpi_dev->dev;
	set_bit(EV_KEY, data->input->evbit);
	set_bit(KEY_SCREENLOCK, data->input->keybit);
	err = input_register_device(data->input);
	if (err) {
		input_free_device(data->input);
		return err;
	}

	librem_ec_battery_init();

	return 0;
}

// Remove a Librem EC ACPI device
static int librem_ec_remove(struct acpi_device *acpi_dev)
{
	struct librem_ec_data *data;

	data = acpi_driver_data(acpi_dev);

	librem_ec_battery_exit();

	devm_led_classdev_unregister(&acpi_dev->dev, &data->ap_led);

	devm_led_classdev_unregister(&acpi_dev->dev, &data->kb_led);
	if (data->nfan)
		kfree(data->nfan);
	if (data->ntmp)
		kfree(data->ntmp);

	librem_ec_get(data, "FINI");

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int librem_ec_suspend(struct device *device)
{
        struct librem_ec_data *data = acpi_driver_data(to_acpi_device(device));

        if (!data)
                return -ENOMEM;

	librem_ec_get(data, "FINI");

        return 0;
}

static int librem_ec_resume(struct device *device)
{
        struct librem_ec_data *data = acpi_driver_data(to_acpi_device(device));

        if (!data)
                return -ENOMEM;

	librem_ec_get(data, "INIT");

        return 0;
}
#else
#define librem_ec_suspend    NULL
#define librem_ec_resume     NULL
#endif

static SIMPLE_DEV_PM_OPS(librem_ec_pm, librem_ec_suspend, librem_ec_resume);


static struct acpi_driver librem_ec_driver = {
	.name = "Librem EC ACPI Driver",
	.owner = THIS_MODULE,
	.class = "hotkey",
	.ids = device_ids,
	// .flags = ACPI_DRIVER_ALL_NOTIFY_EVENTS,
	.ops = {
		.add = librem_ec_add,
		.remove = librem_ec_remove,
		.notify = librem_ec_notify,
	},
	.drv.pm = &librem_ec_pm,
};
module_acpi_driver(librem_ec_driver);

MODULE_DESCRIPTION("Librem EC ACPI Driver");
MODULE_AUTHOR("Nicole Faerber <nicole.faerber@puri.sm>");
MODULE_LICENSE("GPL");
