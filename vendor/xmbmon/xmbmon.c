/*
 * xmbmon  --- X motherboard monitor
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "mbmon.h"

/* X include files */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#define DEFAULT_METHOD	' '

#define RES_NAME   "XMBmon"

#define DEFAULT_GEOMETRY  "100x140"
#define DEFAULT_FONT      "-adobe-helvetica-bold-r-*-*-10-*-*-*-*-*-*-*"

#define DEFAULT_LBFONT    "-adobe-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*"
#define DEFAULT_LBCOLOR   "black"

#define DEFAULT_COUNT        "4"
#define DEFAULT_CSEC        "20"
#define DEFAULT_WSEC      "1800"
#define DEFAULT_TMIN      "10.0"
#define DEFAULT_TMAX      "50.0"
#define DEFAULT_TMINF     "50.0"	/* for Fahrenheit */
#define DEFAULT_TMAXF     "130.0"	/* for Fahrenheit */
#define DEFAULT_VMIN      "1.80"
#define DEFAULT_VMAX      "2.20"
#define DEFAULT_TICK         "3"

#define DEFAULT_CLTMB     "blue"
#define DEFAULT_CLTCPU     "red"
#define DEFAULT_CLTCS     "cyan"
#define DEFAULT_CLVC     "green"

#define DEFAULT_CMTMB   "\\ MB"
#define DEFAULT_CMTMB1    " MB"
#define DEFAULT_CMTCPU    "CPU"
#define DEFAULT_CMTCS    "chip"
#define DEFAULT_CMVC    "Vc\\ "
#define DEFAULT_CMVC1     "Vc "

#define DEFAULT_FAHRN   "False"
#define DEFAULT_EXTRAT  "2"
#define DEFAULT_SMB     "0"
#define DEFAULT_PROBE   "none"
#define DEFAULT_TYAN    "False"

#define NO_TEMP         "NOTEMP"


/* xmbmon variables */

/* temp1(MotherBoard), temp2(CPU) [,temp3(ChipSet)] and Vcore0 */
#define NUM_DATA 4

int noTemp3_flag = 0;
int num_data = NUM_DATA;
int t_indx[4] = {0, 1, 2, 3};

char access_method = DEFAULT_METHOD;

char *font, *label;

int width, height;

int n_count, interval_sec, w_sec;
int counter = 0;
int w_counter = 0;
int w_range;

unsigned long t_sec, t_msec;

float *fmean;

float tmin, tmax, vmin, vmax;
int n_tick;
float tscl, vscl, sscl, htck;

float cur_val[NUM_DATA];
float iir_reg[NUM_DATA];
float iir_tc = 0.25; /* IIR LPF time constant, 0 < iir_tc < 1.0 */

char *l_color[NUM_DATA];
unsigned long cpix[NUM_DATA], d_cpix[NUM_DATA];

#define C_LBL 10

char ctmin[C_LBL], ctmax[C_LBL];
char cvmin[C_LBL], cvmax[C_LBL];

char *c_rdv[NUM_DATA], *c_rdg[NUM_DATA];
char *c_rdp[NUM_DATA] = {"%4.1f", "%4.1f", "%4.1f", "%4.2f"};

char *lb_font, *lb_color;
int lb_width, x_lb, c_lb = 0;

/*	the following arrays have variable dim.
	depending on "num_data" being 3 or 4.
	other arrays is fixed and used
	by using index array "t_indx[]".	*/

float scale[NUM_DATA];
float orign[NUM_DATA];
int x_rdg[NUM_DATA], x_rdv[NUM_DATA], y_rdg, y_rdv;
int cg_width[NUM_DATA], cv_width[NUM_DATA];
int cg_w, cv_w, c_height, cm_width, cm_height;
unsigned long cpixlb, d_cpixlb;

typedef struct ring_data {	/* def. of ring-buffer */
	float val;
	struct ring_data *next;
} ring_d;

ring_d *rdp[NUM_DATA];
ring_d *rdpw[NUM_DATA];

/* variables for X11 */

Widget wgt;
Display	*disp;
Window win;
GC gct, gclb, gcl[NUM_DATA];
XPoint *points;
XFontStruct *fontstr;
XFontStruct *lb_fontstr;

/* Xt Application Context */

XtAppContext app_con;

/* functions */

void get_args(int argc, char *argv[]);
void usage(void);
void get_res(void);
void init_dt(void);
void getWH(int *width, int *height);
void repaint_proc(void);
void quit_proc(void);
void draw_values(void);
int ColorPix(Display *, char *, unsigned long *);
void alarm_handler(void);

/*---------- definitions of static data for X Toolkits ----------*/

	static String fallback_resources[] = {
		"*translations: #override \
			<Configure>: repaint_proc()\\n\
			<Expose>: repaint_proc()\\n\
			<BtnUp>: repaint_proc()\\n\
			<Key>Q: quit_proc()",
		"*geometry: "   DEFAULT_GEOMETRY,
		"*font: "       DEFAULT_FONT,
		"*count: "      DEFAULT_COUNT,
		"*sec: "        DEFAULT_CSEC,
		"*wsec: "       DEFAULT_WSEC,
		"*tmin: "       DEFAULT_TMIN,
		"*tmax: "       DEFAULT_TMAX,
		"*vmin: "       DEFAULT_VMIN,
		"*vmax: "       DEFAULT_VMAX,
		"*tick: "       DEFAULT_TICK,
		"*cltmb: "      DEFAULT_CLTMB,
		"*cltcpu: "     DEFAULT_CLTCPU,
		"*cltcs: "      DEFAULT_CLTCS,
		"*clvc: "       DEFAULT_CLVC,
		"*cmtmb: "      DEFAULT_CMTMB,
		"*cmtcpu: "     DEFAULT_CMTCPU,
		"*cmtcs: "      DEFAULT_CMTCS,
		"*cmvc: "       DEFAULT_CMVC,
		"*fahrn: "      DEFAULT_FAHRN,
		"*label: "      ,
		"*labelfont: "  DEFAULT_LBFONT,
		"*labelcolor: " DEFAULT_LBCOLOR,
		"*extratemp: " DEFAULT_EXTRAT,
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
		"*smbdev: " DEFAULT_SMB,
#endif
		"*probe: " DEFAULT_PROBE,
		"*TyanTigerMP: " DEFAULT_TYAN,
		NULL,
	};
	static XtActionsRec actions[] = {
		{"repaint_proc", (XtActionProc) repaint_proc},
		{"quit_proc", (XtActionProc) quit_proc},
	};
	static struct _app_res {
		char *font;
		int count;
		int sec;
		int wsec;
		float tmin;
		float tmax;
		float vmin;
		float vmax;
		int tick;
		char *cltmb;
		char *cltcpu;
		char *cltcs;
		char *clvc;
		char *cmtmb;
		char *cmtcpu;
		char *cmtcs;
		char *cmvc;
		Boolean fahrn;
		char *label;
		char *labelfont;
		char *labelcolor;
		char *method;
		int extratemp;
		char *probe;
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
		char *smbdev;
#endif
		Boolean TyanTigerMP;
	} app_resources;
	static XtResource resources[] = {
		{"font", "Font", XtRString, sizeof(String),
			XtOffset(struct _app_res*, font),
			XtRString, (XtPointer) NULL},
		{"count", "Count", XtRInt, sizeof(int),
			XtOffset(struct _app_res*, count),
			XtRImmediate, (XtPointer) NULL},
		{"sec", "Sec", XtRInt, sizeof(int),
			XtOffset(struct _app_res*, sec),
			XtRImmediate, (XtPointer) NULL},
		{"wsec", "wSec", XtRInt, sizeof(int),
			XtOffset(struct _app_res*, wsec),
			XtRImmediate, (XtPointer) NULL},
		{"tmin", "Tmin", XtRFloat, sizeof(float),
			XtOffset(struct _app_res*, tmin),
			XtRImmediate, (XtPointer) NULL},
		{"tmax", "Tmax", XtRFloat, sizeof(float),
			XtOffset(struct _app_res*, tmax),
			XtRImmediate, (XtPointer) NULL},
		{"vmin", "Vmin", XtRFloat, sizeof(float),
			XtOffset(struct _app_res*, vmin),
			XtRImmediate, (XtPointer) NULL},
		{"vmax", "Vmax", XtRFloat, sizeof(float),
			XtOffset(struct _app_res*, vmax),
			XtRImmediate, (XtPointer) NULL},
		{"tick", "Tick", XtRInt, sizeof(int),
			XtOffset(struct _app_res*, tick),
			XtRImmediate, (XtPointer) NULL},
		{"cltmb", "ClTmb", XtRString, sizeof(String),
			XtOffset(struct _app_res*, cltmb),
			XtRString, (XtPointer) NULL},
		{"cltcpu", "ClTcpu", XtRString, sizeof(String),
			XtOffset(struct _app_res*, cltcpu),
			XtRString, (XtPointer) NULL},
		{"cltcs", "ClTcs", XtRString, sizeof(String),
			XtOffset(struct _app_res*, cltcs),
			XtRString, (XtPointer) NULL},
		{"clvc", "ClVc", XtRString, sizeof(String),
			XtOffset(struct _app_res*, clvc),
			XtRString, (XtPointer) NULL},
		{"cmtmb", "CMTmb", XtRString, sizeof(String),
			XtOffset(struct _app_res*, cmtmb),
			XtRString, (XtPointer) NULL},
		{"cmtcpu", "CMTcpu", XtRString, sizeof(String),
			XtOffset(struct _app_res*, cmtcpu),
			XtRString, (XtPointer) NULL},
		{"cmtcs", "CMTcs", XtRString, sizeof(String),
			XtOffset(struct _app_res*, cmtcs),
			XtRString, (XtPointer) NULL},
		{"cmvc", "CMVc", XtRString, sizeof(String),
			XtOffset(struct _app_res*, cmvc),
			XtRString, (XtPointer) NULL},
		{"fahrn","Fahrn",XtRBoolean,sizeof(Boolean),
			XtOffset(struct _app_res*, fahrn),
			XtRImmediate, (XtPointer) NULL},
		{"label","Label",XtRString,sizeof(String),
			XtOffset(struct _app_res*, label),
			XtRString,(XtPointer) NULL},
		{"labelfont","LabelFont",XtRString,sizeof(String),
			XtOffset(struct _app_res*, labelfont),
			XtRString,(XtPointer) NULL},
		{"labelcolor","LabelColor",XtRString,sizeof(String),
			XtOffset(struct _app_res*, labelcolor),
			XtRString,(XtPointer) NULL},
		{"method","Method",XtRString,sizeof(String),
			XtOffset(struct _app_res*, method),
			XtRString,(XtPointer) NULL},
		{"extratemp", "ExtraTemp", XtRInt, sizeof(int),
			XtOffset(struct _app_res*, extratemp),
			XtRImmediate, (XtPointer) NULL},
		{"probe","Method",XtRString,sizeof(String),
			XtOffset(struct _app_res*, probe),
			XtRString,(XtPointer) NULL},
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
		{"smbdev","SMBDEV",XtRString,sizeof(String),
			XtOffset(struct _app_res*, smbdev),
			XtRString,(XtPointer) NULL},
#endif
		{"TyanTigerMP","tyantigermp",XtRBoolean,sizeof(Boolean),
			XtOffset(struct _app_res*, TyanTigerMP),
			XtRImmediate, (XtPointer) NULL},
	};
	static XrmOptionDescRec options[] = {
		{"-font", ".font", XrmoptionSepArg, NULL},
		{"-count", ".count", XrmoptionSepArg, NULL},
		{"-sec", ".sec", XrmoptionSepArg, NULL},
		{"-wsec", ".wsec", XrmoptionSepArg, NULL},
		{"-tmin", ".tmin", XrmoptionSepArg, NULL},
		{"-tmax", ".tmax", XrmoptionSepArg, NULL},
		{"-vmin", ".vmin", XrmoptionSepArg, NULL},
		{"-vmax", ".vmax", XrmoptionSepArg, NULL},
		{"-tick", ".tick", XrmoptionSepArg, NULL},
		{"-cltmb", ".cltmb", XrmoptionSepArg, NULL},
		{"-cltcpu", ".cltcpu", XrmoptionSepArg, NULL},
		{"-cltcs", ".cltcs", XrmoptionSepArg, NULL},
		{"-clvc", ".clvc", XrmoptionSepArg, NULL},
		{"-cmtmb", ".cmtmb", XrmoptionSepArg, NULL},
		{"-cmtcpu", ".cmtcpu", XrmoptionSepArg, NULL},
		{"-cmtcs", ".cmtcs", XrmoptionSepArg, NULL},
		{"-cmvc", ".cmvc", XrmoptionSepArg, NULL},
		{"-fahrn",".fahrn",XrmoptionSepArg,NULL},
		{"-label",".label",XrmoptionSepArg,NULL},
		{"-labelfont",".labelfont",XrmoptionSepArg,NULL},
		{"-labelcolor",".labelcolor",XrmoptionSepArg,NULL},
		{"-method",".method",XrmoptionSepArg,NULL},
		{"-extratemp",".extratemp",XrmoptionSepArg,NULL},
		{"-probe",".probe",XrmoptionSepArg,NULL},
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
		{"-smbdev",".smbdev",XrmoptionSepArg,NULL},
#endif
		{"-TyanTigerMP",".TyanTigerMP",XrmoptionSepArg,NULL},
	};

/*---------- end of static data for X Toolkits ----------*/

void get_args(int argc, char *argv[])
{
	int	i;

	for (i = 1; i < argc; i++) {
		if (0 == strcmp("-d", argv[i]) || 0 == strcmp("-debug", argv[i])) {
			debug_flag = 1;
		}
		if (0 == strcmp("-D", argv[i]) || 0 == strcmp("-DEBUG", argv[i])) {
			debug_flag = 2;	/* debug option level 2 */
		}
		if (0 == strcmp("-Y", argv[i]) || 0 == strcmp("-TyanTigerMP", argv[i])) {
			TyanTigerMP_flag = 1;
		}
		if (0 == strcmp("-h", argv[i]) || 0 == strcmp("-help", argv[i])) {
			usage();
		}
	}
}

int chk_notemp(char *p)
{
	char *dum, *t;

	if (*p == (char) NULL)
		return (1);
	dum = (char *) malloc(strlen(p) + 1);
	strcpy(dum, p);
	for (t = dum; *t != (char) NULL; t++)
		if('a' <= *t && *t <= 'z')
			*t -= 'a' - 'A';
	if (strcmp(dum, NO_TEMP) == 0)
		return (1);
	else
		return (0);
}

void usage(void)
{
	fprintf(stderr, \
"X MotherBoard Monitor, ver. %s by YRS.\n"
"  options:\n"
"    : -g      (" DEFAULT_GEOMETRY ") <geometry(Toolkit option)>\n"
"    : -count        (" DEFAULT_COUNT ") <counts in an interval>\n"
"    : -sec         (" DEFAULT_CSEC ") <seconds of an interval>\n"
"    : -wsec      (" DEFAULT_WSEC ") <total seconds shown>\n"
"    : -tmin      (" DEFAULT_TMIN ") <min. temperature>\n"
"    : -tmax      (" DEFAULT_TMAX ") <max. temperature>\n"
"    : -vmin      (" DEFAULT_VMIN ") <min. voltage>\n"
"    : -vmax      (" DEFAULT_VMAX ") <max. voltage>\n"
"    : -tick         (" DEFAULT_TICK ") <ticks in ordinate>\n"
"    : -cltmb     (" DEFAULT_CLTMB ") <Temp1 color>\n"
"    : -cltcpu     (" DEFAULT_CLTCPU ") <Temp2 color>\n"
"    : -cltcs     (" DEFAULT_CLTCS ") <Temp3 color>\n"
"    : -clvc     (" DEFAULT_CLVC ") <Vcore color>\n"
"    : -cmtmb      (" DEFAULT_CMTMB1 ") <comment of Temp1>"
" [Not shown  ]\n"
"    : -cmtcpu     (" DEFAULT_CMTCPU ") <comment of Temp2>"
" [if \"" NO_TEMP "\"]\n"
"    : -cmtcs     (" DEFAULT_CMTCS ") <comment of Temp3>"
" [set.       ]\n"
"    : -cmvc       (" DEFAULT_CMVC1 ") <comment of Vcore>\n"
"    : -fahrn    (" DEFAULT_FAHRN ") <temp. in Fahrenheit (True|False)>\n"
"    : -label        ( ) for showing label [No label if null-string.]\n"
"                         and -labelfont, -labelcolor\n"
"    : -method       (%c) <access method (V|S|I|A)>\n"
"    : -extratemp    (2) set extra-temp. to temp[0|1|2] (need -A)\n"
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
"    : -smbdev [0-9] (0) for using /dev/smb[0-9]\n"
#endif
"    : -probe chip   ( ) chip="CHIPLIST"\n"
"                         for probing monitor chips\n"
"    : -TyanTigerMP      for Tyan Tiger MP motherboard\n"
"    : -debug/DEBUG      for debug information\n"
	, XMBMON_VERSION, DEFAULT_METHOD);
	exit(1);
}

int main(int argc, char *argv[])
{
	{
		uid_t uid = getuid();
		int r = seteuid(uid);
		if(r == -1) {
			perror("seteuid");
			return 1;
		}
	}

	get_args(argc, argv);

	wgt = XtVaAppInitialize(&app_con, RES_NAME,
			options, XtNumber(options),
			&argc, argv, fallback_resources, NULL);
	XtVaGetApplicationResources(wgt, (caddr_t) &app_resources,
			resources, XtNumber(resources), NULL);
	XtAppAddActions(app_con, actions, XtNumber(actions));
	XtRealizeWidget(wgt);

	get_res();
	{
	    int r = seteuid(0);
	    if(r == -1) {
			perror("seteuid");
			return 1;
	    }
	}
	init_dt();

	XtAppAddTimeOut(app_con, t_msec, (XtTimerCallbackProc) alarm_handler, NULL);
	XtAppMainLoop(app_con);

	return 0;
}

void get_res()
{
	int n;

	font = app_resources.font;
	if(font == NULL)
		font = DEFAULT_FONT;
	n_count = app_resources.count;
	if(n_count <= 0)
		n_count = atoi(DEFAULT_COUNT);
	interval_sec = app_resources.sec;
	if(interval_sec <= 0)
		interval_sec = atoi(DEFAULT_CSEC);
	w_sec = app_resources.wsec;
	if(w_sec <= 0)
		w_sec = atoi(DEFAULT_WSEC);
	tmin = app_resources.tmin;
	if(tmin <= 0.0)
		tmin = (float) atof(DEFAULT_TMIN);
	tmax = app_resources.tmax;
	if(tmax <= 0.0)
		tmax = (float) atof(DEFAULT_TMAX);
	vmin = app_resources.vmin;
	if(vmin <= 0.0)
		vmin = (float) atof(DEFAULT_VMIN);
	vmax = app_resources.vmax;
	if(vmax <= 0.0)
		vmax = (float) atof(DEFAULT_VMAX);
	n_tick = app_resources.tick;
	if(n_tick <= 0)
		n_tick = atoi(DEFAULT_TICK);
	l_color[0] = app_resources.cltmb;
	if(l_color[0] == NULL)
		l_color[0] = DEFAULT_CLTMB;
	l_color[1] = app_resources.cltcpu;
	if(l_color[1] == NULL)
		l_color[1] = DEFAULT_CLTCPU;
	l_color[2] = app_resources.cltcs;
	if(l_color[2] == NULL)
		l_color[2] = DEFAULT_CLTCS;
	l_color[3] = app_resources.clvc;
	if(l_color[3] == NULL)
		l_color[3] = DEFAULT_CLVC;
	c_rdg[0] = app_resources.cmtmb;
	if(c_rdg[0] == NULL)
		c_rdg[0] = DEFAULT_CMTMB1;
	else if (chk_notemp(c_rdg[0]))
		noTemp3_flag = 1;
	c_rdg[1] = app_resources.cmtcpu;
	if(c_rdg[1] == NULL)
		c_rdg[1] = DEFAULT_CMTCPU;
	else if (chk_notemp(c_rdg[1]))
		noTemp3_flag = 2;
	c_rdg[2] = app_resources.cmtcs;
	if(c_rdg[2] == NULL)
		c_rdg[2] = DEFAULT_CMTCS;
	else if (chk_notemp(c_rdg[2]))
		noTemp3_flag = 3;
	c_rdg[3] = app_resources.cmvc;
	if(c_rdg[3] == NULL)
		c_rdg[3] = DEFAULT_CMVC1;
	if(app_resources.fahrn) {
		fahrn_flag = 1;
		if( tmin == (float) atof(DEFAULT_TMIN) &&
				tmax == (float) atof(DEFAULT_TMAX) ) {
			tmin = (float) atof(DEFAULT_TMINF);
			tmax = (float) atof(DEFAULT_TMAXF);
		}
	}
	label = app_resources.label;
	lb_font = app_resources.labelfont;
	lb_color = app_resources.labelcolor;
	if(lb_color == NULL)
		lb_color = DEFAULT_LBCOLOR;
	if(app_resources.method != NULL)
		access_method = *app_resources.method;
	n = app_resources.extratemp;
	if (0 <= n && n <= 2)
		extra_tempNO = n;
	if(app_resources.probe != NULL)
		probe_request = app_resources.probe;
#if !defined(LINUX) && defined(HAVE_SMBUS) && defined(SMBUS_IOCTL)
	if(app_resources.smbdev != NULL) {
		n = atoi(app_resources.smbdev);
		if (n >= 1 && n <= 9)
			smb_devbuf[8] = *app_resources.smbdev;
	}
#endif
	if(app_resources.TyanTigerMP) {
		TyanTigerMP_flag = 1;
	}
	if(noTemp3_flag) {
		num_data = 3;
		switch(noTemp3_flag) {
		  case 1:
			t_indx[0] = 1;
			t_indx[1] = 2;
			break;
		  case 2:
			t_indx[0] = 0;
			t_indx[1] = 2;
			break;
		  case 3:
			t_indx[0] = 0;
			t_indx[1] = 1;
		}
			t_indx[2] = 3;
	}
}

void quit_proc(void)
{
	exit(1);
}

void repaint_proc(void)
{
	int wwd, hht;
	int i, id, n, ww = width -1, hh = height - 1;
	ring_d *p;

	for (i = 0; i < num_data; i++) {
		id = t_indx[i];
		sprintf(c_rdv[id], c_rdp[id], cur_val[id]);
		cv_width[i] = XTextWidth(fontstr, c_rdv[id], strlen(c_rdv[id]));
	}

	getWH(&wwd, &hht);
	if (wwd != width) {
		width = wwd;
		ww = width - 1;
		sscl = (float) width / (float) w_range;
	  if (noTemp3_flag) {
		x_rdg[1] = (width - cg_width[1]) / 2;
		x_rdg[2] =  width - cg_width[2]  - 1;
		x_rdv[1] = (width - cv_width[1]) / 2;
		x_rdv[2] =  width - cv_width[2]  - 1;
	  } else {
		x_rdg[1] = cg_width[0] / 2 + (width - cg_w) / 3 - cg_width[1] / 2;
		x_rdg[2] = cg_width[0] / 2 + (width - cg_w) * 2 / 3 - cg_width[2] / 2;
		x_rdg[3] = width - cg_width[3] - 1;
		cv_w = (cv_width[0] + cv_width[3]) / 2;
		x_rdv[1] = cv_width[0] / 2 + (width - cv_w) / 3 - cv_width[1] / 2;
		x_rdv[2] = cv_width[0] / 2 + (width - cv_w) * 2 / 3 - cv_width[2] / 2;
		x_rdv[3] =  width - cv_width[3]  - 1;
	  }
/*		x_lb = (width - lb_width) / 2;*/
	}
	if (hht != height) {
		height = hht;
		hh = height -1;
		tscl = (float) (height - c_lb - c_height)/ (tmax - tmin);
		vscl = (float) (height - c_lb - c_height)/ (vmax - vmin);
		htck = (float) (height - c_lb - c_height)/ (float) (n_tick + 1);
		scale[0] = tscl;
		scale[1] = tscl;
	  if (noTemp3_flag) {
		scale[2] = vscl;
	  } else {
		scale[2] = tscl;
		scale[3] = vscl;
	  }
	}

	XClearWindow(disp, win);

	if(*label != (char) NULL)
		XDrawString(disp, win, gclb, x_lb, c_lb - 2, label, strlen(label));

	for (i = 0; i < num_data; i++) {
		id = t_indx[i];
		XDrawString(disp, win, gcl[id], x_rdg[i], y_rdg, \
					c_rdg[id], strlen(c_rdg[id]));
		XDrawString(disp, win, gcl[id], x_rdv[i], y_rdv, \
					c_rdv[id], strlen(c_rdv[id]));
	}

	XDrawString(disp, win, gct, 0, hh+1, ctmin, strlen(ctmin));
	XDrawString(disp, win, gct, 0, cm_height, ctmax, strlen(ctmax));
	XDrawString(disp, win, gct, ww-cm_width, hh+1, cvmin, strlen(cvmin));
	XDrawString(disp, win, gct, ww-cm_width, cm_height, cvmax, strlen(cvmax));

	for (i = 0; i < n_tick + 1; i++) {
		n = hh - (int) ((float) (i + 1) * htck + 0.5);
		XDrawLine(disp, win, gct, 0, n, ww, n);
	}

	for (i = num_data - 1; i >= 0; i--) {
		id = t_indx[i];
		for (n = 0, p = rdp[i]; n < w_counter; n++) {
			(points + n)->x = (short) ((float) n * sscl + 0.5);
			(points + n)->y = hh \
				- (short) ((p->val - orign[i]) * scale[i] + 0.5);
			p = p->next;
		}
		XDrawLines(disp, win, gcl[id], points, w_counter, CoordModeOrigin);
	}

	XFlush(disp);
}

void getWH(int *width, int *height)
{
	Dimension wd, ht;

	XtVaGetValues(wgt, XtNwidth, &wd, XtNheight, &ht, NULL);
	*width = (int) wd;
	*height = (int) ht;
}

int ColorPix(Display *display, char *color, unsigned long *cpix)
{
	Colormap cmap;
	XColor c0, c1;

	if (color == NULL)
		return (1);
	cmap = DefaultColormap(display, 0);
	if (XAllocNamedColor(disp, cmap, color, &c1, &c0)) {
		*cpix = c1.pixel;
		return (0);
	} else {
		return (1);
	}
}

void init_dt(void)
{
	ring_d *p;
	int n, i, id;
	float d;
	
	if (w_sec < 10 * interval_sec) {
		fprintf(stderr,"wsec(%d) is too small w.r.t sec(%d) --- exit.\n",\
			w_sec, interval_sec);
		exit (1);
	}
	if (tmax < tmin) {
		fprintf(stderr,"tmin(%f) > tmax(%f), not allowed! --- exit.\n",\
			tmin, tmax);
		exit (1);
	}
	if (vmax < vmin) {
		fprintf(stderr,"vmin(%f) > vmax(%f), not allowed! --- exit.\n",\
			vmin, vmax);
		exit (1);
	}
	if ((n = InitMBInfo(access_method)) != 0) {
		perror("InitMBInfo");
		if(n < 0)
			fprintf(stderr,"This program needs \"setuid root\"!!\n");
		exit (1);
	}
	if (debug_flag)
		exit (0);

	getTemp(&cur_val[0], &cur_val[1], &cur_val[2]);
	getVolt(&cur_val[3], &d, &d, &d, &d, &d, &d);
	
	disp = XtDisplay(wgt);
	win = XtWindow(wgt);
	getWH(&width, &height);

	if ((fontstr = XLoadQueryFont(disp, font)) == NULL) {
		fprintf(stderr,"Can't find font: %s\n", font);
		exit (1);
	}

	ColorPix(disp, DEFAULT_CLTMB, &(d_cpix[0]));
	ColorPix(disp, DEFAULT_CLTCPU, &(d_cpix[1]));
	ColorPix(disp, DEFAULT_CLTCS, &(d_cpix[2]));
	ColorPix(disp, DEFAULT_CLVC, &(d_cpix[3]));
	gct = XCreateGC(disp, win, 0, 0);
	XSetFont(disp, gct, fontstr->fid);
	for (i = 0; i < num_data; i++) {
		id = t_indx[i];
		if (ColorPix(disp, l_color[id], &(cpix[id])))
			cpix[id] = d_cpix[id];
		gcl[id] = XCreateGC(disp, win, 0, 0);
		XSetLineAttributes(disp, gcl[id], 2, LineSolid, CapRound, JoinRound);
		XSetForeground(disp, gcl[id], cpix[id]);
		XSetFont(disp, gcl[id], fontstr->fid);
		c_rdv[id] = (char *) malloc(C_LBL);
		sprintf(c_rdv[id], c_rdp[id], cur_val[id]);
		cg_width[i] = XTextWidth(fontstr, c_rdg[id], strlen(c_rdg[id]));
		cv_width[i] = XTextWidth(fontstr, c_rdv[id], strlen(c_rdv[id]));
	}

	if(*label != (char) NULL) {
	  if ((lb_fontstr = XLoadQueryFont(disp, lb_font)) == NULL) {
		fprintf(stderr,"Can't find font for label: %s\n", font);
		exit (1);
	  }
		ColorPix(disp, DEFAULT_LBCOLOR, &d_cpixlb);
		if (ColorPix(disp, lb_color, &cpixlb))
			cpixlb = d_cpixlb;
		gclb = XCreateGC(disp, win, 0, 0);
		XSetFont(disp, gclb, lb_fontstr->fid);
		XSetForeground(disp, gclb, cpixlb);
		lb_width = XTextWidth(lb_fontstr, label, strlen(label));
	}

	sprintf(ctmin, "%2.0f", tmin);
	sprintf(ctmax, "%2.0f", tmax);
	sprintf(cvmin, "%3.1f", vmin);
	sprintf(cvmax, "%3.1f", vmax);

	w_range = w_sec / interval_sec;
	points = (XPoint *) malloc(sizeof(short) * 2 * (w_range + 1));

	for (i = 0; i < num_data; i++) {
		rdp[i] = p = (ring_d *) malloc(sizeof(ring_d));	
		rdpw[i] = p;
		for (n = 1; n < (w_range + 1); n++) {
			p->next = (ring_d *) malloc(sizeof(ring_d));	
			p = p->next;
		}
		p->next = rdp[i];
	}
	fmean = (float *) malloc(sizeof(float) * num_data * n_count);

	sscl = (float) width / (float) w_range;
	orign[0] = tmin;
	orign[1] = tmin;
	x_rdg[0] = 0;
	x_rdv[0] = 0;
  if (noTemp3_flag) {
	orign[2] = vmin;
	x_rdg[1] = (width - cg_width[1]) / 2;
	x_rdg[2] =  width - cg_width[2]  - 1;
	x_rdv[1] = (width - cv_width[1]) / 2;
	x_rdv[2] =  width - cv_width[2]  - 1;
  } else {
	orign[2] = tmin;
	orign[3] = vmin;
	cg_w = (cg_width[0] + cg_width[3]) / 2;
	x_rdg[1] = cg_width[0] / 2 + (width - cg_w) / 3 - cg_width[1] / 2;
	x_rdg[2] = cg_width[0] / 2 + (width - cg_w) * 2 / 3 - cg_width[2] / 2;
	x_rdg[3] = width - cg_width[3] - 1;
	cv_w = (cv_width[0] + cv_width[3]) / 2;
	x_rdv[1] = cv_width[0] / 2 + (width - cv_w) / 3 - cv_width[1] / 2;
	x_rdv[2] = cv_width[0] / 2 + (width - cv_w) * 2 / 3 - cv_width[2] / 2;
	x_rdv[3] = width - cv_width[3] - 1;
  }
	y_rdg = fontstr->max_bounds.ascent - 1;
/*	y_rdv = 2 * fontstr->max_bounds.ascent + fontstr->max_bounds.descent;
	y_rdv = 2 * fontstr->max_bounds.ascent;*/
	y_rdv = 2 * y_rdg;
/*	c_height = 2 *(fontstr->max_bounds.ascent + fontstr->max_bounds.descent);*/
	c_height = 2 * fontstr->max_bounds.ascent;
	if(*label != (char) NULL) {
/*		x_lb = (width - lb_width) / 2;*/
		x_lb = 2;
		c_lb = lb_fontstr->max_bounds.ascent + 1;
		y_rdg += c_lb;
		y_rdv += c_lb;
	}
	cm_height = c_height + y_rdg - 1;
	cm_width = XTextWidth(fontstr, cvmin, strlen(cvmin)) - 1;
	tscl = (float) (height - c_lb - c_height)/ (tmax - tmin);
	vscl = (float) (height - c_lb - c_height)/ (vmax - vmin);
	htck = (float) (height - c_lb - c_height)/ (float) (n_tick + 1);
	scale[0] = tscl;
	scale[1] = tscl;
  if (noTemp3_flag) {
	scale[2] = vscl;
  } else {
	scale[2] = tscl;
	scale[3] = vscl;
  }

	t_sec = interval_sec / n_count;
	t_msec = (1000 * interval_sec) / n_count;
	iir_tc = t_msec > 500 ? 0.5 : 0.25;
	for (i = 0; i < NUM_DATA; i++)
		iir_reg[i] = 0;
}

#define	THREDMAX	1.5
#define	THREDMIN	0.5
void alarm_handler(void)
{
	float d, *p;
	int i, id, n;
	float temp_in[4], tpd[4];

	getTemp(&tpd[0], &tpd[1], &tpd[2]);
	getVolt(&tpd[3], &d, &d, &d, &d, &d, &d);
	if (t_msec <= 1000){
		for (i = 0; i < NUM_DATA; i++) {
			if (THREDMIN < tpd[i]/cur_val[i] && tpd[i]/cur_val[i] < THREDMAX)
				temp_in[i] = tpd[i];
		}
			/* apply 2nd order IIR LPF, by Takayuki Hosoda */
		for (i = 0; i < NUM_DATA; i++) {
			d = iir_reg[i];
			iir_reg[i] = d - (2.0 * d + temp_in[i] - cur_val[i]) * iir_tc;
			cur_val[i] -= d * iir_tc;
		}
	} else {
		for (i = 0; i < NUM_DATA; i++) {
			if (THREDMIN < tpd[i]/cur_val[i] && tpd[i]/cur_val[i] < THREDMAX)
				cur_val[i] = tpd[i];
		}
	}

	draw_values();

	p = fmean + counter;
	for (i = 0; i < num_data; i++) {
		id = t_indx[i];
		*(p + i * n_count) = cur_val[id];
	}

	counter++;
	if (counter == n_count) {
		if (w_counter <= w_range)
			w_counter++;
		else {
			for (i = 0; i < num_data; i++)
				rdp[i] = rdp[i]->next;
		}
		counter = 0;
		p = fmean;
		for (i = 0; i < num_data; i++) {
			for (n = 0, d = 0.0; n < n_count; n++, p++)
				d += *p;
			rdpw[i]->val = d / (float) n_count;
			rdpw[i] = rdpw[i]->next;
		}
		repaint_proc();
	}
	XtAppAddTimeOut(app_con, t_msec, (XtTimerCallbackProc) alarm_handler, NULL);
}

void draw_values(void)
{
	int i, id;

	XClearArea(disp, win, 0, y_rdg , 0, y_rdg - c_lb, False);
	for (i = 0; i < num_data; i++) {
		id = t_indx[i];
		sprintf(c_rdv[id], c_rdp[id], cur_val[id]);
		XDrawString(disp, win, gcl[id], x_rdv[i], y_rdv, \
					c_rdv[id], strlen(c_rdv[id]));
	}
	XFlush(disp);
}
