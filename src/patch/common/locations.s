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

.set LoadLibrary,                           0x0FFD3078
.set GetProcAddress,                        0x0FFD305C

.set handle_query_performance_counter_addr, 0x0FFDAEA8
.set int_fps_addr,                          0x0FFDAEAC

.set getPerformanceCounterFrequency,        0x0FFD28E4
.set getPerformanceCounterIfInitialized,    0x0FFD2974
.set scaleTimeDeltaToTargetFps,             0x0FFD29A0
.set tryParseFpsValueFromConfig,            0x0FFD29CC

.set updateGravityProcessingDecayValues,    0x0FEAFA54
.set updateGravityCutoffTimer,              0x0FEAFB04

.ifdef PATCH_FOR_GOG

.set compareStringsCaseInsensitive,         0x0FFBD990
.set stringToInt,                           0x0FFB40F9

.else
.ifdef PATCH_FOR_OTHER

.set compareStringsCaseInsensitive,         0x0FFBD940
.set stringToInt,                           0x0FFBD9C9

.else
.error "You need to specify which game distribution to build the patcher for."
.endif
.endif
