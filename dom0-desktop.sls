{% import_yaml "config.yaml" as config %}

include:
  - fonts

Xresources:
  file.managed:
    - name: /home/{{ config.dom0_user.username }}/.Xresources
    - source: salt://files/X11/Xresources
    - mode: 644
    - user: {{ config.dom0_user.username }}
    - group: {{ config.dom0_user.group }}

trackpad-config:
  file.managed:
    - name: /etc/X11/xorg.conf.d/90-trackpad.conf
    - source: salt://files/X11/xorg.conf.d/90-trackpad.conf
    - mode: 644
    - user: root
    - group: root

qubes-repo-contrib:
  pkg.installed: []

i3:
  pkg.installed:
    - name: i3-gaps
    - require:
      - pkg: qubes-repo-contrib

i3-config:
  file.managed:
    - name: /home/{{ config.dom0_user.username}}/.config/i3/config
    - requires:
      - file: i3-autolayout-script
    - source: salt://files/i3/config
    - mode: 644
    - user: {{ config.dom0_user.username }}
    - group: {{ config.dom0_user.group }}

i3-autolayout-script:
  file.managed:
    - name: /home/{{ config.dom0_user.username }}/.local/bin/i3-autolayout.py
    - requires:
      - cmd: i3-autolayout-install-deps
    - source: salt://files/i3/i3-autolayout.py
    - mode: 755
    - user: {{ config.dom0_user.username }}
    - group: {{ config.dom0_user.group }}

# TODO: Come up with a better way to manage these dependencies. Remember that
# dom0 intentionally cannot connect to the internet.
i3-autolayout-install-deps:
  file.recurse:
    - name: /tmp/i3-autolayout-deps
    - source: salt://files/i3/i3-autolayout-deps
  cmd.wait:
    - name: pip3 install --user *.whl
    - cwd: /tmp/i3-autolayout-deps
    - runas: {{ config.dom0_user.username }}
    - watch:
      - file: i3-autolayout-install-deps

rofi:
  pkg.installed:
    - require:
      - pkg: qubes-repo-contrib

rofi-config:
  file.recurse:
    - name: /home/{{ config.dom0_user.username}}/.config/rofi/
    - source: salt://files/rofi
    - dir_mode: 755
    - file_mode: 644
    - user: {{ config.dom0_user.username }}
    - group: {{ config.dom0_user.group }}
