# Qubes OS Workstation

This repo captures all of my customizations to a vanilla Qubes OS 4.0.4 install as a set of Salt states. Since Qubes OS includes SaltStack, this makes for a convenient way to reproducibly configure it.


## Installation

Clone the repo in an AppVM, edit `config.yaml` as appropriate, then run the below script to sync the configuration with dom0. To apply, run `qubesctl --all state.highstate`.

```bash
#!/bin/bash
set -ueo pipefail

SOURCE_QUBE=personal
SOURCE_DIR=/home/user/qubes-workstation
TARGET_DIR=/srv/user_salt

echo "Replacing '$TARGET_DIR' with the contents of '$SOURCE_DIR' from the '$SOURCE_QUBE' qube."

rm -rf $TARGET_DIR/*
qvm-run --pass-io $SOURCE_QUBE "tar -c -f- -C $SOURCE_DIR ." | tar -x -f- -C $TARGET_DIR

echo "Done! Run 'qubesctl --all state.highstate' to (re-)configure Qubes OS."
```


## Debugging

The output from Salt for a given Qube can be found under `/var/log/qubes/mgmt-<qube name>.log` in dom0.


## Librem 14 troubleshooting

To get the Librem 14 to consistently charge correctly, I had to flash a newer EC firmware and install the librem-ec-acpi kernel module with DKMS.


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


### Installing the librem-ec-acpi kernel module

This module is included with PureOS, but it is not currently packaged for Qubes. Besides fixing charging issues (together with the EC firmware update above), it adds a bunch of controls for LEDs and such.

This currently is done automatically for dom0, but can be disabled by commenting out the `- librem-ec-acpi` line in `top.sls`.
