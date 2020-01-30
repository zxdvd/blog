## package manage cheatsheet

### package managers
#### dpkg
`dpkg` is the `deb` package manager. You can use it to manage deb packages on system.

#### apt
`apt` is package manager of debian, ubuntu and variants. It's frontend of `dpkg`. You can use it to get packages from remote repositories and then install them using `dpkg` backend.

#### rpm
`rpm` is the `rpm` package manager. Often used on redhat, fedora, suse.

#### yum
`yum` is frontend of `rpm`, used on redhat, fedora. It's like `apt`.

#### zypper
`zypper` is also frontend of `rpm`, used on opensuse.

#### macports
`macports` is third-part package manager worked on mac osx. You can use it to install packages from repositories.

#### homebrew
`homebrew` is another third-part package manager on mac osx.

#### pip
`pip` is used to manage python packages.

### install package
#### install from repo or source
    # debian/ubuntu install with version
    apt# apt-get install linux-image-amd64=3.16+63

#### install a local package
    dpkg# dpkg -i golang.deb       # debian/ubuntu
    rpm# rpm -ivh local.rpm        # redhat

### query package or file
#### list all installed packages

    dpkg$ dpkg -l
    rpm$ rpm -qa
    pip$ pip list
    macports$ port installed

#### list all package and sort by size reversely

    dpkg$ dpkg-query  --show  -f='${Installed-Size} ${Package} ${Version}\n' | sort -rn
    rpm$ rpm -qa --queryformat '%{SIZE} %{NAME} %{VENDOR}\n' | sort -rn
    rpm$ rpm -qa --last          # list recent installed packages

#### list all files of a installed package

    dpkg$ dpkg -L golang
    rpm$ rpm -ql golang
    pip$ pip show -f requests
    macports$ port content python38

#### show info/description of a package

    rpm$ rpm -qi golang          # info of installed package

#### find which package owned a specific file

    dpkg$ dpkg -S /usr/bin/make
    rpm$ rpm -qf $(which make)

#### show dependencies of a package

    dpkg$ 
    rpm$ rpm -qR make
    rpm$ rpm -qR -p golang.rpm                 # show dependencies of a downloaded package

#### list all files of a local package without install it

    dpkg$ dpkg -c golang.deb
    OR shell$ ar t golang.deb            # you can also use ar since xx.deb is a archive
    dpkg$ dpkg-deb -x golang.deb         # this will extract a local package


    rpm -ql -p golang.rpm

### update package
#### upgrade from local package
    rpm# rpm -Uvh golang.rpm


### remove package
    dpkg# dpkg -r linux-image-4.4.0-150-generic
    rpm# rpm -ev golang
    rpm# rpm -ev --nodeps golang    # remove without check dependencies


### apt package priority
You can add many repos. Then you may get cases that there are many versions of a same package in different repos. So which one will be installed? How to install a specific one?

Each repo has a priority number, by default it is 500. For backport repo, it is 100. So that you won't install from backport repo by default since it has a lower priority.

Show current priority of all repos:

    # apt-cache policy
    100 http://mirrors.tencentyun.com/debian stretch-backports/main amd64 Packages
    release o=Debian Backports,a=stretch-backports,n=stretch-backports,l=Debian Backports,c=main,b=amd64
    origin mirrors.tencentyun.com
    500 http://mirrors.tencentyun.com/debian stretch/main amd64 Packages
    release v=9.11,o=Debian,a=oldstable,n=stretch,l=Debian,c=main,b=amd64
    origin mirrors.tencentyun.com


Show all candidates of a package:

    # apt-cache policy nginx
    nginx:
      Installed: 1.10.3-1+deb9u3
      Candidate: 1.16.1-3
      Version table:
        1.16.1-3 500
            500 http://mirrors.tencentyun.com/debian unstable/main amd64 Packages
        1.14.1-1~bpo9+1 100
            100 http://mirrors.tencentyun.com/debian stretch-backports/main amd64 Packages
    *** 1.10.3-1+deb9u3 500
            500 http://mirrors.tencentyun.com/debian stretch/main amd64 Packages
            500 http://mirrors.tencentyun.com/debian-security stretch/updates/main amd64 Packages


You can force install from a lower priority repo:

    ## use -V to show detail package version
    ## -t stretch-backports means install from this repo
    # apt-get -V install -t stretch-backports nginx

You can also change priority of a specific package or a repo.

Set a higher priority for backport repo:

    # add a file under /etc/apt/preferences.d/
    Package: *
    Pin: release a=stretch-backports
    Pin-Priority: 600

If you only want to set higher priority for a specific package, just change the `*` to the package name.

Multiple packages can be concated using whitespace. Like

    Package: nginx golang