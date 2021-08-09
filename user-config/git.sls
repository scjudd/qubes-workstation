~/.gitconfig:
  file.managed:
    - name: /home/user/.gitconfig
    - source: salt://user-config/files/git/gitconfig
    - mode: 644
    - user: user
    - group: user
