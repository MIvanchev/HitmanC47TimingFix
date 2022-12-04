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

reloc_delta:
    call base_addr
base_addr:
    mov eax, [esp]
    sub eax, base + (base_addr - reloc_delta)
    add esp, 4
    ret

# -----------------------------------------------------------------------------
# PROC getPerformanceCounterFrequency
# -----------------------------------------------------------------------------
.align 4

    call reloc_delta
    mov edi, eax

    add eax, offset (base + str_kernel32)
    push eax
    call [edi+LoadLibrary]
    mov esi, eax

    mov eax, edi
    add eax, offset (base + str_QueryPerformanceCounter)
    push eax
    push esi
    call [edi+GetProcAddress]
    mov [edi+handle_query_performance_counter_addr], eax

    mov eax, edi
    add eax, offset (base + str_QueryPerformanceFrequency)
    push eax
    push esi
    call [edi+GetProcAddress]

    lea esi, [esp+0x14]         # call QueryPerformanceFrequency
    push esi
    call eax

    fild qword ptr [esp+0x14]
    fstp qword ptr [esp+0x14]

    ret

.align 4
str_kernel32:                   .asciz "kernel32.dll"
.align 4
str_QueryPerformanceFrequency:  .asciz "QueryPerformanceFrequency"
.align 4
str_QueryPerformanceCounter:    .asciz "QueryPerformanceCounter"

# -----------------------------------------------------------------------------
# PROC getPerformanceCounterIfInitialized
# -----------------------------------------------------------------------------
.align 4

    call reloc_delta
    mov eax, [eax+handle_query_performance_counter_addr]
    mov ebx, [esp+4]
    test eax, eax
    jz query_performance_counter_not_loaded
    push ebx
    call eax
    ret 4
query_performance_counter_not_loaded:
    mov dword ptr [ebx], 0
    mov dword ptr [ebx+4], 0
    ret 4

# -----------------------------------------------------------------------------
# PROC scaleTimeDeltaToTargetFps
# -----------------------------------------------------------------------------
.align 4

    jnz delta_leq_1
    fstp st(0)
    fld1
delta_leq_1:
    call reloc_delta
    add eax, int_fps_addr
    cmp dword ptr [eax], 0
    jle fps_not_set
    fild dword ptr [eax]
    fld1
    fdiv st(0), st(1)
    fcomp st(2)
    fnstsw ax
    test ah, 0x41
    jnz delta_above_threshold
    fmul st(0), st(1)
    fmul st(1), st(0)
delta_above_threshold:
    fstp st
fps_not_set:
    ret
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 # -----------------------------------------------------------------------------
# PROC tryParseFpsValueFromConfig
# -----------------------------------------------------------------------------
.align 4

    push eax
    mov eax, [esp+4]
    mov [esp+0xC], eax
    pop eax
    add esp, 8
    test eax, eax
    jnz check_for_fps_attrib
    ret
check_for_fps_attrib:
    call reloc_delta
    push eax
    push esi
    mov eax, [esp+4]
    add eax, offset (base + str_fps)
    push eax
    call compareStringsCaseInsensitive - base
    add esp, 8
    test eax,eax
    jz parse_fps_val
    add esp, 4
    ret
parse_fps_val:
    mov eax, [ebp+0xC]
    push eax
    call stringToInt - base
    push eax
    mov eax, [esp+8]
    pop [eax+int_fps_addr]
    pop eax
    pop eax
    xor eax, eax
    inc eax
    test eax, eax
    ret

.align 4
str_fps: .asciz "Fps"

