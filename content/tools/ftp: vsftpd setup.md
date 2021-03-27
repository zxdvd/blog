```metadata
tags: server, ftp
```

## ftp server: vsftpd setup

`vsftpd` is a simple ftp server that it's very easy to install in almost all linux
 distribution. And you can connect to it using common ftp clients in all platforms.

A simple workable config:

```text
listen=YES                   # run as a standalone server instead of inetd service
listen_port=36800            # change the listening port
anonymous_enable=NO          # disable anonymous
local_enable=YES
pam_service_name=vsftpd      # auth via pam

pasv_enable=YES              # enable passive mode
pasv_address=8.8.8.8         # public ip
pasv_min_port=36802
pasv_max_port=36822

write_enable=YES

guest_enable=YES
guest_username=ftp
virtual_use_local_privs=YES  # this is important for uploading file, it maps virtual user to system user
local_umask=022

local_root=/srv/ftp
chroot_local_user=YES
allow_writeable_chroot=YES
hide_ids=YES
```

### pam authentication
Create a PAM config file `/etc/pam.d/vsftpd` with following content:

    auth required pam_pwdfile.so pwdfile /etc/vsftpd/passwd
    account required pam_permit.so

Assure that you have package `libpam-pwdfile` installed.

Now we can add users in `/etc/vsftpd/passwd` file. Take use `zhang:123456` as
 example.

Firstly, get hashed password using `openssl passwd -1 123456`. The result is
`$1$hBWoheGJ$zFCK1dES36652BruHcF1a1` (it changes every time since it has salt).
 And then add line `zhang:$1$hBWoheGJ$zFCK1dES36652BruHcF1a1` into file
 `/etc/vsftpd/passwd`. Then you can try to connect using user `zhang` with password
 `123456`.

### passive mode
A `ftp` connection involves two channels. One is for command and one is for data
 transportation. The `listen_port` (default is 21) is used for command. For data
 transportation, it has two modes: active mode and passive mode.

For the active mode, the client opened a random port and told the server its ip
 and data port. And then the server connects to client and transfers data.

For the passive mode, the server opened another port and told the client via the
 command channel. Then client connects to this port and then transfers data.

It's easy to find that the active mode has following problems:

    - most clients are inside NAT network, they don't know their public IPs of the
      internet, it will tell the server a IP like `192.168.1.10` but it won't work
    - port of client behind NAT is not its real port
    - some firewalls reject connections that initiated from ouside

Due to these problems, passive mode is preferred. But there are also some problems
 you need to take care of.

If the server is behind NAT or you are using elastic IP in cloud, you need to set
 its public IP explicitly. You'd better specify the data port range and then you
 need to configure the firewall or cloud securty group properly to allow these ports.

Following is passive related config:

    pasv_enable=YES
    pasv_address=8.8.8.8
    pasv_min_port=36802
    pasv_max_port=36822

