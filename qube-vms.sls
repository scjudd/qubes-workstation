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
