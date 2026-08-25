#include "bloom.h"
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "framebuffer.h"

typedef struct { u8 r,g,b; } BloomPixel;
static BloomPixel a[BLOOM_W*BLOOM_H];
static BloomPixel b[BLOOM_W*BLOOM_H];
static int bloomMode;

static void add_cell(int x,int y,Color c,int strength) {
    if ((unsigned)x>=BLOOM_W||(unsigned)y>=BLOOM_H) return;
    BloomPixel *p=&a[y*BLOOM_W+x];
    int r=p->r+(c.r*strength>>8),g=p->g+(c.g*strength>>8),bb=p->b+(c.b*strength>>8);
    p->r=(u8)(r>255?255:r); p->g=(u8)(g>255?255:g); p->b=(u8)(bb>255?255:bb);
}

void bloom_begin(int mode) { bloomMode=mode; memset(a,0,sizeof(a)); }

void bloom_line(int x0,int y0,int x1,int y1,Color c,int strength) {
    if (bloomMode<=0) return;
    x0>>=2; y0>>=2; x1>>=2; y1>>=2;
    int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;
    for (;;) {
        add_cell(x0,y0,c,strength); add_cell(x0-1,y0,c,strength>>2); add_cell(x0+1,y0,c,strength>>2); add_cell(x0,y0-1,c,strength>>2); add_cell(x0,y0+1,c,strength>>2);
        if (x0==x1&&y0==y1) break;
        int e2=2*err; if (e2>=dy) { err+=dy; x0+=sx; } if (e2<=dx) { err+=dx; y0+=sy; }
    }
}

void bloom_disc(int cx,int cy,int radius,Color c,int strength) {
    if (bloomMode<=0) return;
    int bx=cx>>2,by=cy>>2,br=radius/4+1;
    for (int y=-br;y<=br;++y) for (int x=-br;x<=br;++x) {
        int d=x*x+y*y; if (d>br*br) continue;
        int fall=(strength*(br*br-d+1))/(br*br+1); add_cell(bx+x,by+y,c,fall);
    }
}

static void blur_h(const BloomPixel *src,BloomPixel *dst) {
    for (int y=0;y<BLOOM_H;++y) for (int x=0;x<BLOOM_W;++x) {
        int xm2=x>1?x-2:0,xm1=x>0?x-1:0,xp1=x<BLOOM_W-1?x+1:BLOOM_W-1,xp2=x<BLOOM_W-2?x+2:BLOOM_W-1;
        const BloomPixel *p0=&src[y*BLOOM_W+xm2],*p1=&src[y*BLOOM_W+xm1],*p2=&src[y*BLOOM_W+x],*p3=&src[y*BLOOM_W+xp1],*p4=&src[y*BLOOM_W+xp2];
        BloomPixel *o=&dst[y*BLOOM_W+x];
        o->r=(u8)((p0->r+4*p1->r+6*p2->r+4*p3->r+p4->r)>>4); o->g=(u8)((p0->g+4*p1->g+6*p2->g+4*p3->g+p4->g)>>4); o->b=(u8)((p0->b+4*p1->b+6*p2->b+4*p3->b+p4->b)>>4);
    }
}

static void blur_v(const BloomPixel *src,BloomPixel *dst) {
    for (int y=0;y<BLOOM_H;++y) {
        int ym2=y>1?y-2:0,ym1=y>0?y-1:0,yp1=y<BLOOM_H-1?y+1:BLOOM_H-1,yp2=y<BLOOM_H-2?y+2:BLOOM_H-1;
        for (int x=0;x<BLOOM_W;++x) {
            const BloomPixel *p0=&src[ym2*BLOOM_W+x],*p1=&src[ym1*BLOOM_W+x],*p2=&src[y*BLOOM_W+x],*p3=&src[yp1*BLOOM_W+x],*p4=&src[yp2*BLOOM_W+x];
            BloomPixel *o=&dst[y*BLOOM_W+x];
            o->r=(u8)((p0->r+4*p1->r+6*p2->r+4*p3->r+p4->r)>>4); o->g=(u8)((p0->g+4*p1->g+6*p2->g+4*p3->g+p4->g)>>4); o->b=(u8)((p0->b+4*p1->b+6*p2->b+4*p3->b+p4->b)>>4);
        }
    }
}

void bloom_apply(void) {
    if (bloomMode<=0) return;
    blur_h(a,b); blur_v(b,a);
    if (bloomMode>=2) { blur_h(a,b); blur_v(b,a); }
    int intensity=bloomMode>=2?220:158;
    for (int by=0;by<BLOOM_H;++by) for (int bx=0;bx<BLOOM_W;++bx) {
        const BloomPixel *p=&a[by*BLOOM_W+bx]; if (!(p->r|p->g|p->b)) continue;
        Color c={(u8)(p->r*intensity>>8),(u8)(p->g*intensity>>8),(u8)(p->b*intensity>>8)};
        int x0=bx<<2,y0=by<<2; for (int y=0;y<4;++y) for (int x=0;x<4;++x) fb_add_pixel(x0+x,y0+y,c);
    }
}
