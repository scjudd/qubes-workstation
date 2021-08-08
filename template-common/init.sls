include:
  - fonts

common-packages:
  pkg.installed:
    - pkgs:
      - ack
      - htop
      - pass

gnome-terminal:
  pkg.installed

/etc/dconf/db/local.d/gnome-terminal:
  require:
    - pkg: gnome-terminal
    - file: fonts
  file.managed:
    - source: salt://template-common/files/dconf/gnome-terminal
    - dir_mode: 755
    - mode: 644
    - user: root
    - group: root

dconf update:
  cmd.run:
    - onchanges:
      - file: /etc/dconf/db/local.d/gnome-terminal
