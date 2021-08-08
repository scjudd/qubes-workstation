{% import_yaml "config.yaml" as config %}

~/.vimrc:
  file.managed:
    - name: /home/user/.vimrc
    - source: salt://user-config/files/vim/vimrc
    - mode: 644
    - user: user
    - group: user
