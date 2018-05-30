// prog.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <time.h>

#include "md5.h"
#include "sf.h"
#include "ui.h"


class CProg {
public:
    HANDLE              thd;
	DWORD				tid;

	uiWindow            *win;
    uiMultilineEntry    *entry;

	bool				devflag;
	bool				logflag=true;
};



static CProg mProg;

#define BASE_MSG (WM_USER+100)
enum {
	START_MSG = BASE_MSG,
	STOP_MSG,
	MAKE_MSG,

};


////////////////////////////////////////////////////
static void log(const char *fmt, ...)
{
	int i;
	va_list args;
	char buffer[1000];

	va_start(args, fmt);
	i = vsprintf(buffer, fmt, args);
	va_end(args);
	if (mProg.entry && mProg.logflag) {
		uiMultilineEntryAppend(mProg.entry, buffer);
	}
}

static void log_clr()
{
	if (mProg.entry) {
		uiMultilineEntrySetText(mProg.entry, "");
	}
}

static void log_en(bool flag)
{
	mProg.logflag = flag;
}

////////////////////////////////////////////////////

enum {
	BIN_UBOOT = 0,
	BIN_ART,
	BIN_SYSTEM,
	BIN_NUM
};

enum {
	FLAG_WEB = 0,
	FLAG_SERVER,
	FLAG_BOTH,
	FLAG_SYSTEM,
	FLAG_UBOOT,
	FLAG_ART,
};

typedef struct {
	int			min;
	int			max;
	uint32_t	addr;
	int         flag;
	char        *name;
}binfo_t;


typedef struct {
	uint16_t     chipID;
	uint16_t     revsion;
	uint8_t      macw[6];

	uint8_t      rsv0[30];
	uint8_t      mac0[6];
	uint8_t      mac1[6];

	uint16_t     cfg0;
	uint16_t     cfg1;

	uint8_t      rsv1;
	uint8_t      country;
	uint8_t      rsv2;
	uint8_t      led_mode;
	uint8_t      rsv3[6];

	uint16_t    cfg2;
	uint8_t     lna_gain;
	uint8_t     rsv4;

	uint16_t    rssi;
	uint8_t     rsv5[8];
	uint8_t     txpwr_delta;
	uint8_t     rsv6[4];
	uint8_t     tmp_sen_cal;

	uint8_t     tx0_pa_lsb;
	uint8_t     tx0_pa_msb;

	uint8_t     tx0_pwr;		//0x58
	uint8_t     tx0_pwr_l;
	uint8_t     tx0_pwr_m;
	uint8_t     tx0_pwr_h;

	uint8_t     rsv7[152];
	uint8_t		xtal_call;
}art_t;



#define KB 1024
#define MB (1024*1024)
static binfo_t binInfos[BIN_NUM] = {
	{100*KB,		192*KB,		0x00000,		FLAG_UBOOT,		"uboot" },		//uboot
	{60*KB,			64*KB,		0x40000,		FLAG_ART,		"art"   },		//art
	{4*MB,			15*MB,		0x50000,		FLAG_SYSTEM,	"system"},		//system
};

#define UPG_FILE                    "../bin/KOBBLE"
#define ART_FILE					"../bin/ART"
#define NART_FILE					"../bin/art.bin"

#define FLAG_NAME                   "FLAG="
#define DEFAULT_NAME                "DEFAULT="
#define MAGIC_NAME                  "@@@KOBBLE@@@\n"

#define HEAD_LEN                    (10*KB)

#define CHECK(len,flag,bi)	((len>=bi.min && len<=bi.max) && (flag==bi.flag))


static int str_match(char *buf, int blen, char *str, int slen)
{
    int i;
    char *p;
    int offset=-1;
    
    for (i=0; i<blen-slen; i++) {
        p = buf+i;

        if (memcmp(p, str, slen)==0) {
            offset = i;
            break;
        }
    }
    
    return offset;
}


static int is_space(char x)  
{  
    if(x==' '||x=='\t'||x=='\n'||x=='\f'||x=='\b'||x=='\r')  
        return 1;  
    else    
        return 0;  
}
static int is_digit(char x)  
{  
    if(x<='9'&&x>='0')           
        return 1;   
    else   
        return 0;  
  
}
static int str2int(char *ptr)  
{  
    char c;              /* current char */  
    int  total;         /* current total */  
    char sign;           /* if '-', then negative, otherwise positive */  

    /* skip whitespace */  
    while (is_space(*ptr) )  
        ++ptr;  

    c = *ptr++;  
    sign = c;           /* save sign indication */  
    if (c == '-' || c == '+')  
        c = *ptr++;    /* skip sign */  

    total = 0;  

    while (is_digit(c)) {  
        total = 10 * total + (c - '0');     /* accumulate digit */  
        c = *ptr++;    /* get next char */  
    }  

    if (sign == '-')  
        return -total;  
    else  
        return total;   /* return result, negated if necessary */  
}


static int get_token_offset(char *p, int len, char *token)
{
    int l;
    int offset;
    
    l = (int)strlen(token);
    offset = str_match(p, len, token, l);
    if (offset<0) {
        log("____ not found %s\n", token);
        return -1;
    }
    
    return (offset+l);
}


static int get_bin_offset(char *p, int *offset)
{
    int x;
    
    x = get_token_offset(p, HEAD_LEN, MAGIC_NAME);
    if (x<0) {
        return -1;
    }
    
    if (offset) {
        *offset = x;
    }
    
    return 0;
}

static int get_flag(char *p, int *flag)
{
    int i=0;
    int offset;
    char *ptr;
    char tmp[20];    
    
    offset = get_token_offset(p, HEAD_LEN, FLAG_NAME);
    if (offset<0) {
        return -1;
    }
    
    ptr = p+offset;
    while(ptr[i]>0x20 && ptr[i]<=0x7e) i++;
    if (i==0 || i>4) {
        return -1;
    }
    
    memcpy(tmp, ptr, i);
    tmp[i] = 0;

    if (flag) {
        *flag = str2int(tmp);
    }
    
    return 0;
}

static int get_default(char *p, int *def)
{
    int i=0;
    int offset;
    char *ptr;
    char tmp[20];    
    
    if (!p || !def) {
        return -1;
    }
    
    offset = get_token_offset(p, HEAD_LEN, DEFAULT_NAME);
    if (offset<0) {
        return -1;
    }
    
    ptr = p+offset;
    while(ptr[i]>0x20 && ptr[i]<=0x7e) i++;
    if (i==0 || i>4) {
        return -1;
    }
    
    memcpy(tmp, ptr, i);
    tmp[i] = 0;

    *def = str2int(tmp);
    
    return 0;
}

static int get_md5(char *p, int len, char *md5)
{
    memcpy(md5, p+len-32, 32);
    md5[32] = 0;
    
    return 0;
}


#define get_chr(b) (b<=9)?(b+0x30):(b-10+0x61);
static void byte2char(char *chr, uint8_t b)
{
	char h, l;

	l = b & 0x0f;
	h = (b >> 4) & 0x0f;

	chr[0] = get_chr(h);
	chr[1] = get_chr(l);
}
static int calc_md5(char *p, int len, char *md5)
{
	int i;
	uint8_t tmp[16];
	MD5_CTX ctx;

	MD5_Init(&ctx);
	MD5_Update(&ctx, p, len);
	MD5_Final(tmp, &ctx);

	for (i = 0; i<16; i++) {
		byte2char(md5 + i * 2, tmp[i]);
		//log("%02X, ", tmp[i]);
	}
	md5[32] = 0;
	//log("\n");

	return 0;
}


static int md5_check(char *p, int len)
{
	int  r;
	char m1[40];
	char m2[40];

	r = get_md5(p, len, m1);
	if (r != 0) {
		log("___get_md5 failed\n");
		return -1;
	}

	calc_md5(p, len-32, m2);
	r = strcmp(m1, m2);
	log("___md1: %s\n", m1);
	log("___md2: %s\n", m2);
	log("___cmp: %d\n", r);

	return r;
}


static bool file_read(char *path, char **buf, int *len)
{
	FILE *file;
	bool r=true;
	char *buffer;
	int flen, rlen;

	file = fopen(path, "rb");
	if (!file) {
		log("____ %s open failed\n", path);
		r = false;
		goto exit;
	}

	fseek(file, 0, SEEK_END);
	flen = ftell(file);
	if (flen <= 0) {
		log("____ %s length error\n", path);
		r = false;
		goto exit;
	}

	//log("_____%s length: %d\n", path, flen);
	buffer = (char *)malloc(flen);
	if (!buffer) {
		log("____ buffer malloc failed\n");
		r = false;
		goto exit;
	}

	fseek(file, 0, SEEK_SET);
	rlen = (int)fread(buffer, 1, flen, file);
	if (rlen != flen)
	{
		log("____ %s fread failed, flen: %d, rlen: %d\n", path, flen, rlen);
		r = false;
		goto exit;
	}

	if (buf && len) {
		*buf = buffer;
		*len = flen;
	}

exit:
	fclose(file);

	return r;
}

static bool file_write(char *path, char *buf, int len)
{
	bool r = true;
	FILE *file;
	int wlen;

	if (!path || !buf || !len) {
		return false;
	}

	file = fopen(path, "wb");
	if (!file) {
		log("____ uboot open failed\n");
		return false;
	}

	wlen = (int)fwrite((void*)buf, 1, len, file);
	if (wlen != len) {
		r = false;
	}

	fclose(file);

	return r;
}

static bool open_dev()
{
	if (!mProg.devflag) {
		mProg.devflag = sf_init();
		if (mProg.devflag) {
			log("device open ok!\n");
		}
		else {
			log("device open failed!\n");
			log("please check the device and the connection, then try again.\n");
		}
	}

	return mProg.devflag;
}


static bool prog_bin()
{
    bool b;
	int i,r;
    int inx=-1;
    int offset;
    int blen,len;
    int flag;
    char *bin;
    char *buf;
	char *name;
	int dflt=0;
    uint32_t addr;
    
	if (!open_dev()) {
		uiMsgBoxError(mProg.win, "Device Error", "Device Not Opened\n");
		return false;
	}

    b = file_read(UPG_FILE, &buf, &len);
	if (!b) {
		log("file read failed");
		return false;
	} 
    
    r = get_bin_offset(buf, &offset);
    if (r!=0) {
        log("____ magic not found\n");
        return false;
    }
	log("____offset: %d\n", offset);

    r = get_flag(buf, &flag);
    if (r<0) {
        log("____ flag not found\n");
        return false;
    }
    
    blen = len-offset;
    bin  = buf+offset;

	r = md5_check(buf, len);
	if (r != 0) {
		log("____ md5 check failed\n");
		return false;
	}

    for (i=0; i<BIN_NUM; i++) {
		if (CHECK(blen, flag, binInfos[i])) {
			inx = i;
			break;
		}
	}
	if (inx < 0) {
		log("____ bin invalid\n");
		return false;
	}

    addr = binInfos[inx].addr;
	name = binInfos[inx].name;

    get_default(buf, &dflt);
    if (dflt>0 &&  && flag==FLAG_SYSTEM) {
        size_t size = sf_capacity();
        log("___ capacity: %d, erase to default ...\n", size);
        sf_unprotect();
        sf_erase(addr+blen, size-addr-blen-1);
    }

	log("____prog %s: addr: 0x%x, len: %d\n", name, addr, blen);
	b = sf_write(addr, (uint8_t *)bin, blen, false);
	free(buf);

	return b;
}

static uint32_t mac_cnt = 54;
static void make_mac(uint8_t *mac, uint32_t cnt)
{
	int i;
	uint8_t *p = (uint8_t *)(&cnt);

	mac[0] = 0x0c;
	mac[1] = 0xef;

	for (i = 0; i<4; i++) {
		mac[5-i] = p[i];
	}
}


static void print_art(art_t *art)
{
	if (art) {
		log("_____ chipID:      0x%04x\n", art->chipID);
		log("_____ macw:        %02X:%02X:%02X:%02X:%02X:%02X\n", art->macw[0], art->macw[1], art->macw[2], art->macw[3], art->macw[4], art->macw[5]);
		log("_____ mac0:        %02X:%02X:%02X:%02X:%02X:%02X\n", art->mac0[0], art->mac0[1], art->mac0[2], art->mac0[3], art->mac0[4], art->mac0[5]);
		log("_____ mac1:        %02X:%02X:%02X:%02X:%02X:%02X\n", art->mac1[0], art->mac1[1], art->mac1[2], art->mac1[3], art->mac1[4], art->mac1[5]);
		log("_____ tx0_pwr:     0x%02x, offset: 0x%x\n", art->tx0_pwr, &art->tx0_pwr - (uint8_t *)art);
		log("_____ tx0_pwr_l:   0x%02x\n", art->tx0_pwr_l);
		log("_____ tx0_pwr_m:   0x%02x\n", art->tx0_pwr_m);
		log("_____ tx0_pwr_h:   0x%02x\n", art->tx0_pwr_h);
		log("_____ xtal_call:   0x%02x\n", art->xtal_call);
	}
}

static bool make_art()
{
	bool r;
	char *buf;
	int len;
	art_t *art;

	r = file_read(ART_FILE, &buf, &len);
	if (!r) {
		log("file read failed");
		return false;
	}

	art = (art_t *)buf;
	make_mac(art->macw, mac_cnt);
	make_mac(art->mac0, mac_cnt++);
	make_mac(art->mac1, mac_cnt);

	print_art(art);

#if 0
	char tmp[20];
	sprintf(tmp, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],mac[1], mac[2], mac[3], mac[4], mac[5]);
	log("____ mac: %s\n", tmp);
#endif

	file_write(NART_FILE, buf, len);
	free(buf);

	return true;
}


///////////////////////////////////////////////////

static int onClosing(uiWindow *w, void *data)
{
	uiQuit();
	return 1;
}

static int onShouldQuit(void *data)
{
	uiControlDestroy(uiControl(mProg.win));
	return 1;
}

static void onOpenClicked(uiButton *b, void *data)
{
	uiEntry *entry = uiEntry(data);
	char *filename;

	filename = uiOpenFile(mProg.win);
	if (filename == NULL) {
		uiEntrySetText(entry, "");
		return;
	}
	uiEntrySetText(entry, filename);
	uiFreeText(filename);
}


static void onStartClicked(uiButton *b, void *data)
{
	uiEntry *entry = uiEntry(data);

	PostThreadMessage(mProg.tid, START_MSG, NULL, NULL);
}

static void onMakeClicked(uiButton *b, void *data)
{
	PostThreadMessage(mProg.tid, MAKE_MSG, NULL, NULL);
}


static void onSaveClicked(uiButton *b, void *data)
{
	uiEntry *entry = uiEntry(data);
	char *filename;

	filename = uiSaveFile(mProg.win);
	if (filename == NULL) {
		uiEntrySetText(entry, "");
		return;
	}
	uiEntrySetText(entry, filename);
	uiFreeText(filename);
}


static uiBox *boxInit(char *name, bool hv, bool seperater)
{
	uiBox *box;

	if (hv==false) {
		box = uiNewHorizontalBox();
	}
	else {
		box = uiNewVerticalBox();
	}

	uiBoxSetPadded(box, 1);
	uiBoxAppend(box, uiControl(uiNewLabel(name)), 0);
	if (seperater) {
		if (hv == false) {
			uiBoxAppend(box, uiControl(uiNewHorizontalSeparator()), 0);
		}
		else {
			uiBoxAppend(box, uiControl(uiNewVerticalSeparator()), 0);
		}
	}

	return box;
}

static void itemAppend(uiBox *box, char *title, bool seperater)
{
	uiBox    *h;
	uiEntry  *entry;
	uiButton *button;
    
    h = uiNewHorizontalBox();
	uiBoxSetPadded(h, 1);
    
    uiBoxAppend(h, uiControl(uiNewLabel(title)), 0);

    button = uiNewButton("Open");
	entry = uiNewEntry();
	uiEntrySetReadOnly(entry, 0);
	uiButtonOnClicked(button, onOpenClicked, entry);

	uiBoxAppend(h, uiControl(button), 0);
	uiBoxAppend(h, uiControl(entry), 0);
    
    uiBoxAppend(h, uiControl(uiNewLabel("addr")), 0);
    uiBoxAppend(h, uiControl(uiNewEntry()), 0);
    //uiEntrySetText(entry, (const char*)tmp);
    
    button = uiNewButton("Start");
    uiButtonOnClicked(button, onStartClicked, entry);
    uiBoxAppend(h, uiControl(button), 0);

	if (seperater) {
		uiBoxAppend(h, uiControl(uiNewHorizontalSeparator()), 0);
	}

	uiBoxAppend(box, uiControl(h), 0);
}


static void labelAppend(uiBox *box, char *name, char *val)
{
	uiBox    *h;
	uiEntry  *entry;
	//uiButton *button;

	h = uiNewHorizontalBox();
	uiBoxAppend(h, uiControl(uiNewLabel(name)), 0);

	entry = uiNewEntry();
	uiBoxAppend(h, uiControl(entry), 0);
	uiEntrySetText(entry, val);

	uiBoxAppend(box, uiControl(h), 0);
}

static uiControl *burnPage(void)
{
	uiBox   *box;

	box = boxInit("BURN ", false, true);
	itemAppend(box, "Bin", false);

	return uiControl(box);
}

static uiControl *artPage(void)
{
	uiBox *box;
	uiButton *button;

	box = boxInit("ART   ", false, true);
	labelAppend(box, "mac", "");

	button = uiNewButton("make");
	uiButtonOnClicked(button, onMakeClicked, box);

	uiBoxAppend(box, uiControl(button), 0);

	return uiControl(box);
}


static DWORD WINAPI threadProc(LPVOID lpParam) 
{
	bool  r;
    MSG   msg;
    
    while(GetMessage(&msg, NULL, 0, 0)) {
    
        switch(msg.message) {
            
            case START_MSG:
			{
				r = prog_bin();
				if (r) {
					log("prog_bin ok!\n");
				}
				else {
					log("prog_bin failed!\n");
				}
            }
            break;
            
            case STOP_MSG:
            {
				
            }
            break;

			case MAKE_MSG:
			{
				r = make_art();
			}
			break;
        }
    }
    
    return 0;  
}


static void ui_init()
{
	uiBox *box;
	const char *err;
	uiInitOptions options;
    
    memset(&options, 0, sizeof(options));
	err = uiInit(&options);
	if (err != NULL) {
		uiFreeInitError(err);
		return;
	}

	mProg.win = uiNewWindow("Prog", 640, 480, 1);
	uiWindowOnClosing(mProg.win, onClosing, NULL);
	uiOnShouldQuit(onShouldQuit, mProg.win);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);

	uiWindowSetChild(mProg.win, uiControl(box));
	uiWindowSetMargined(mProg.win, 1);
	
	uiBoxAppend(box, burnPage(), 1);
	uiBoxAppend(box, artPage(), 1);
	
	mProg.entry = uiNewMultilineEntry();
	uiMultilineEntrySetReadOnly(mProg.entry, 1);
	uiBoxAppend(box, uiControl(mProg.entry), 1);

	uiControlShow(uiControl(mProg.win));
	mProg.thd = CreateThread(NULL, 0, threadProc, NULL, 0, &mProg.tid);
}


static void ui_loop()
{
    uiMain();
    sf_free();
}


int main(int argc, char *argv[])
{
	ui_init();
	ui_loop();
    
	return 0;
}





