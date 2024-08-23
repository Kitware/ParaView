#!/bin/sh
set -e

# Skip if we're not in a `fedora` job.
if ! echo "$CMAKE_CONFIGURATION" | grep -q -e 'fedora'; then
  exit 0
fi

# Generate keys and append own public key as authorized
mkdir ~/.ssh
ssh-keygen -t rsa -q -f "$HOME/.ssh/id_rsa" -N ""
cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys

# Generate a SSH server configuration
echo "
Port 2222
HostKey ~/.ssh/id_rsa
PidFile ~/.ssh/sshd.pid
AuthorizedKeysFile ~/.ssh/id_rsa.pub
" > $HOME/.ssh/sshd_conf

# Disable host checking
echo "
Host *
    StrictHostKeyChecking no
" > $HOME/.ssh/config

# Run SSH server as daemon
/usr/sbin/sshd -f $HOME/.ssh/sshd_conf
