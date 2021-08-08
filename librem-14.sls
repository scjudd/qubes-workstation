/usr/src/librem_ec_acpi-1.0.0:
  file.recurse:
    - source: salt://files/librem-14/librem_ec_acpi-1.0.0
    - dir_mode: 755
    - file_mode: 644
    - user: root
    - group: root

dkms-build-librem-ec-acpi:
  cmd.run:
    - name: dkms build librem_ec_acpi/1.0.0
    - onchanges:
      - file: /usr/src/librem_ec_acpi-1.0.0

dkms-install-librem-ec-acpi:
  cmd.run:
    - name: dkms install librem_ec_acpi/1.0.0
    - onchanges:
      - file: /usr/src/librem_ec_acpi-1.0.0
    - require:
      - cmd: dkms-build-librem-ec-acpi
