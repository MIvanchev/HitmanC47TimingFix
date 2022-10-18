mov eax, [0x0FFDAEEC]
lea ebx, [ebp-0x14]
test eax, eax
.byte 0x74, 0x0FFB1738-0x0FFB174C-$-1   # JE
push ebx
call eax

