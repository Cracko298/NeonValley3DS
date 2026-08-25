#include "framebuffer.h"
#include <3ds.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "math2d.h"

static u8 *fb;
static u16 fbW,fbH;
static const u8 digits[10][5]={{7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},{7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7}};

void fb_begin(void) { fb=gfxGetFramebuffer(GFX_TOP,GFX_LEFT,&fbW,&fbH); }

void fb_pixel(int x,int y,Color c) {
    if ((unsigned)x>=TOP_W||(unsigned)y>=TOP_H) return;
    int o=(x*TOP_H+(TOP_H-1-y))*3;
    fb[o]=c.b; fb[o+1]=c.g; fb[o+2]=c.r;
}

void fb_add_pixel(int x,int y,Color c) {
    if ((unsigned)x>=TOP_W||(unsigned)y>=TOP_H) return;
    int o=(x*TOP_H+(TOP_H-1-y))*3;
    int b=fb[o]+c.b,g=fb[o+1]+c.g,r=fb[o+2]+c.r;
    fb[o]=(u8)(b>255?255:b); fb[o+1]=(u8)(g>255?255:g); fb[o+2]=(u8)(r>255?255:r);
}

Color color_scale(Color c,float s) {
    Color o={(u8)clampf(c.r*s,0,255),(u8)clampf(c.g*s,0,255),(u8)clampf(c.b*s,0,255)};
    return o;
}

void fb_clear(Color c) {
    if (!c.r&&!c.g&&!c.b) { memset(fb,0,TOP_W*TOP_H*3); return; }
    for (int x=0;x<TOP_W;++x) for (int y=0;y<TOP_H;++y) fb_pixel(x,y,c);
}

void fb_disc(int cx,int cy,int r,Color c) {
    int rr=r*r;
    for (int y=-r;y<=r;++y) for (int x=-r;x<=r;++x) if (x*x+y*y<=rr) fb_pixel(cx+x,cy+y,c);
}

void fb_line(int x0,int y0,int x1,int y1,int radius,Color c) {
    int dx=abs(x1-x0),sx=x0<x1?1:-1;
    int dy=-abs(y1-y0),sy=y0<y1?1:-1;
    int err=dx+dy;
    for (;;) {
        if (radius<=0) fb_pixel(x0,y0,c); else fb_disc(x0,y0,radius,c);
        if (x0==x1&&y0==y1) break;
        int e2=2*err;
        if (e2>=dy) { err+=dy; x0+=sx; }
        if (e2<=dx) { err+=dx; y0+=sy; }
    }
}

void fb_ring(int cx,int cy,int r,Color c) {
    int lx=cx+r,ly=cy;
    for (int i=1;i<=28;++i) {
        float a=2.0f*PI*(float)i/28.0f;
        int x=cx+(int)(cosf(a)*r),y=cy+(int)(sinf(a)*r);
        fb_line(lx,ly,x,y,0,c); lx=x; ly=y;
    }
}

static int outcode(int x,int y) {
    int c=0;
    if (x<-28) c|=1; else if (x>TOP_W+27) c|=2;
    if (y<-28) c|=4; else if (y>TOP_H+27) c|=8;
    return c;
}

bool fb_clip_line(int *x0,int *y0,int *x1,int *y1) {
    int c0=outcode(*x0,*y0),c1=outcode(*x1,*y1);
    for (;;) {
        if (!(c0|c1)) return true;
        if (c0&c1) return false;
        int c=c0?c0:c1,x=0,y=0;
        if (c&8) { y=TOP_H+27; if (*y1==*y0) return false; x=*x0+(*x1-*x0)*(y-*y0)/(*y1-*y0); }
        else if (c&4) { y=-28; if (*y1==*y0) return false; x=*x0+(*x1-*x0)*(y-*y0)/(*y1-*y0); }
        else if (c&2) { x=TOP_W+27; if (*x1==*x0) return false; y=*y0+(*y1-*y0)*(x-*x0)/(*x1-*x0); }
        else { x=-28; if (*x1==*x0) return false; y=*y0+(*y1-*y0)*(x-*x0)/(*x1-*x0); }
        if (c==c0) { *x0=x; *y0=y; c0=outcode(*x0,*y0); }
        else { *x1=x; *y1=y; c1=outcode(*x1,*y1); }
    }
}

void fb_diamond(int cx,int cy,int r,Color c) {
    for (int y=-r;y<=r;++y) { int w=r-abs(y); for (int x=-w;x<=w;++x) fb_pixel(cx+x,cy+y,c); }
}

static void digit(int x,int y,int d,int s,Color c) {
    if (d<0||d>9) return;
    for (int yy=0;yy<5;++yy) for (int xx=0;xx<3;++xx) if (digits[d][yy]&(1u<<(2-xx)))
        for (int py=0;py<s;++py) for (int px=0;px<s;++px) fb_pixel(x+xx*s+px,y+yy*s+py,c);
}

int fb_number_width(int n,int s) {
    int d=1; while (n>=10) { n/=10; ++d; } return d*4*s-s;
}

void fb_number(int x,int y,int n,int s,Color c) {
    if (n<0) n=0;
    int p=1,d=1; while (n/p>=10) { p*=10; ++d; }
    for (int i=0;i<d;++i) { digit(x+i*4*s,y,(n/p)%10,s,c); p/=10; }
}

void fb_triangle_outline(int cx,int cy,int r,float angle,Color c) {
    int x[3],y[3];
    for (int i=0;i<3;++i) { float a=angle+i*(2.0f*PI/3.0f); x[i]=cx+(int)(cosf(a)*r); y[i]=cy+(int)(sinf(a)*r); }
    fb_line(x[0],y[0],x[1],y[1],0,c); fb_line(x[1],y[1],x[2],y[2],0,c); fb_line(x[2],y[2],x[0],y[0],0,c);
}


void fb_polygon_fill(const int *x,const int *y,int count,Color c) {
    if (!x||!y||count<3||count>16) return;
    int miny=y[0],maxy=y[0];
    for (int i=1;i<count;++i) {
        if (y[i]<miny) miny=y[i];
        if (y[i]>maxy) maxy=y[i];
    }
    if (miny<0) miny=0;
    if (maxy>=TOP_H) maxy=TOP_H-1;
    for (int py=miny;py<=maxy;++py) {
        int xs[16],n=0;
        for (int i=0,j=count-1;i<count;j=i++) {
            int y0=y[j],y1=y[i];
            if ((y0<=py&&y1>py)||(y1<=py&&y0>py)) {
                int dy=y1-y0;
                int ix=x[j]+(int)((long long)(py-y0)*(x[i]-x[j])/dy);
                if (n<16) xs[n++]=ix;
            }
        }
        for (int i=1;i<n;++i) {
            int v=xs[i],j=i-1;
            while (j>=0&&xs[j]>v) { xs[j+1]=xs[j]; --j; }
            xs[j+1]=v;
        }
        for (int i=0;i+1<n;i+=2) {
            int x0=xs[i],x1=xs[i+1];
            if (x0<0) x0=0;
            if (x1>=TOP_W) x1=TOP_W-1;
            for (int px=x0;px<=x1;++px) fb_pixel(px,py,c);
        }
    }
}
