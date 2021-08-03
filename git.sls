{% import_yaml "config.yaml" as config %}

gitconfig:
  file.managed:
    - name: /home/{{ config.domU_user.username }}/.gitconfig
    - source: salt://files/git/gitconfig
    - mode: 644
    - user: {{ config.domU_user.username }}
    - group: {{ config.domU_user.group }}
