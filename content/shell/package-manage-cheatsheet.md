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

#### query package or file
* basic query
  <details open>
    <summary>dpkg</summary>

      dpkg -l                 # dpkg --list, list all installed package
      # list all package and sort by size reversely
      dpkg-query  --show  -f='${Installed-Size} ${Package} ${Version}\n' | sort -rn
      dpkg -L golang          # list all files of a installed package
      dpkg -S /usr/bin/make   # find which package own a specific file

  </details>
  <details>
    <summary>rpm</summary>

      rpm -qa                 # list all installed package
      # list all package and sort by size reversely
      rpm -qa --queryformat '%{SIZE} %{NAME} %{VENDOR}\n' | sort -rn
      rpm -qa --last          # list recent installed packages
      rpm -ql golang          # list all files of a installed package
      rpm -qi golang          # show info of a installed package
      rpm -qf $(which make)   # find which package own a specific file

  </details>

* query a local package
  <details open>
    <summary>dpkg</summary>

      dpkg -c golang.deb                   # list all files of a local package WITHOUT install
      ar t golang.deb                      # this also list all files of deb package since it is a archive
      dpkg-deb -x golang.deb               # this will EXTRACT a local deb package
  </details>
  <details>
    <summary>rpm</summary>

      rpm -qlp golang.rpm                  # list all files of a local package WITHOUT install
  </details>

 * query dependency
  <details open>
    <summary>dpkg</summary>

      TODO
  </details>
  <details>
    <summary>rpm</summary>

      rpm -qR make                         # query dependency of make (make depends on what?)
      rpm -qRp golang.rpm                  # query depency of a local package
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

