#!/bin/bash

PATCH_DIR=src/patch

for asm_file in "$PATCH_DIR"/*.s; do
  obj_file=${asm_file%.s}.o
  hex_file=${asm_file%.s}.hex
  module=${asm_file##*/}
  module=${module%%_*}.dll
  address=${asm_file##*_}
  address=${address%.s}

  as -msyntax=intel -mnaked-reg -32 -march=i386+387 -o "$obj_file" "$asm_file" || exit
  ld -melf_i386 --oformat=binary -e 0 -Ttext=0 -o "$hex_file" "$obj_file" || exit

  echo "$module 0x$address :"
  echo
  od -A none -t x1 "$hex_file" || exit
  echo
  echo
done

rm "$PATCH_DIR"/*.o
rm "$PATCH_DIR"/*.hex

