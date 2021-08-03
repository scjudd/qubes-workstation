java-11-openjdk-devel:
  pkg.installed: []

install-ghidra:
  cmd.script:
    - name: salt://files/ghidra/install.sh

ghidra-desktop-file:
  file.managed:
    - name: /usr/share/applications/ghidra.desktop
    - source: salt://files/ghidra/ghidra.desktop
    - mode: 644
