/*Copyright(c) 2015, Job Talle
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met :

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#include <string.h>
#include <stdlib.h>

#include "MemoryPool.h"

#ifndef max
#define max(a,b) ((a)<(b)?(b):(a))
#endif

void poolInitialize(pool *p, const uint32_t elementSize, const uint32_t blockSize)
{
	uint32_t i;

	p->elementSize = max(elementSize, sizeof(poolFreed));
	p->blockSize = blockSize;

	poolFreeAll(p);

	p->blocksUsed = POOL_BLOCKS_INITIAL;
	p->blocks = malloc(sizeof(uint8_t*)* p->blocksUsed);

	for (i = 0; i < p->blocksUsed; ++i)
		p->blocks[i] = NULL;
}

void poolFreePool(pool *p)
{
	uint32_t i;
	for (i = 0; i < p->blocksUsed; ++i) {
		if (p->blocks[i] == NULL)
			break;
		else
			free(p->blocks[i]);
	}

	free(p->blocks);
}

#ifndef DISABLE_MEMORY_POOLING
void *poolMalloc(pool *p)
{
	if (p->freed != NULL) {
		void *recycle = p->freed;
		p->freed = p->freed->nextFree;
		return recycle;
	}

	if (++p->used == p->blockSize) {
		p->used = 0;
		if (++p->block == (int32_t)p->blocksUsed) {
			uint32_t i;

			p->blocksUsed <<= 1;
			p->blocks = realloc(p->blocks, sizeof(uint8_t*)* p->blocksUsed);

			for (i = p->blocksUsed >> 1; i < p->blocksUsed; ++i)
				p->blocks[i] = NULL;
		}

		if (p->blocks[p->block] == NULL)
			p->blocks[p->block] = malloc(p->elementSize * p->blockSize);
	}

	return p->blocks[p->block] + p->used * p->elementSize;
}

void poolFree(pool *p, void *ptr)
{
	poolFreed *pFreed = p->freed;

	p->freed = ptr;
	p->freed->nextFree = pFreed;
}
#endif

void poolFreeAll(pool *p)
{
	p->used = p->blockSize - 1;
	p->block = -1;
	p->freed = NULL;
}