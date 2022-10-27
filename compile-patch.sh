#!/bin/bash

echo "#ifndef __PATCHES_H__"
echo "#define __PATCHES_H__"
echo
echo "struct patched_bytes {"
echo "    DWORD addr;"
echo "    const char *bytes;"
echo "};"
echo
echo "static struct patched_bytes changes[] = {"

FIRST_DISTRIB=1

for distrib_path in src/patch/dist_*; do

    distrib=${distrib_path##*_}
    distrib=${distrib^^}

    if [[ $FIRST_DISTRIB -eq 1 ]]; then
        echo "#ifdef PATCH_FOR_$distrib"
        FIRST_DISTRIB=0
    else
        echo "#elif defined PATCH_FOR_$distrib"
    fi

    for asm_file in "$distrib_path"/*.s; do
      obj_file=${asm_file%.s}.o
      hex_file=${asm_file%.s}.hex
      module=${asm_file##*/}
      module=${module%%_*}.dll
      address=${asm_file##*_}
      address=${address%.s}

      as -msyntax=intel -mnaked-reg -32 -march=i386+387 --defsym base=0x$address -Isrc/patch -o "$obj_file" "$asm_file" || exit
      ld -melf_i386 --oformat=binary -e 0 -Ttext=0 -o "$hex_file" "$obj_file" || exit

      hexdump -e '16/1 "%02x " "\n"' "$hex_file" | \
      sed -e "s/ *$//" \
          -e "1 s/^.*$/    { 0x$address, \"\0\"/" \
          -e '2,$ s/^.*$/                  "\0"/' \
          -e '$ s/$/ },/' || exit
    done

    echo "    { 0, NULL }"

    rm "$distrib_path"/*.o || exit
    rm "$distrib_path"/*.hex || exit
done

echo "#else"
echo "#error You need to specify which game distribution to build the patcher for."
echo "#endif"
echo "};"
echo
echo "#endif /* __PATCHES_H__ */"
echo
