#!/bin/bash
set -e

# Generate host keys if not already present (e.g. when /etc/ssh is bind-mounted)
ssh-keygen -A

# If SSH_PUBLIC_KEY is set at runtime, append it to authorized_keys.
# Useful when the key wasn't available at build time:
#   docker run -e SSH_PUBLIC_KEY="$(cat ~/.ssh/id_rsa.pub)" ...
if [ -n "${SSH_PUBLIC_KEY}" ]; then
    mkdir -p /home/testuser/.ssh
    echo "${SSH_PUBLIC_KEY}" >> /home/testuser/.ssh/authorized_keys
    sort -u /home/testuser/.ssh/authorized_keys -o /home/testuser/.ssh/authorized_keys
    chmod 600 /home/testuser/.ssh/authorized_keys
    chown testuser:testuser /home/testuser/.ssh/authorized_keys
fi

exec /usr/sbin/sshd -D -e
