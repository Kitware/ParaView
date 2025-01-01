#!/bin/sh

set -e

# Skip if we're not in a `fedora` job.
if ! echo "$CMAKE_CONFIGURATION" | grep -q -e 'fedora'; then
    exit 0
fi

# Only works when running as root.
if [ "$( id -u )" != "0" ]; then
    exit 0
fi

readonly ssh_root=".gitlab/ssh"
mkdir -p "$ssh_root"

# Generate keys and append own public key as authorized
mkdir ~/.ssh
ssh-keygen -t rsa -q -f "$ssh_root/id_rsa" -N ""
cp "$ssh_root/id_rsa.pub" "$ssh_root/authorized_keys"

# Generate a SSH server configuration
cat > "$ssh_root/sshd_config" <<EOF
Port 2222
HostKey $ssh_root/id_rsa
PidFile $ssh_root/sshd.pid
AuthorizedKeysFile $ssh_root/id_rsa.pub
EOF

# Run SSH server as daemon
/usr/sbin/sshd -f "$ssh_root/sshd_config"
