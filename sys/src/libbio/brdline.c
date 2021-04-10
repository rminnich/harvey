/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<u.h>
#include	<libc.h>
#include	<bio.h>

void*
Brdline(Biobufhdr *bp, int delim)
{
	char *ip, *ep;
	int i, j;

	i = -bp->icount;
	if(i == 0) {
		/*
		 * eof or other error
		 */
		if(bp->state != Bractive) {
			if(bp->state == Bracteof)
				bp->state = Bractive;
			bp->rdline = 0;
			bp->gbuf = bp->ebuf;
			return 0;
		}
	}

	/*
	 * first try in remainder of buffer (gbuf doesn't change)
	 */
	ip = (char*)bp->ebuf - i;
	ep = memchr(ip, delim, i);
	if(ep) {
		j = (ep - ip) + 1;
		bp->rdline = j;
		bp->icount += j;
		return ip;
	}

	/*
	 * copy data to beginning of buffer
	 */
	if(i < bp->bsize)
		memmove(bp->bbuf, ip, i);
	bp->gbuf = bp->bbuf;

	/*
	 * append to buffer looking for the delim
	 */
	ip = (char*)bp->bbuf + i;
	while(i < bp->bsize) {
		j = read(bp->fid, ip, bp->bsize-i);
		if(j <= 0) {
			/*
			 * end of file with no delim
			 */
			memmove(bp->ebuf-i, bp->bbuf, i);
			bp->rdline = i;
			bp->icount = -i;
			bp->gbuf = bp->ebuf-i;
			return 0;
		}
		bp->offset += j;
		i += j;
		ep = memchr(ip, delim, j);
		if(ep) {
			/*
			 * found in new piece
			 * copy back up and reset everything
			 */
			ip = (char*)bp->ebuf - i;
			if(i < bp->bsize){
				memmove(ip, bp->bbuf, i);
				bp->gbuf = (u8*)ip;
			}
			j = (ep - (char*)bp->bbuf) + 1;
			bp->rdline = j;
			bp->icount = j - i;
			return ip;
		}
		ip += j;
	}

	/*
	 * full buffer without finding
	 */
	bp->rdline = bp->bsize;
	bp->icount = -bp->bsize;
	bp->gbuf = bp->bbuf;
	return 0;
}

int
Blinelen(Biobufhdr *bp)
{

	return bp->rdline;
}
