install-ghidra-deps:
  pkg.installed:
    - name: java-11-openjdk-devel

install-ghidra:
  cmd.script:
    - name: salt://infosec-template/files/install-ghidra.sh
    - requires:
      - pkg: install-ghidra-deps
