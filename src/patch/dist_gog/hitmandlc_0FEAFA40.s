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

.include "common/consts.s"

reloc_delta:
    call base_addr
base_addr:
    mov eax, [esp]
    sub eax, base + (base_addr - reloc_delta)
    add esp, 4
    ret

# -----------------------------------------------------------------------------
# PROC updateGravityProcessingDecayValues
# -----------------------------------------------------------------------------
.align 4

    # FIXME: Preserve the FPU reg stack

    push eax
    push eax
    fst qword ptr [esp]

    fld1                        # Compute actual FPS
    fdiv dword ptr [ebp-4]
    push float_60
    push float_1
    fdiv dword ptr [esp+4]
    fcom dword ptr [esp]        # Only update if faster than threshold FPS
    pop eax
    pop eax
    fnstsw ax
    test ah, 0x41
    jnz not_above_fps_threshold
    fmul st(1), st(0)
    fmul st(1), st(0)
    fmul st(1), st(0)
not_above_fps_threshold:
    fstp st(0)
    push float_25
    fcom dword ptr [esp]
    pop eax
    fnstsw ax
    fstp st(0)
    test ah, 0x41
    jnz update_decay_params
    fld dword ptr [esi+0xa4]
    fadd qword ptr [esp]
    fst dword ptr [esi+0xa4]
    pop eax
    pop eax
    xor eax, eax
    pop esi
    mov esp, ebp
    pop ebp
    ret 4
update_decay_params:
    push double_1_004_hi
    push double_1_004_lo
    push double_0_9_hi
    push double_0_9_lo

    #
    # Code copied from original function & modified
    #

    fld dword ptr [esi+0xa4]
    fmul qword ptr [esp]
    fadd qword ptr [esp+0x10]
    fst dword ptr [esi+0xa4]
    fcomp dword ptr [esi+0xa8]
    fnstsw ax
    test ah, 1
    je gravity_still_active
    mov byte ptr [esi+0xe0],0
gravity_still_active:
    fld dword ptr [esi+0xa8]
    fmul qword ptr [esp+8]
    fstp dword ptr [esi+0xa8]
    add esp, 0x18
    xor eax, eax
    pop esi
    mov esp, ebp
    pop ebp
    ret 4

# -----------------------------------------------------------------------------
# PROC updateGravityCutoffTimer
# -----------------------------------------------------------------------------
.align 4

    # FIXME: Preserve the FPU reg stack

    fld dword ptr [esi+0xa0]
    ftst
    fnstsw ax
    test ah, 0x41
    jnz gravity_timer_expired
    call reloc_delta
    mov eax, [eax+0x0FEB000C]
    mov eax, [eax]
    fadd qword ptr [eax+0x37CD]
    fsub qword ptr [eax+0x37C5]
    ftst
    fnstsw ax
    test ah, 0x41
    jz gravity_timer_active
    fstp st(0)
    fldz
    xor eax, eax
    mov dword ptr [esi+0xe0], eax
gravity_timer_active:
    fst dword ptr [esi+0xa0]
gravity_timer_expired:
    fstp st(0)
    ret

