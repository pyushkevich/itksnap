# itksnap-sshtest

A minimal Ubuntu container running an SSH server with sample ITK-SNAP datasets pre-loaded under `/data/`. Use it to test ITK-SNAP's SSH/remote workspace functionality without needing a real remote machine.

## Build

```sh
docker build -t itksnap-sshtest .
```

A pre-built image is available on Docker Hub:

```sh
docker pull pyushkevich/itksnap-sshtest:latest
```

## Run

```sh
docker run -d -p 2222:22 \
  -e SSH_PUBLIC_KEY="$(cat ~/.ssh/id_rsa.pub)" \
  --name itksnap-sshtest \
  pyushkevich/itksnap-sshtest:latest
```

Connect as `testuser` (password: `testpassword`) on `localhost:2222`. Open a sample workspace directly in itksnap via the `-w` flag:

```sh
itksnap -w "sftp://testuser@localhost:2222/data/braintumor/BRATS_HG0015.itksnap"
itksnap -w "sftp://testuser@localhost:2222/data/diffusion/diffusion.itksnap"
itksnap -w "sftp://testuser@localhost:2222/data/ashs_test/ashs_test.itksnap"
```

## Stop / remove

```sh
docker stop itksnap-sshtest && docker rm itksnap-sshtest
```
