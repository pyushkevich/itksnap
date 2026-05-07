FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# ── System packages ───────────────────────────────────────────────────────────
RUN apt-get update && apt-get install -y --no-install-recommends \
        openssh-server \
        wget \
        unzip \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# ── sshd configuration ────────────────────────────────────────────────────────
RUN mkdir -p /var/run/sshd /etc/ssh/sshd_config.d

# Overwrite with a clean, minimal sshd_config for testing
RUN cat > /etc/ssh/sshd_config <<'EOF'
Port 22
PermitRootLogin no
PasswordAuthentication yes
PubkeyAuthentication yes
AuthorizedKeysFile .ssh/authorized_keys
AllowTcpForwarding yes
Subsystem sftp /usr/lib/openssh/sftp-server
EOF

# ── testuser ──────────────────────────────────────────────────────────────────
RUN useradd -m -s /bin/bash testuser && \
    echo 'testuser:testpassword' | chpasswd

RUN mkdir -p /home/testuser/.ssh && \
    chmod 700 /home/testuser/.ssh && \
    chown testuser:testuser /home/testuser/.ssh

# Optionally inject a public key at build time:
#   docker build --build-arg SSH_PUBLIC_KEY="$(cat ~/.ssh/id_rsa.pub)" .
ARG SSH_PUBLIC_KEY=""
RUN if [ -n "$SSH_PUBLIC_KEY" ]; then \
        echo "$SSH_PUBLIC_KEY" > /home/testuser/.ssh/authorized_keys && \
        chmod 600 /home/testuser/.ssh/authorized_keys && \
        chown testuser:testuser /home/testuser/.ssh/authorized_keys; \
    fi

# ── ITK-SNAP sample data ──────────────────────────────────────────────────────
# Downloads from NITRC. If the build fails here (NITRC may require a login
# cookie for some releases), pre-download the zips, place them alongside this
# Dockerfile, and replace the wget lines with COPY / RUN unzip.
RUN mkdir -p /data && \
    cd /tmp && \
    wget -q --no-check-certificate \
        "https://www.nitrc.org/frs/download.php/6161/braintumor.zip" \
        -O braintumor.zip && \
    unzip -q braintumor.zip -d /data && \
    rm braintumor.zip && \
    wget -q --no-check-certificate \
        "https://www.nitrc.org/frs/download.php/6176/diffusion.zip" \
        -O diffusion.zip && \
    unzip -q diffusion.zip -d /data && \
    rm diffusion.zip && \
    wget -q --no-check-certificate \
        "https://www.nitrc.org/frs/download.php/10983/ashs_test.zip" \
        -O ashs_test.zip && \
    unzip -q ashs_test.zip -d /data && \
    rm ashs_test.zip && \
    rm -rf /data/__MACOSX && \
    chmod -R 755 /data && \
    chown -R testuser:testuser /data

# ── Entrypoint ────────────────────────────────────────────────────────────────
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

EXPOSE 22

ENTRYPOINT ["/entrypoint.sh"]
