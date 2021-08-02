fonts:
  file.recurse:
    - name: /usr/share/fonts/salt/
    - source: salt://files/fonts
    - dir_mode: 755
    - file_mode: 644
    - user: root
    - group: root

clear-font-cache:
  cmd.run:
    - name: fc-cache -r
    - onchanges:
      - file: fonts
