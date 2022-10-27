/*
This file is part of HitmanC47TimingFix licensed under the BSD 3-Clause License
the full text of which is given below.

Copyright (c) 2022-present, Mihail Ivanchev
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

.include "common/locations.s"

.ifndef str_to_int_addr
.set str_to_int_addr, 0x0FFB40F9
.endif

start:

    #
    # The following code is taken from the unmodified function
    #

    push ebp
    mov ebp, esp
    push edi
    push esi
    push ebx
    mov esi, [ebp+0xC]
    mov edi, [ebp+0x8]
    lea eax, [0x0FFE0850]
    cmp dword ptr [eax+0x8], 0
    jnz done

    #
    # The following code is new
    #

    push -1
    push esi
    push -1
    push edi
    push 1
    push 0x7F
    call [StringCompareA]
    dec eax
    dec eax
    .byte 0xE9, 0x9C, 0x00, 0x00, 0x00      # jmp 0x0FFBDA5B
    push esi
    push str_fps_addr
    call start
    test eax,eax
    pop eax
    pop eax
    jnz start
    mov eax, [ebp+0xC]
    push eax
    call str_to_int_addr - base
    mov [int_fps_addr], eax
    pop eax
    xor eax,eax
    inc eax
    ret

done:

