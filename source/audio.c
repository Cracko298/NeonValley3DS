#include "audio.h"
#include <string.h>

extern const u8 music_bcwav[];
extern const u8 music_bcwav_end[];

typedef struct {
    bool ndspReady;
    bool ready;
    u8 *samples;
    u32 sampleBytes;
    u32 sampleCount;
    u32 sampleRate;
    u16 coefs[16];
    ndspAdpcmData context;
    ndspWaveBuf wave;
    int volume;
} AudioState;

static AudioState a;

static u16 rd16(const u8 *p) {
    return (u16)p[0] | ((u16)p[1] << 8);
}

static u32 rd32(const u8 *p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static bool range_ok(size_t size,size_t off,size_t len) {
    return off <= size && len <= size - off;
}

static void apply_volume(void) {
    if (!a.ndspReady) return;
    float v=(float)a.volume*0.25f;
    float mix[12]={0};
    mix[0]=0.72f*v;
    mix[1]=0.72f*v;
    mix[2]=0.18f*v;
    mix[3]=0.18f*v;
    ndspChnSetMix(0,mix);
}

static bool parse_music(const u8 *buf,size_t size,const u8 **samplePtr) {
    if (size<0x40 || memcmp(buf,"CWAV",4)!=0) return false;
    if (rd16(buf+4)!=0xFEFF || rd32(buf+8)!=0x02010000) return false;
    if (rd32(buf+0x0C)>size) return false;

    u32 infoOff=rd32(buf+0x18);
    u32 dataOff=rd32(buf+0x24);
    if (!range_ok(size,infoOff,0x28) || !range_ok(size,dataOff,8)) return false;
    if (memcmp(buf+infoOff,"INFO",4)!=0 || memcmp(buf+dataOff,"DATA",4)!=0) return false;

    const u8 *info=buf+infoOff;
    if (info[8]!=2 || rd32(info+0x1C)!=1) return false;

    a.sampleRate=rd32(info+0x0C);
    a.sampleCount=rd32(info+0x14);
    if (!a.sampleRate || !a.sampleCount) return false;

    const u8 *tableCount=info+0x1C;
    const u8 *chanRef=info+0x20;
    if (rd16(chanRef)!=0x7100) return false;

    u32 chanRel=rd32(chanRef+4);
    size_t chanOff=(size_t)(tableCount-buf)+chanRel;
    if (!range_ok(size,chanOff,0x14)) return false;
    const u8 *chan=buf+chanOff;
    if (rd16(chan)!=0x1F00 || rd16(chan+8)!=0x0300) return false;

    u32 sampleRel=rd32(chan+4);
    u32 adpcmRel=rd32(chan+0x0C);
    size_t adpcmOff=chanOff+adpcmRel;
    size_t sampleOff=(size_t)dataOff+8u+sampleRel;
    if (!range_ok(size,adpcmOff,0x2E)) return false;

    a.sampleBytes=((a.sampleCount+13u)/14u)*8u;
    if (!range_ok(size,sampleOff,a.sampleBytes)) return false;

    const u8 *adpcm=buf+adpcmOff;
    for (int i=0;i<16;++i) a.coefs[i]=rd16(adpcm+i*2);
    memcpy(&a.context,adpcm+0x20,sizeof(a.context));
    *samplePtr=buf+sampleOff;
    return true;
}

static void queue_music(void) {
    if (!a.ready) return;
    ndspChnWaveBufClear(0);
    ndspChnSetInterp(0,NDSP_INTERP_LINEAR);
    ndspChnSetRate(0,(float)a.sampleRate);
    ndspChnSetFormat(0,NDSP_FORMAT_MONO_ADPCM);
    ndspChnSetAdpcmCoefs(0,a.coefs);
    apply_volume();

    memset(&a.wave,0,sizeof(a.wave));
    a.wave.data_adpcm=a.samples;
    a.wave.nsamples=a.sampleCount;
    a.wave.adpcm_data=&a.context;
    a.wave.looping=true;
    ndspChnWaveBufAdd(0,&a.wave);
}

bool audio_init(void) {
    memset(&a,0,sizeof(a));
    a.volume=4;

    const u8 *samplePtr=NULL;
    size_t size=(size_t)((uintptr_t)music_bcwav_end-(uintptr_t)music_bcwav);
    if (!parse_music(music_bcwav,size,&samplePtr)) return false;

    a.samples=(u8*)linearAlloc(a.sampleBytes);
    if (!a.samples) return false;
    memcpy(a.samples,samplePtr,a.sampleBytes);
    DSP_FlushDataCache(a.samples,a.sampleBytes);

    if (R_FAILED(ndspInit())) {
        linearFree(a.samples);
        a.samples=NULL;
        return false;
    }

    a.ndspReady=true;
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    a.ready=true;
    queue_music();
    return true;
}

void audio_exit(void) {
    if (a.ndspReady) {
        ndspChnWaveBufClear(0);
        ndspExit();
    }
    if (a.samples) linearFree(a.samples);
    memset(&a,0,sizeof(a));
}

void audio_update(void) {
    if (!a.ready) return;
    if (a.wave.status==NDSP_WBUF_DONE) queue_music();
}

void audio_set_volume(int level) {
    if (level<0) level=0;
    if (level>4) level=4;
    a.volume=level;
    apply_volume();
}

int audio_get_volume(void) {
    return a.volume;
}

bool audio_is_ready(void) {
    return a.ready;
}
