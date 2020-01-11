### package manage cheatsheet

#### install related
<details open>
  <summary>debian/dpkg</summary>

    dpkg -i local.deb               # install a local package
    apt-get install linux-image-amd64=3.16+63         # install a package with specific version
</details>
<details>
  <summary>yum/rpm</summary>

    rpm -ivh local.rpm              # install a local package
</details>

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

#### basic query
  <details>
    <summary>rpm</summary>

      rpm -qi golang          # show info of a installed package

  </details>


#### updated related
<details open>
  <summary>debian/dpkg</summary>

    TODO
</details>
<details>
  <summary>yum/rpm</summary>

    rpm -Uvh golang.rpm             # upgrade from a local package
</details>

#### remove package
<details open>
  <summary>debian/dpkg</summary>

    dpkg -r linux-image-4.4.0-150-generic           # remove installed package
</details>
<details>
  <summary>yum/rpm</summary>

    rpm -ev golang             # remove package
    rpm -ev --nodeps golang    # remove without check dependencies
</details>

