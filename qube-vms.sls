fedora-32-customized-template:
  qvm.vm:
    - name: fedora-32-customized
    - clone:
      - source: fedora-32
      - label: black

create-personal-vm:
  qvm.vm:
    - name: personal
    - present:
      - template: fedora-32-customized
    - prefs:
      - template: fedora-32-customized

fedora-32-infosec-template:
  qvm.vm:
    - name: fedora-32-infosec
    - clone:
      - source: fedora-32
      - label: black

infosec-dvm:
  qvm.vm:
    - present:
      - template: fedora-32-infosec
      - label: red
    - prefs:
      - template: fedora-32-infosec
      - template-for-dispvms: true
    - features:
      - enable:
        - appmenus-dispvm
