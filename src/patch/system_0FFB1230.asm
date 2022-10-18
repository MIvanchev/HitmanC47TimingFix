
.set LoadLibrary,                           0x0FFD3078
.set GetProcAddress,                        0x0FFD305C
.set str_kernel32_addr,                     0x0FFDAEA8
.set str_query_performance_frequency_addr,  0x0FFDAEB8
.set str_query_performance_counter_addr,    0x0FFDAED4
.set handle_query_performance_counter_addr, 0x0FFDAEEC

start:
  fld qword ptr [ecx+0xA61]
  fcomp qword ptr [0x0FFD3730]
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
.align 4, 0x90
  jnz delta_leq_1
  fst st(0)
  fld1
delta_leq_1:
  push dword ptr 45
  fild dword ptr [esp]
  pop eax
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
  ret

