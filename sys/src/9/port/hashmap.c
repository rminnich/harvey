/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

void
dumphpm(Hpm *h)
{
	print("Type %#x ", h->type);
	print("Base %#x ", h->base);
	print("Size %#x ", h->size);
	print("chan %#p ", h->chan);
	print("off %#x ", h->offset);
	print("pte %#p ", h->pte);
}

Hpm *
phmapget(Proc *p, uintptr_t addr)
{
	char *err;
	uintptr_t ret;

	uint64_t key = addr & ~0x1fffffULL;
	// TODO: look up using all page sizes.
	err = hmapget(&p->pages, key, (uint64_t*)&ret);
	if (err)
		print("phmapget(%d): %s\n", p->pid, err);
	return (Hpm*)ret;
}

char *
phmapput(Proc *p, Hpm *h)
{
	char *err;

	uint64_t key = h->base & ~0x1fffffULL;
	// TODO: look up using all page sizes.
	err = hmapput(&p->pages, key, (uint64_t)h);
	if (err)
		print("phmapput(%d): %s\n", p->pid, err);
	return err;
}
