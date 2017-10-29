/*Copyright(c) 2015, Job Talle
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met :

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#pragma once

#include <stdint.h>

#define POOL_BLOCKS_INITIAL 1

typedef struct poolFreed {
	struct poolFreed *nextFree;
} poolFreed;

typedef struct {
	uint32_t elementSize;
	uint32_t blockSize;
	uint32_t used;
	int32_t block;
	poolFreed *freed;
	uint32_t blocksUsed;
	uint8_t **blocks;
} pool;

void poolInitialize(pool *p, const uint32_t elementSize, const uint32_t blockSize);
void poolFreePool(pool *p);

#ifndef DISABLE_MEMORY_POOLING
void *poolMalloc(pool *p);
void poolFree(pool *p, void *ptr);
#else
#include <stdlib.h>
#define poolMalloc(p) malloc((p)->blockSize)
#define poolFree(p, d) free(d)
#endif
void poolFreeAll(pool *p);