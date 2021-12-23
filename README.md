# Qubes OS Workstation

This repo captures all of my customizations to a vanilla Qubes OS 4.0.4 install as a set of Salt states. Since Qubes OS includes SaltStack, this makes for a convenient way to reproducibly configure it.


## Installation

From an AppVM (e.g., 'personal'):

* `$REPO_URL` is the URL of this repository (for example, 'https://github.com/scjudd/qubes-workstation.git')

```bash
git clone $REPO_URL
vim qubes-workstation/config.yaml       # make sure user settings are correct
vim qubes-workstation/scripts/dom0-sync # make sure variables are correct
```

From dom0:

* `$SOURCE_QUBE` is the Qube where you've cloned the repo (for example, 'personal')
* `$REPO_PATH` is the path to the cloned repo within `$SOURCE_QUBE` (for example, '/home/user/qubes-workstation')

```bash
qvm-run --pass-io $SOURCE_QUBE "cat $REPO_PATH/scripts/dom0-sync" > qubes-workstation-dom0-sync
chmod +x qubes-workstation-dom0-sync
sudo ./qubes-workstation-dom0-sync
sudo qubesctl --all state.highstate
```


## Debugging

The output from Salt for a given Qube can be found under `/var/log/qubes/mgmt-<qube name>.log` in dom0.


## Reinstalling a template

If you have deleted a standard template, or would like to reset it to its default state, you can do so with the `qubes-dom0-update` command. For example, to reset the `fedora-34` template to its default state:

```bash
sudo qubes-dom0-update --action=reinstall qubes-template-fedora-34
```


## Running a specific state against a specific target Qube

A state may be applied to a given Qube, regardless of whether or not it is listed in the `top.sls` file. For example, to apply the `user-config` state to the `personal-dev` Qube:

```bash
qubesctl --skip-dom0 --show-output --target personal-dev user-config saltenv=user
```


## Librem 14 specifics

To get the Librem 14 to consistently charge correctly, I had to flash a newer EC firmware and install the librem-ec-acpi kernel module with DKMS.


### Enable the Librem 14 top file

PureOS includes a Librem 14 EC ACPI DKMS module which solves a number of suspend/resume and charging issues. To do the same on Qubes, we must build the DKMS module ourselves. This, along with any other Librem 14-specific configuration, can be enabled like so:

```bash
sudo qubectl top.enable librem-14
```


### Flashing the EC firmware

**WARNING: This process is not currently recommended by Purism, as it could brick your machine if performed incorrectly. Proceed with caution.**

Download the `purism_ectool` binary and EC ROM file (e.g., `ec-2021-06-04_ef9fd3c.rom.gz`):

* [purism_ectool.gz](https://source.puri.sm/firmware/releases/-/blob/a82fe5219983d735e87760790fae3a120f92c03e/tools/purism_ectool.gz)
* [ec-2021-06-04_ef9fd3c.rom.gz](https://source.puri.sm/firmware/releases/-/blob/a82fe5219983d735e87760790fae3a120f92c03e/librem_14/ec-2021-06-04_ef9fd3c.rom.gz)

After extracting both and copying to dom0, run the following command:

```bash
sudo ./purism_ectool flash_backup ec-2021-06-04_ef9fd3c.rom
```

After this completes, your machine will shut down. On the next boot you should be able to verify that you are using the updated firmware like so:

```bash
sudo ./purism_ectool info
```


### Battery charge controller tuning

I usually have my laptop plugged into AC power, so I've configured the charge controller to start charging when the battery is below 40% charged and to stop once it hits 80%. This should increase the lifespan of the battery.

From dom0, as root:

```bash
echo 40 > /sys/class/power_supply/BAT0/charge_control_start_threshold
echo 80 > /sys/class/power_supply/BAT0/charge_control_end_threshold
```
