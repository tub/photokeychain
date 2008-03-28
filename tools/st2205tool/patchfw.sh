#!/bin/bash

echo "this file blindly injects the code from hack/m_tomtech/ into a clean fwimage.bin (from ../../disassembly/original.fwdump"

pa=2ddd
em=3a00
x=hack/m_tomtech

echo "ASSUMING: m_tomtech and hack_jmp at 0x$pa   hack.bin at 0x$em"

cp ../../disassembly/original.fwdump fwimage.bin

./splice fwimage.bin $x/hack_jmp.bin 0x$pa >/dev/null || exit 1
./splice fwimage.bin $x/hack.bin 0x$em >/dev/null || exit 1

echo "fwimage.bin is now ready"

echo "now you can probably run ./phack -uf fwimage.bin /dev/whatever"
