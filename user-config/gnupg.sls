{% import_yaml "config.yaml" as config %}

~/.gnupg/gpg-agent.conf:
  file.managed:
    - name: /home/user/.gnupg/gpg-agent.conf
    - source: salt://user-config/files/gnupg/gpg-agent.conf
    - mode: 644
    - user: user
    - group: user

~/.config/systemd/user/sockets.targets.wants/gpg-agent-ssh.socket:
  file.symlink:
    - name: /home/user/.config/systemd/user/sockets.target.wants/gpg-agent-ssh.socket
    - target: /usr/lib/systemd/user/gpg-agent-ssh.socket

export SSH_AUTH_SOCK:
  file.replace:
    - name: /home/user/.bash_profile
    - pattern: "^export SSH_AUTH_SOCK=.*$"
    - repl: "export SSH_AUTH_SOCK=$(gpgconf --list-dirs agent-ssh-socket)"
    - append_if_not_found: true
