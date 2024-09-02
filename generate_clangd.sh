#! /bin/bash

version=$(uname -r | cut -d'-' -f1,2)
PATH_1="/usr/src/linux-headers-$version/include"
PATH_2="/usr/src/linux-headers-$version-generic/arch/x86/include/generated"
PATH_3="/usr/src/linux-headers-$version-generic"
PATH_3="/usr/src/linux-headers-$version-generic/include/generated"
PATH_4="/usr/src/linux-headers-$version/arch/x86/include"

cat <<EOF > .clangd
CompileFlags:
  Add:
    - -I${PATH_1}
    - -I${PATH_2}
    - -I${PATH_3}
    - -I${PATH_4}
    - -D__KERNEL__
    - -DMODULE
EOF
#- -D'CONFIG_X86_L1_CACHE_SHIFT='
