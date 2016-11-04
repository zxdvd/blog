+++
title = "PXE server without control of dhcp server"
date = "2014-08-21"
categories = ["Linux"]
tags = ["linux"]
slug = "pxeserver-without-dhcp"
Authors = "Xudong"
draft = true
+++

Since we do all kinds of installation frequently, PXE server is really helpful
to us. However, one of our three subnets doesn't have it since the dhcp server
of that subnet is out of our control. Most often, pxe server needs dhcp server
to help to provide some parameters to the client, like tftp server address. So
it's almost impossible to setup a pxe server without control of dhcp server.

After a lot of googling, I found the method--proxy dhcp. It's really doable and
reliable. I set up it in our subnet and it has run for more than 3 months
without problems and side-effects.

I'll explain it first. Why dhcp server is needed?

1. The client needs to get ip address from the dhcp server.
2. The client needs to get tftp server address and other parameters.

There is no problem with the first one. Client could get ip address from any
dhcp server. So the problem is the second one. Fortunately, we can overcome it
via the proxy dhcp.

Don't worry about the "two dhcp server in a same subnet" problem.  A proxy
dhcp server won't assignee ip address to any client. It will keep silence and
wait for the BOOTP client (PXE client) and send boot options like tftp server
address to the client.

Now both of the 2 important requirements are satisfied. You can enjoy your pxe
server.

### Implementation
I set up the whole services in a vagrant box (ubuntu 14.04).

1. Install dnsmasq. `#aptitude install dnsmasq`.
2. Configure the dnsmasq

        port=0 # disable dns function
        dhcp-range=192.168.1.1,proxy
        dhcp-boot=pxelinux.0
        pxe-service=x86PC, "INSTALL from PXE server", pxelinux
        pxe-service=x86PC, "Boot from local disk"
        enable-tftp
        tftp-root=/tftpboot

    I use the built-in tftp server of dnsmasq. Of course, you could use another one
if you like.

3. Set up the tftp

         = = =text
        /tftpboot/
             menu.c32
             pxelenux.0
             pxelinux.cfg/
                default
             images/
                opensuse-13.1/
                SLES11SP3/

    - Copy `menu.c32` and `pxelinux.0` file to the root of tftp server (`/tftpboot`). They
belongs to package syslinux. Location of them may vary due to different
distributions. (`/usr/lib/syslinux` for ubuntu 14.04)

    - `#mkdir /tftpboot/pxelinux.cfg`
Create a configuration file under the pxelinux.cfg folder.

            default menu.c32
            prompt 0
            timeout 300
            ONTIMEOUT local
            LABEL SLES-11-SP3-GM-x86_64
                MENU LABEL SLES-11-SP3-GM-x86_64
                    KERNEL images/SLES-11-SP3/boot/x86_64/loader/linux
                    APPEND initrd=images/SLES-11-SP3/boot/x86_64/loader/initrd  install=ftp =//192.168.1.1/install/SLES-11-SP3
                MENU LABEL openSUSE-13.1
                    KERNEL images/openSUSE-13.1/boot/x86_64/loader/linux
                    APPEND initrd=images/openSUSE-13.1/boot/x86_64/loader/initrd install=ftp =//192.168.1.1/install/openSUSE-13.1

3. Extract images to `/tftpboot/images`. And then you can set up
   ftp/http/nfs server there and use them as repositories during installation.

Now a pxe server without control of dhcp is done.

### References

1. [proxy dhcp](http =//etherboot.org/wiki/proxydhcp)

2. [pxelinux-detailed info about pxelinux.cfg conf](http =//www.syslinux.org/wiki/index.php/PXELINUX)

3. [wikipedia-pxe](http =//en.wikipedia.org/wiki/Preboot_Execution_Environment#Proxy_DHCP)
