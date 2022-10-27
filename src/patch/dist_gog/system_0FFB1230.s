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

start:
    fldz
    fld qword ptr [ecx+0xA61]
    fcomp
    fnstsw ax
    test ah, 0x40
    jnz cont
    ret
cont:
    push eax                  # | shorter "sub esp, 8"
    push eax                  # |
    push ebx
    mov ebx, ecx
    push esi
    mov edx, str_query_performance_frequency_addr
    push edx
    mov dl, handle_query_performance_counter_addr & 0xFF
    push edx
    mov dl, str_query_performance_counter_addr & 0xFF
    push edx
    mov dl, str_kernel32_addr & 0xFF
    push edx
    call [LoadLibrary]        # kernel32.dll
    mov esi, eax
    push eax
    call [GetProcAddress]     # QueryPerformanceCounter
    pop edx
    mov [edx], eax
    push esi
    call [GetProcAddress]     # QueryPerformanceFrequency
    lea esi, [esp+0x8]
    push esi
    call eax                  # QueryPerformanceFrequency
    fild qword ptr [esi]
    fstp qword ptr [esi]
    pop esi

    #
    # The following code is taken from the unmodified function
    #

    mov eax, [0x0FFD3024]
    mov ecx, [eax]
    mov al, byte ptr [ecx+0x38F1]
    test al, al
    jnz short unknown1
    mov edx, [0x0FFD3028]
    mov ecx, [edx]
    mov eax, [ecx]
    push 0x8DC
    push 0x0FFDA268
    call [eax+0x20]
    mov edx, [esp+8]
    mov ecx, [eax]
    push edx
    mov edx, [esp+8]
    push edx
    push 0x0FFDAACC
    push eax
    call [ecx+0x2C]
    add esp, 0x10
unknown1:
    mov eax, [0x0FFD9A44]
    test eax, eax
    jnz unknown2
    call [0xFFD9A4C]
unknown2:
    mov eax, [esp+4]
    mov ecx, [esp+8]
    mov [ebx+0xA61],eax
    mov [ebx+0xA65],ecx
    pop ebx
    pop eax                   # | shorter "add esp, 8"
    pop eax                   # |
    ret

    #
    # The following code is new
    #

.align 4, 0x90
    jnz delta_leq_1
    fstp st(0)
    fld1
delta_leq_1:
    mov eax, int_fps_addr
    cmp dword ptr [eax], 0
    jle done
    fild dword ptr [eax]
    fld1
    fdiv st(0), st(1)
    fcomp st(2)
    fnstsw ax
    test ah, 0x41
    jnz delta_within_limit
    fmul st(0), st(1)
    fmul st(1), st(0)
delta_within_limit:
    fstp st
done:
    ret

