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
	print("PTE %#p ", h->Pte);
	if (h->Pte)
		print("Image %#P ", h->Pte->image);
	print("va %#p ", h->va);
	print("szi %#x ", h->pgszi);
	print("maxperms %#x ", h->maxperms);
	print("perms %#x ", h->perms);
}

char *
phmapget(Proc *p, uintptr_t addr, Hpm **pp, uint64_t *type)
{
	char *err;
	uintptr_t ret;

	*type = 0;
	*pp = nil;
	uint64_t key = addr & ~0x1fffffULL;
	// TODO: look up using all page sizes.
	rlock(&p->hml);
	err = hmapget(&p->ptes, key, (uint64_t*)&ret);
	runlock(&p->hml);
	if (err) {
		print("phmapget(%d): %s\n", p->pid, err);
		return err;
	}
	*type = ret & 0xff;
	if (ret > 256) {
		*pp = (void *)(ret & ~0xff);
	}
		
	return err;
}

char *
phmapput(Proc *p, Hpm *h, int replace)
{
	char *err;
	
	uint64_t key = h->va & ~0x1fffffULL;
	// TODO: look up using all page sizes.
	print("phmapput va %#p key %#p\n", h->va, key);
	wlock(&p->hml);
	if (replace) {
		print("(%d) put: try a replace\n", p->pid);
		hmapdel(&p->ptes, key, (uint64_t*)&h);
	}
	err = hmapput(&p->ptes, key, (uint64_t)h);
	wunlock(&p->hml);
	if (err)
		print("phmapput(%d): %s\n", p->pid, err);
	return err;
}

static char *freehpm(Hashentry *h, void *arg)
{
	Proc *p = arg;
	char *err;
	Hpm *hp;
	print("Proc %p(%d): copy (%#lx, %#lx)", p, p->pid, h->key, h->val);
	err = hmapdel(&p->ptes, h->key, (uint64_t*)&hp);
	if (err != nil)
		return err;
	freepte(hp->Pte, hp->type);
	return err;
}


void
phmapfree(Proc *p)
{
	Proc *up = externup();
	char *err;
	err = hmapapply(&up->ptes, freehpm, (void *)p);
	if (err != nil)
		panic(err);

}

void phmapexit(Proc *p)
{
	int len = hmaplen(&p->ptes);
	if (len > 0)
		panic("Pid %d: hmaplen is %d, not 0", p->pid, len);
	hmapfree(&p->ptes);
}
