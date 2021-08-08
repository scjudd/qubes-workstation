infosec-template:
  qvm.vm:
    - name: infosec
    - clone:
      - source: fedora-32
      - label: black

infosec-dvm:
  qvm.vm:
    - present:
      - template: infosec
      - label: red
    - prefs:
      - template: infosec
      - template-for-dispvms: true
    - features:
      - enable:
        - appmenus-dispvm
