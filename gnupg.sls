{% import_yaml "config.yaml" as config %}

gpg-agent.conf:
  file.managed:
    - name: /home/{{ config.domU_user.username }}/.gnupg/gpg-agent.conf
    - source: salt://files/gnupg/gpg-agent.conf
    - mode: 644
    - user: {{ config.domU_user.username }}
    - group: {{ config.domU_user.group }}

gpg-agent-ssh.socket:
  file.symlink:
    - name: /home/{{ config.domU_user.username }}/.config/systemd/user/sockets.target.wants/gpg-agent-ssh.socket
    - target: /usr/lib/systemd/user/gpg-agent-ssh.socket

export-ssh-auth-sock:
  file.replace:
    - name: /home/{{ config.domU_user.username }}/.bash_profile
    - pattern: "^export SSH_AUTH_SOCK=.*$"
    - repl: "export SSH_AUTH_SOCK=$(gpgconf --list-dirs agent-ssh-socket)"
    - append_if_not_found: true
