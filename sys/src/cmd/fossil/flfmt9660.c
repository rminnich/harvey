/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Initialize a fossil file system from an ISO9660 image already in the
 * file system.  This is a fairly bizarre thing to do, but it lets us generate
 * installation CDs that double as valid Plan 9 disk partitions.
 * People having trouble booting the CD can just copy it into a disk
 * partition and you've got a working Plan 9 system.
 *
 * I've tried hard to keep all the associated cruft in this file.
 * If you deleted this file and cut out the three calls into it from flfmt.c,
 * no traces would remain.
 */

#include "stdinc.h"
#include "dat.h"
#include "fns.h"
#include "flfmt9660.h"
#include <bio.h>
#include <ctype.h>

static Biobuf *b;

enum{
	Tag = 0x96609660,
	Blocksize = 2048,
};


typedef struct Voldesc Voldesc;
struct Voldesc {
	u8	magic[8];	/* 0x01, "CD001", 0x01, 0x00 */
	u8	systemid[32];	/* system identifier */
	u8	volumeid[32];	/* volume identifier */
	u8	unused[8];	/* character set in secondary desc */
	u8	volsize[8];	/* volume size */
	u8	charset[32];
	u8	volsetsize[4];	/* volume set size = 1 */
	u8	volseqnum[4];	/* volume sequence number = 1 */
	u8	blocksize[4];	/* logical block size */
	u8	pathsize[8];	/* path table size */
	u8	lpathloc[4];	/* Lpath */
	u8	olpathloc[4];	/* optional Lpath */
	u8	mpathloc[4];	/* Mpath */
	u8	ompathloc[4];	/* optional Mpath */
	u8	rootdir[34];	/* root directory */
	u8	volsetid[128];	/* volume set identifier */
	u8	publisher[128];
	u8	prepid[128];	/* data preparer identifier */
	u8	applid[128];	/* application identifier */
	u8	notice[37];	/* copyright notice file */
	u8	abstract[37];	/* abstract file */
	u8	biblio[37];	/* bibliographic file */
	u8	cdate[17];	/* creation date */
	u8	mdate[17];	/* modification date */
	u8	xdate[17];	/* expiration date */
	u8	edate[17];	/* effective date */
	u8	fsvers;		/* file system version = 1 */
};

/* Not used?
static void
dumpbootvol(void *a)
{
	Voldesc *v;

	v = a;
	print("magic %.2x %.5s %.2x %2ux\n",
		v->magic[0], v->magic+1, v->magic[6], v->magic[7]);
	if(v->magic[0] == 0xFF)
		return;

	print("system %.32T\n", v->systemid);
	print("volume %.32T\n", v->volumeid);
	print("volume size %.4N\n", v->volsize);
	print("charset %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x\n",
		v->charset[0], v->charset[1], v->charset[2], v->charset[3],
		v->charset[4], v->charset[5], v->charset[6], v->charset[7]);
	print("volume set size %.2N\n", v->volsetsize);
	print("volume sequence number %.2N\n", v->volseqnum);
	print("logical block size %.2N\n", v->blocksize);
	print("path size %.4L\n", v->pathsize);
	print("lpath loc %.4L\n", v->lpathloc);
	print("opt lpath loc %.4L\n", v->olpathloc);
	print("mpath loc %.4B\n", v->mpathloc);
	print("opt mpath loc %.4B\n", v->ompathloc);
	print("rootdir %D\n", v->rootdir);
	print("volume set identifier %.128T\n", v->volsetid);
	print("publisher %.128T\n", v->publisher);
	print("preparer %.128T\n", v->prepid);
	print("application %.128T\n", v->applid);
	print("notice %.37T\n", v->notice);
	print("abstract %.37T\n", v->abstract);
	print("biblio %.37T\n", v->biblio);
	print("creation date %.17s\n", v->cdate);
	print("modification date %.17s\n", v->mdate);
	print("expiration date %.17s\n", v->xdate);
	print("effective date %.17s\n", v->edate);
	print("fs version %d\n", v->fsvers);
}
*/

typedef struct Cdir Cdir;
struct Cdir {
	u8	len;
	u8	xlen;
	u8	dloc[8];
	u8	dlen[8];
	u8	date[7];
	u8	flags;
	u8	unitsize;
	u8	gapsize;
	u8	volseqnum[4];
	u8	namelen;
	u8	name[1];	/* chumminess */
};

static int
Dfmt(Fmt *fmt)
{
	char buf[128];
	Cdir *c;

	c = va_arg(fmt->args, Cdir*);
	if((c->namelen == 1 && c->name[0] == '\0') || c->name[0] == '\001') {
		snprint(buf, sizeof buf, ".%s dloc %.4N dlen %.4N",
			c->name[0] ? "." : "", c->dloc, c->dlen);
	} else {
		snprint(buf, sizeof buf, "%.*T dloc %.4N dlen %.4N", c->namelen, c->name,
			c->dloc, c->dlen);
	}
	fmtstrcpy(fmt, buf);
	return 0;
}

char longc, shortc;
/* Not used?
static void
bigend(void)
{
	longc = 'B';
}

static void
littleend(void)
{
	longc = 'L';
}
*/

static u32
big(void *a, int n)
{
	u8 *p;
	u32 v;
	int i;

	p = a;
	v = 0;
	for(i=0; i<n; i++)
		v = (v<<8) | *p++;
	return v;
}

static u32
little(void *a, int n)
{
	u8 *p;
	u32 v;
	int i;

	p = a;
	v = 0;
	for(i=0; i<n; i++)
		v |= (*p++<<(i*8));
	return v;
}

/* numbers in big or little endian. */
static int
BLfmt(Fmt *fmt)
{
	u32 v;
	u8 *p;
	char buf[20];

	p = va_arg(fmt->args, u8*);

	if(!(fmt->flags&FmtPrec)) {
		fmtstrcpy(fmt, "*BL*");
		return 0;
	}

	if(fmt->r == 'B')
		v = big(p, fmt->prec);
	else
		v = little(p, fmt->prec);

	sprint(buf, "0x%.*lux", fmt->prec*2, v);
	fmt->flags &= ~FmtPrec;
	fmtstrcpy(fmt, buf);
	return 0;
}

/* numbers in both little and big endian */
static int
Nfmt(Fmt *fmt)
{
	char buf[100];
	u8 *p;

	p = va_arg(fmt->args, u8*);

	sprint(buf, "%.*L %.*B", fmt->prec, p, fmt->prec, p+fmt->prec);
	fmt->flags &= ~FmtPrec;
	fmtstrcpy(fmt, buf);
	return 0;
}

static int
asciiTfmt(Fmt *fmt)
{
	char *p, buf[256];
	int i;

	p = va_arg(fmt->args, char*);
	for(i=0; i<fmt->prec; i++)
		buf[i] = *p++;
	buf[i] = '\0';
	for(p=buf+strlen(buf); p>buf && p[-1]==' '; p--)
		;
	p[0] = '\0';
	fmt->flags &= ~FmtPrec;
	fmtstrcpy(fmt, buf);
	return 0;
}

static void
ascii(void)
{
	fmtinstall('T', asciiTfmt);
}

/*Not used?
static int
runeTfmt(Fmt *fmt)
{
	Rune buf[256], *r;
	int i;
	u8 *p;

	p = va_arg(fmt->args, u8*);
	for(i=0; i*2+2<=fmt->prec; i++, p+=2)
		buf[i] = (p[0]<<8)|p[1];
	buf[i] = L'\0';
	for(r=buf+i; r>buf && r[-1]==L' '; r--)
		;
	r[0] = L'\0';
	fmt->flags &= ~FmtPrec;
	return fmtprint(fmt, "%S", buf);
}
*/

static void
getsect(u8 *buf, int n)
{
	if(Bseek(b, n*2048, 0) != n*2048 || Bread(b, buf, 2048) != 2048)
{
abort();
		sysfatal("reading block at %,d: %r", n*2048);
}
}

static Header *h;
static int fd;
static char *file9660;
static int off9660;
static u32 startoff;
static u32 endoff;
static u32 fsoff;
static u8 root[2048];
static Voldesc *v;
static u32 iso9660start(Cdir*);
static void iso9660copydir(Fs*, File*, Cdir*);
static void iso9660copyfile(Fs*, File*, Cdir*);

void
iso9660init(int xfd, Header *xh, char *xfile9660, int xoff9660)
{
	u8 sect[2048], sect2[2048];

	fmtinstall('L', BLfmt);
	fmtinstall('B', BLfmt);
	fmtinstall('N', Nfmt);
	fmtinstall('D', Dfmt);

	fd = xfd;
	h = xh;
	file9660 = xfile9660;
	off9660 = xoff9660;

	if((b = Bopen(file9660, OREAD)) == nil)
		vtFatal("Bopen %s: %r", file9660);

	getsect(root, 16);
	ascii();

	v = (Voldesc*)root;
	if(memcmp(v->magic, "\001CD001\001\000", 8) != 0)
		vtFatal("%s not a cd image", file9660);

	startoff = iso9660start((Cdir*)v->rootdir)*Blocksize;
	endoff = little(v->volsize, 4);	/* already in bytes */

	fsoff = off9660 + h->data*h->blockSize;
	if(fsoff > startoff)
		vtFatal("fossil data starts after cd data");
	if(off9660 + (i64)h->end*h->blockSize < endoff)
		vtFatal("fossil data ends before cd data");
	if(fsoff%h->blockSize)
		vtFatal("cd offset not a multiple of fossil block size");

	/* Read "same" block via CD image and via Fossil image */
	getsect(sect, startoff/Blocksize);
	if(seek(fd, startoff-off9660, 0) < 0)
		vtFatal("cannot seek to first data sector on cd via fossil");
fprint(2, "look for %lu at %lu\n", startoff, startoff-off9660);
	if(readn(fd, sect2, Blocksize) != Blocksize)
		vtFatal("cannot read first data sector on cd via fossil");
	if(memcmp(sect, sect2, Blocksize) != 0)
		vtFatal("iso9660 offset is a lie %08ux %08ux",
		        *(i32*)sect, *(i32*)sect2);
}

void
iso9660labels(Disk *disk, u8 *buf, void (*write)(int, u32))
{
	u32 sb, eb, bn, lb, llb;
	Label l;
	int lpb;
	u8 sect[Blocksize];

	if(!diskReadRaw(disk, PartData, (startoff-fsoff)/h->blockSize, buf))
		vtFatal("disk read failed: %r");
	getsect(sect, startoff/Blocksize);
	if(memcmp(buf, sect, Blocksize) != 0)
		vtFatal("fsoff is wrong");

	sb = (startoff-fsoff)/h->blockSize;
	eb = (endoff-fsoff+h->blockSize-1)/h->blockSize;

	lpb = h->blockSize/LabelSize;

	/* for each reserved block, mark label */
	llb = ~0;
	l.type = BtData;
	l.state = BsAlloc;
	l.tag = Tag;
	l.epoch = 1;
	l.epochClose = ~(u32)0;
	for(bn=sb; bn<eb; bn++){
		lb = bn/lpb;
		if(lb != llb){
			if(llb != ~0)
				(*write)(PartLabel, llb);
			memset(buf, 0, h->blockSize);
		}
		llb = lb;
		labelPack(&l, buf, bn%lpb);
	}
	if(llb != ~0)
		(*write)(PartLabel, llb);
}

void
iso9660copy(Fs *fs)
{
	File *root;

	root = fileOpen(fs, "/active");
	iso9660copydir(fs, root, (Cdir*)v->rootdir);
	fileDecRef(root);
	vtRUnlock(fs->elk);
	if(!fsSnapshot(fs, nil, nil, 0))
		vtFatal("snapshot failed: %R");
	vtRLock(fs->elk);
}

/*
 * The first block used is the first data block of the leftmost file in the tree.
 * (Just an artifact of how mk9660 works.)
 */
static u32
iso9660start(Cdir *c)
{
	u8 sect[Blocksize];

	while(c->flags&2){
		getsect(sect, little(c->dloc, 4));
		c = (Cdir*)sect;
		c = (Cdir*)((u8*)c+c->len);	/* skip dot */
		c = (Cdir*)((u8*)c+c->len);	/* skip dotdot */
		/* oops: might happen if leftmost directory is empty or leftmost file is zero length! */
		if(little(c->dloc, 4) == 0)
			vtFatal("error parsing cd image or unfortunate cd image");
	}
	return little(c->dloc, 4);
}

static void
iso9660copydir(Fs *fs, File *dir, Cdir *cd)
{
	u32 off, end, len;
	u8 sect[Blocksize], *esect, *p;
	Cdir *c;

	len = little(cd->dlen, 4);
	off = little(cd->dloc, 4)*Blocksize;
	end = off+len;
	esect = sect+Blocksize;

	for(; off<end; off+=Blocksize){
		getsect(sect, off/Blocksize);
		p = sect;
		while(p < esect){
			c = (Cdir*)p;
			if(c->len <= 0)
				break;
			if(c->namelen!=1 || c->name[0]>1)
				iso9660copyfile(fs, dir, c);
			p += c->len;
		}
	}
}

static char*
getname(u8 **pp)
{
	u8 *p;
	int l;

	p = *pp;
	l = *p;
	*pp = p+1+l;
	if(l == 0)
		return "";
	memmove(p, p+1, l);
	p[l] = 0;
	return (char*)p;
}

static char*
getcname(Cdir *c)
{
	u8 *up;
	char *p, *q;

	up = &c->namelen;
	p = getname(&up);
	for(q=p; *q; q++)
		*q = tolower(*q);
	return p;
}

static char
dmsize[12] =
{
	31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
};

static u32
getcdate(u8 *p)	/* yMdhmsz */
{
	Tm tm;
	int y, M, d, h, m, s, tz;

	y=p[0]; M=p[1]; d=p[2];
	h=p[3]; m=p[4]; s=p[5]; tz=p[6];
	USED(tz);
	if (y < 70)
		return 0;
	if (M < 1 || M > 12)
		return 0;
	if (d < 1 || d > dmsize[M-1])
		return 0;
	if (h > 23)
		return 0;
	if (m > 59)
		return 0;
	if (s > 59)
		return 0;

	memset(&tm, 0, sizeof tm);
	tm.sec = s;
	tm.min = m;
	tm.hour = h;
	tm.mday = d;
	tm.mon = M-1;
	tm.year = 1900+y;
	tm.zone[0] = 0;
	return tm2sec(&tm);
}

static int ind;

static void
iso9660copyfile(Fs *fs, File *dir, Cdir *c)
{
	Dir d;
	DirEntry de;
	int sysl;
	u8 score[VtScoreSize];
	u32 off, foff, len, mode;
	u8 *p;
	File *f;

	ind++;
	memset(&d, 0, sizeof d);
	p = c->name + c->namelen;
	if(((uintptr)p) & 1)
		p++;
	sysl = (u8*)c + c->len - p;
	if(sysl <= 0)
		vtFatal("missing plan9 directory entry on %d/%d/%.*s", c->namelen, c->name[0], c->namelen, c->name);
	d.name = getname(&p);
	d.uid = getname(&p);
	d.gid = getname(&p);
	if((uintptr)p & 1)
		p++;
	d.mode = little(p, 4);
	if(d.name[0] == 0)
		d.name = getcname(c);
	d.mtime = getcdate(c->date);
	d.atime = d.mtime;

if(d.mode&DMDIR)	print("%*scopy %s %s %s %lo\n", ind*2, "", d.name, d.uid, d.gid, d.mode);

	mode = d.mode&0777;
	if(d.mode&DMDIR)
		mode |= ModeDir;
	if((f = fileCreate(dir, d.name, mode, d.uid)) == nil)
		vtFatal("could not create file '%s': %r", d.name);
	if(d.mode&DMDIR)
		iso9660copydir(fs, f, c);
	else{
		len = little(c->dlen, 4);
		off = little(c->dloc, 4)*Blocksize;
		for(foff=0; foff<len; foff+=h->blockSize){
			localToGlobal((off+foff-fsoff)/h->blockSize, score);
			if(!fileMapBlock(f, foff/h->blockSize, score, Tag))
				vtFatal("fileMapBlock: %R");
		}
		if(!fileSetSize(f, len))
			vtFatal("fileSetSize: %R");
	}
	if(!fileGetDir(f, &de))
		vtFatal("fileGetDir: %R");
	de.uid = d.uid;
	de.gid = d.gid;
	de.mtime = d.mtime;
	de.atime = d.atime;
	de.mode = d.mode&0777;
	if(!fileSetDir(f, &de, "sys"))
		vtFatal("fileSetDir: %R");
	fileDecRef(f);
	ind--;
}
