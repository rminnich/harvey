/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bin.h>

enum
{
	StructAlign = sizeof(union {i64 vl; double d; u32 p; void *v;
				struct{i64 v;}vs; struct{double d;}ds; struct{u32 p;}ss; struct{void *v;}xs;})
};

enum
{
	BinSize	= 8*1024
};

struct Bin
{
	Bin	*next;
	u32	total;			/* total bytes allocated in can->next */
	usize	pos;
	usize	end;
	usize	v;			/* last value allocated */
	u8	body[BinSize];
};

/*
 * allocator which allows an entire set to be freed at one time
 */
static Bin*
mkbin(Bin *bin, u32 size)
{
	Bin *b;

	size = ((size << 1) + (BinSize - 1)) & ~(BinSize - 1);
	b = malloc(sizeof(Bin) + size - BinSize);
	if(b == nil)
		return nil;
	b->next = bin;
	b->total = 0;
	if(bin != nil)
		b->total = bin->total + bin->pos - (usize)bin->body;
	b->pos = (usize)b->body;
	b->end = b->pos + size;
	return b;
}

void*
binalloc(Bin **bin, u32 size, int zero)
{
	Bin *b;
	usize p;

	if(size == 0)
		size = 1;
	b = *bin;
	if(b == nil){
		b = mkbin(nil, size);
		if(b == nil)
			return nil;
		*bin = b;
	}
	p = b->pos;
	p = (p + (StructAlign - 1)) & ~(StructAlign - 1);
	if(p + size > b->end){
		b = mkbin(b, size);
		if(b == nil)
			return nil;
		*bin = b;
		p = b->pos;
	}
	b->pos = p + size;
	b->v = p;
	if(zero)
		memset((void*)p, 0, size);
	return (void*)p;
}

void*
bingrow(Bin **bin, void *op, u32 osize, u32 size, int zero)
{
	Bin *b;
	void *np;
	usize p;

	p = (usize)op;
	b = *bin;
	if(b != nil && p == b->v && p + size <= b->end){
		b->pos = p + size;
		if(zero)
			memset((char*)p + osize, 0, size - osize);
		return op;
	}
	np = binalloc(bin, size, zero);
	if(np == nil)
		return nil;
	memmove(np, op, osize);
	return np;
}

void
binfree(Bin **bin)
{
	Bin *last;

	while(*bin != nil){
		last = *bin;
		*bin = (*bin)->next;
		last->pos = (usize)last->body;
		free(last);
	}
}
