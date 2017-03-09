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

void
_sha1block(uint8_t *p, uint32_t len, uint32_t *s)
{
	uint32_t a, b, c, d, e, x;
	uint8_t *end;
	uint32_t *wp, *wend;
	uint32_t w[80];

	/* at this point, we have a multiple of 64 bytes */
	for(end = p+len; p < end;){
		a = s[0];
		b = s[1];
		c = s[2];
		d = s[3];
		e = s[4];

		wend = w + 15;
		for(wp = w; wp < wend; wp += 5){
			wp[0] = (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
			e += ((a<<5) | (a>>27)) + wp[0];
			e += 0x5a827999 + (((c^d)&b)^d);
			b = (b<<30)|(b>>2);

			wp[1] = (p[4]<<24) | (p[5]<<16) | (p[6]<<8) | p[7];
			d += ((e<<5) | (e>>27)) + wp[1];
			d += 0x5a827999 + (((b^c)&a)^c);
			a = (a<<30)|(a>>2);

			wp[2] = (p[8]<<24) | (p[9]<<16) | (p[10]<<8) | p[11];
			c += ((d<<5) | (d>>27)) + wp[2];
			c += 0x5a827999 + (((a^b)&e)^b);
			e = (e<<30)|(e>>2);

			wp[3] = (p[12]<<24) | (p[13]<<16) | (p[14]<<8) | p[15];
			b += ((c<<5) | (c>>27)) + wp[3];
			b += 0x5a827999 + (((e^a)&d)^a);
			d = (d<<30)|(d>>2);

			wp[4] = (p[16]<<24) | (p[17]<<16) | (p[18]<<8) | p[19];
			a += ((b<<5) | (b>>27)) + wp[4];
			a += 0x5a827999 + (((d^e)&c)^e);
			c = (c<<30)|(c>>2);

			p += 20;
		}

		wp[0] = (p[0]<<24) | (p[1]<<16) | (p[2]<<8) | p[3];
		e += ((a<<5) | (a>>27)) + wp[0];
		e += 0x5a827999 + (((c^d)&b)^d);
		b = (b<<30)|(b>>2);

		x = wp[-2] ^ wp[-7] ^ wp[-13] ^ wp[-15];
		wp[1] = (x<<1) | (x>>31);
		d += ((e<<5) | (e>>27)) + wp[1];
		d += 0x5a827999 + (((b^c)&a)^c);
		a = (a<<30)|(a>>2);

		x = wp[-1] ^ wp[-6] ^ wp[-12] ^ wp[-14];
		wp[2] = (x<<1) | (x>>31);
		c += ((d<<5) | (d>>27)) + wp[2];
		c += 0x5a827999 + (((a^b)&e)^b);
		e = (e<<30)|(e>>2);

		x = wp[0] ^ wp[-5] ^ wp[-11] ^ wp[-13];
		wp[3] = (x<<1) | (x>>31);
		b += ((c<<5) | (c>>27)) + wp[3];
		b += 0x5a827999 + (((e^a)&d)^a);
		d = (d<<30)|(d>>2);

		x = wp[1] ^ wp[-4] ^ wp[-10] ^ wp[-12];
		wp[4] = (x<<1) | (x>>31);
		a += ((b<<5) | (b>>27)) + wp[4];
		a += 0x5a827999 + (((d^e)&c)^e);
		c = (c<<30)|(c>>2);

		wp += 5;
		p += 4;

		wend = w + 40;
		for(; wp < wend; wp += 5){
			x = wp[-3] ^ wp[-8] ^ wp[-14] ^ wp[-16];
			wp[0] = (x<<1) | (x>>31);
			e += ((a<<5) | (a>>27)) + wp[0];
			e += 0x6ed9eba1 + (b^c^d);
			b = (b<<30)|(b>>2);

			x = wp[-2] ^ wp[-7] ^ wp[-13] ^ wp[-15];
			wp[1] = (x<<1) | (x>>31);
			d += ((e<<5) | (e>>27)) + wp[1];
			d += 0x6ed9eba1 + (a^b^c);
			a = (a<<30)|(a>>2);

			x = wp[-1] ^ wp[-6] ^ wp[-12] ^ wp[-14];
			wp[2] = (x<<1) | (x>>31);
			c += ((d<<5) | (d>>27)) + wp[2];
			c += 0x6ed9eba1 + (e^a^b);
			e = (e<<30)|(e>>2);

			x = wp[0] ^ wp[-5] ^ wp[-11] ^ wp[-13];
			wp[3] = (x<<1) | (x>>31);
			b += ((c<<5) | (c>>27)) + wp[3];
			b += 0x6ed9eba1 + (d^e^a);
			d = (d<<30)|(d>>2);

			x = wp[1] ^ wp[-4] ^ wp[-10] ^ wp[-12];
			wp[4] = (x<<1) | (x>>31);
			a += ((b<<5) | (b>>27)) + wp[4];
			a += 0x6ed9eba1 + (c^d^e);
			c = (c<<30)|(c>>2);
		}

		wend = w + 60;
		for(; wp < wend; wp += 5){
			x = wp[-3] ^ wp[-8] ^ wp[-14] ^ wp[-16];
			wp[0] = (x<<1) | (x>>31);
			e += ((a<<5) | (a>>27)) + wp[0];
			e += 0x8f1bbcdc + ((b&c)|((b|c)&d));
			b = (b<<30)|(b>>2);

			x = wp[-2] ^ wp[-7] ^ wp[-13] ^ wp[-15];
			wp[1] = (x<<1) | (x>>31);
			d += ((e<<5) | (e>>27)) + wp[1];
			d += 0x8f1bbcdc + ((a&b)|((a|b)&c));
			a = (a<<30)|(a>>2);

			x = wp[-1] ^ wp[-6] ^ wp[-12] ^ wp[-14];
			wp[2] = (x<<1) | (x>>31);
			c += ((d<<5) | (d>>27)) + wp[2];
			c += 0x8f1bbcdc + ((e&a)|((e|a)&b));
			e = (e<<30)|(e>>2);

			x = wp[0] ^ wp[-5] ^ wp[-11] ^ wp[-13];
			wp[3] = (x<<1) | (x>>31);
			b += ((c<<5) | (c>>27)) + wp[3];
			b += 0x8f1bbcdc + ((d&e)|((d|e)&a));
			d = (d<<30)|(d>>2);

			x = wp[1] ^ wp[-4] ^ wp[-10] ^ wp[-12];
			wp[4] = (x<<1) | (x>>31);
			a += ((b<<5) | (b>>27)) + wp[4];
			a += 0x8f1bbcdc + ((c&d)|((c|d)&e));
			c = (c<<30)|(c>>2);
		}

		wend = w + 80;
		for(; wp < wend; wp += 5){
			x = wp[-3] ^ wp[-8] ^ wp[-14] ^ wp[-16];
			wp[0] = (x<<1) | (x>>31);
			e += ((a<<5) | (a>>27)) + wp[0];
			e += 0xca62c1d6 + (b^c^d);
			b = (b<<30)|(b>>2);

			x = wp[-2] ^ wp[-7] ^ wp[-13] ^ wp[-15];
			wp[1] = (x<<1) | (x>>31);
			d += ((e<<5) | (e>>27)) + wp[1];
			d += 0xca62c1d6 + (a^b^c);
			a = (a<<30)|(a>>2);

			x = wp[-1] ^ wp[-6] ^ wp[-12] ^ wp[-14];
			wp[2] = (x<<1) | (x>>31);
			c += ((d<<5) | (d>>27)) + wp[2];
			c += 0xca62c1d6 + (e^a^b);
			e = (e<<30)|(e>>2);

			x = wp[0] ^ wp[-5] ^ wp[-11] ^ wp[-13];
			wp[3] = (x<<1) | (x>>31);
			b += ((c<<5) | (c>>27)) + wp[3];
			b += 0xca62c1d6 + (d^e^a);
			d = (d<<30)|(d>>2);

			x = wp[1] ^ wp[-4] ^ wp[-10] ^ wp[-12];
			wp[4] = (x<<1) | (x>>31);
			a += ((b<<5) | (b>>27)) + wp[4];
			a += 0xca62c1d6 + (c^d^e);
			c = (c<<30)|(c>>2);
		}

		/* save state */
		s[0] += a;
		s[1] += b;
		s[2] += c;
		s[3] += d;
		s[4] += e;
	}
}
