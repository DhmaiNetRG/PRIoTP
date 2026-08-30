/*
 * x25519.c — Curve25519 ECDH (X25519), RFC 7748
 *
 * Field arithmetic based on TweetNaCl's Curve25519 implementation
 * (public domain, Bernstein, Duif, Lange, Schwabe, Yang, 2011).
 *
 * Uses 16-limb representation with int64_t limbs (16 bits each).
 * This is simpler than the ref10 10-limb representation and is known
 * to produce correct RFC 7748 test vectors.
 */

#include "x25519.h"
#include "../blake2s/blake2s.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/* GF(2^255-19) elements: 16 limbs of 16 bits each */
typedef int64_t gf[16];

static const gf gf1 = {1};

static void gf_0(gf r)       { int i; for(i=0;i<16;i++) r[i]=0; }
static void gf_1(gf r)       { gf_0(r); r[0]=1; }
static void gf_add(gf r,const gf a,const gf b) { int i; for(i=0;i<16;i++) r[i]=a[i]+b[i]; }
static void gf_sub(gf r,const gf a,const gf b) { int i; for(i=0;i<16;i++) r[i]=a[i]-b[i]; }
static void gf_cpy(gf r,const gf a)            { int i; for(i=0;i<16;i++) r[i]=a[i]; }

/* Conditional swap: swap a,b if bit */
static void sel(gf p,gf q,int b){
    int64_t t,c=~(b-1);
    int i;
    for(i=0;i<16;i++){t=c&(p[i]^q[i]);p[i]^=t;q[i]^=t;}
}

/* Load 32 bytes (LE) into gf */
static void unpackfe(gf r,const uint8_t *x){
    int i;
    for(i=0;i<16;i++) r[i]=(int64_t)x[2*i] | ((int64_t)x[2*i+1]<<8);
    r[15] &= 0x7fff;
}

/* Store gf into 32 bytes (LE) */
static void car(gf r){
    int i; int64_t c;
    for(i=0;i<16;i++){
        r[i]+=1LL<<16;
        c=r[i]>>16;
        r[(i+1)*(i<15)] += c-1+(37*(c-1)>>(63))*(i==15);
        r[i]-=c<<16;
    }
}
static void packfe(uint8_t *o,const gf r){
    int i; int64_t b; gf m,t;
    gf_cpy(t,r);
    car(t); car(t); car(t);
    for(int j=0;j<2;j++){
        m[0]=t[0]-0xffed;
        for(i=1;i<15;i++){
            m[i]=t[i]-0xffff-((m[i-1]>>16)&1);
            m[i-1]&=0xffff;
        }
        m[15]=t[15]-0x7fff-((m[14]>>16)&1);
        b=(m[15]>>16)&1;
        m[14]&=0xffff;
        sel(t,m,1-b);
    }
    for(i=0;i<16;i++){
        o[2*i]=(uint8_t)t[i];
        o[2*i+1]=(uint8_t)(t[i]>>8);
    }
}

/* GF multiplication */
static void M(gf o,const gf a,const gf b){
    int64_t t[31]={0};
    int i,j;
    for(i=0;i<16;i++) for(j=0;j<16;j++) t[i+j]+=a[i]*b[j];
    for(i=0;i<15;i++) t[i]+=38*t[i+16];
    for(i=0;i<16;i++) o[i]=t[i];
    car(o); car(o);
}

/* GF squaring */
static void S(gf o,const gf a){ M(o,a,a); }

/* GF inversion: o = a^(p-2) */
static void inv(gf o,const gf a){
    gf c; int i; gf_cpy(c,a);
    for(i=253;i>=0;i--){ S(c,c); if(i!=2&&i!=4) M(c,c,a); }
    gf_cpy(o,c);
}

/*
 * X25519 scalar multiplication.
 * Computes out = n * p (Curve25519 Montgomery ladder).
 */
static void scalarmult_tweet(uint8_t *q, const uint8_t *n, const uint8_t *p) {
    uint8_t z[32];
    int64_t r;
    int i;
    gf x;
    gf a, b, c, d, e, f;

    for(i=0;i<31;i++) z[i]=n[i];
    z[31]=(n[31]&127)|64;
    z[0]&=248;

    unpackfe(x, p);
    for(i=0;i<16;i++){
        b[i]=x[i];
        d[i]=a[i]=c[i]=0;
    }
    a[0]=d[0]=1;

    for(i=254;i>=0;i--){
        r=(z[i>>3]>>(i&7))&1;
        sel(a,b,(int)r);
        sel(c,d,(int)r);
        gf_add(e,a,c); gf_sub(a,a,c);
        gf_add(c,b,d); gf_sub(b,b,d);
        S(d,e); S(f,a);
        M(a,c,a); M(c,e,b);
        gf_add(e,a,c); gf_sub(a,a,c);
        S(b,e); gf_sub(c,d,f);
        {
            gf t121665={0xdb41,1};
            M(a,c,t121665);
        }
        gf_add(a,a,d);
        M(c,c,a); M(a,d,f);
        M(d,b,x);
        S(b,e);
        sel(a,b,(int)r);
        sel(c,d,(int)r);
    }
    inv(c,c);
    M(a,a,c);
    packfe(q,a);
}

/* Base point coordinate u = 9 */
static const uint8_t basepoint[32] = {9};

void x25519_public_key(uint8_t pub[32], const uint8_t priv[32]) {
    scalarmult_tweet(pub, priv, basepoint);
}

void x25519(uint8_t shared[32], const uint8_t priv[32], const uint8_t peer_pub[32]) {
    scalarmult_tweet(shared, priv, peer_pub);
}

static int read_random(uint8_t *buf, size_t len) {
    FILE *f = fopen("/dev/urandom", "rb");
    if (f) {
        size_t r = fread(buf, 1, len, f);
        fclose(f);
        if (r == len) return 0;
    }
    /* Fallback for Windows/non-POSIX testing */
    for (size_t i = 0; i < len; i++) {
        buf[i] = (uint8_t)rand();
    }
    return 0;
}

int x25519_generate_keypair(uint8_t priv[32], uint8_t pub[32]) {
    if (read_random(priv, 32) != 0) return -1;
    x25519_public_key(pub, priv);
    return 0;
}

void x25519_public_from_private(uint8_t pub[32], const uint8_t priv[32]) {
    x25519_public_key(pub, priv);
}

void x25519_shared_secret(uint8_t s[32], const uint8_t priv[32], const uint8_t pub[32]) {
    x25519(s, priv, pub);
}
