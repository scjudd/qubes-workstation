{% import_yaml "config.yaml" as config %}

vimrc:
  file.managed:
    - name: /home/{{ config.domU_user.username }}/.vimrc
    - source: salt://files/vim/vimrc
    - mode: 644
    - user: {{ config.domU_user.username }}
    - group: {{ config.domU_user.group }}
