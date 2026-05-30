/*
 * ============================================================
 *  BALLISTIC CALCULATOR v1.2
 *  M5Stack Cardputer ADV (ESP32-S3FN8)
 *  - Fixed: drop sign, cartridge zero override, reset consistency
 *  - Added: numeric input, panel consolidation, caliber scroll
 * ============================================================
 */
#include <M5Cardputer.h>
#include <math.h>
#include <string.h>

#define SW 240
#define SH 135

#define C_BG    0x08C4
#define C_SURF  0x10E6
#define C_SURF2 0x1928
#define C_ACC   0xF4E0
#define C_ACC2  0xFD80
#define C_WHT   0xFFFF
#define C_DIM   0xA554
#define C_RED   0xF940
#define C_GRN   0x4E46
#define C_CYA   0x2F7F
#define C_ORG   0xFCA0
#define C_YEL   0xFF86
#define C_GRD   0x2145
#define C_DRK   0x0842
#define C_SEL   0x1168

#define K_UP  0x33
#define K_DN  0x37
#define K_LT  0x36
#define K_RT  0x38
#define K_ESC 0x35
#define K_ENT 0x28
#define K_BK  0x2A
#define K_SP  0x2C
#define K_1   0x1E
#define K_2   0x1F
#define K_3   0x20
#define K_4   0x21
#define K_5   0x22
#define K_6   0x23
#define K_7   0x24
#define K_8   0x25
#define K_9   0x26
#define K_0   0x27
#define K_MI  0x2D
#define K_EQ  0x2E
#define K_DOT 0x56

// ═══════════════════════════════════════════════════════════
//  ALL STRUCT DEFINITIONS
// ═══════════════════════════════════════════════════════════

struct Cart {
    const char* name;
    const char* cal;
    float mv, bc, wt;
    uint8_t dm;
};

struct Res {
    float rng, drop, drift, vel, ke, time;
};

struct Params {
    float mv   = 792.0f;
    int   zr   = 100;
    float sh   = 3.8f;
    int   dm   = 1;
    float bc   = 0.505f;
    float wt   = 11.34f;
    float ws   = 0.0f;
    float wa   = 90.0f;
    float tr   = 500.0f;
    float temp = 15.0f;
    float pr   = 1013.25f;
    int   cartIdx = 0;
};

// ═══════════════════════════════════════════════════════════
//  CARTRIDGE DATABASE
// ═══════════════════════════════════════════════════════════

const Cart CART[] = {
    {"55gr V-Max",     ".223",  988, 0.255f,  3.56f, 1},
    {"62gr Fusion",    ".223",  914, 0.310f,  4.02f, 1},
    {"77gr TMK",       ".223",  823, 0.420f,  4.99f, 1},
    {"130gr Hybrid",   "6.5CM", 876, 0.532f,  8.42f, 7},
    {"140gr ELD-M",    "6.5CM", 826, 0.590f,  9.07f, 7},
    {"143gr ELD-X",    "6.5CM", 823, 0.595f,  9.27f, 7},
    {"150gr TSX",      ".308",  853, 0.435f,  9.72f, 1},
    {"168gr GM",       ".308",  808, 0.462f, 10.89f, 1},
    {"175gr GM",       ".308",  792, 0.505f, 11.34f, 1},
    {"150gr TSX",      ".30-06",890, 0.435f,  9.72f, 1},
    {"165gr TSX",      ".30-06",860, 0.462f, 10.69f, 1},
    {"180gr ELD-X",    ".30-06",823, 0.564f, 11.66f, 7},
    {"180gr TSX",      ".300WM",884, 0.564f, 11.66f, 1},
    {"200gr ELD-X",    ".300WM",853, 0.631f, 12.96f, 7},
    {"150gr TSX",      "7mmRM", 945, 0.450f,  9.72f, 1},
    {"175gr ELD-X",    "7mmRM", 884, 0.604f, 11.34f, 7},
    {"250gr GM",       ".338LM",902, 0.600f, 16.20f, 1},
    {"300gr SMK",      ".338LM",838, 0.768f, 19.44f, 1},
    {"Custom Load",    "Custom",792, 0.505f, 11.34f, 1},
};
const int CART_N = sizeof(CART) / sizeof(CART[0]);

// ── Caliber index (built at runtime) ───────────────────────
int calStart[12];
int calLoadN[12];
int calN = 0;

void buildCalIndex() {
    calN = 0;
    for (int i = 0; i < CART_N; i++) {
        if (i == 0 || strcmp(CART[i].cal, CART[i - 1].cal) != 0) {
            calStart[calN] = i;
            calLoadN[calN] = 1;
            calN++;
        } else {
            calLoadN[calN - 1]++;
        }
    }
}

// ── Global State ────────────────────────────────────────────
Params P;
Res    R[500];
int    RN = 0;
bool   computed = false, dirty = true, needsDraw = true;
int    scr = 0, mSel = 0, eFld = 0, eCur = 0;
int    rSel = 0, rOff = 0;
int    gOff = 0;

// Cartridge selector state
int    cartLevel = 0;
int    calSel    = 0;
int    calOff    = 0;   //caliber list scroll offset
int    loadSel   = 0;

// Range card view
int    cardView  = 0;

// Numeric input state for editor
char   editBuf[16];
int    editLen = 0;
bool   editing = false;

uint8_t       _rk = 0;
unsigned long _rkt = 0, _rrt = 0;

// ═══════════════════════════════════════════════════════════
//  PHYSICS ENGINE (time-stepping)
// ═══════════════════════════════════════════════════════════

static const float G1M[] = {
    0, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.825f, 0.85f, 0.875f,
    0.9f, 0.925f, 0.95f, 0.975f, 1.0f, 1.05f, 1.1f, 1.2f,
    1.3f, 1.5f, 1.7f, 2.0f, 2.5f
};
static const float G1D[] = {
    0.1198f, 0.1196f, 0.1194f, 0.1193f, 0.1194f, 0.1224f, 0.1242f, 0.1282f, 0.1358f,
    0.1492f, 0.1646f, 0.1858f, 0.2124f, 0.3820f, 0.3682f, 0.3510f, 0.3304f,
    0.3204f, 0.3074f, 0.2996f, 0.2934f, 0.2894f
};
static const float G7M[] = {
    0, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.85f, 0.9f, 0.925f,
    0.95f, 0.975f, 1.0f, 1.025f, 1.05f, 1.075f, 1.1f, 1.15f, 1.2f,
    1.3f, 1.5f, 1.7f, 2.0f, 2.5f
};
static const float G7D[] = {
    0.1198f, 0.1197f, 0.1196f, 0.1194f, 0.1194f, 0.1210f, 0.1226f, 0.1264f, 0.1302f,
    0.1386f, 0.1532f, 0.3204f, 0.2970f, 0.2840f, 0.2730f, 0.2654f, 0.2542f, 0.2460f,
    0.2378f, 0.2284f, 0.2230f, 0.2196f, 0.2174f
};

float interpCd(float m, const float* mt, const float* dt, int n) {
    if (m <= mt[0]) return dt[0];
    if (m >= mt[n - 1]) return dt[n - 1];
    int lo = 0;
    for (int i = 1; i < n; i++) { if (mt[i] >= m) { lo = i - 1; break; } }
    float t = (m - mt[lo]) / (mt[lo + 1] - mt[lo]);
    return dt[lo] + t * (dt[lo + 1] - dt[lo]);
}

float getCD(float mach, int dm) {
    return dm == 7 ? interpCd(mach, G7M, G7D, 23)
                   : interpCd(mach, G1M, G1D, 23);
}

void compute(Params* p, Res* o, int* cnt) {
    float v0     = p->mv * 3.28084f;
    float shFt   = p->sh / 30.48f;
    float zrFt   = (float)p->zr * 3.28084f;
    float tgtFt  = p->tr * 3.28084f;
    float tempF  = p->temp * 1.8f + 32.0f;
    float prInHg = p->pr * 0.02953f;
    float wf     = p->ws * 3.28084f;
    float waRad  = p->wa * 0.01745329f;
    float wCross = wf * sinf(waRad);

    float tau   = (tempF + 459.67f) / 518.67f;
    float sigma = (prInHg / 29.92f) / tau;
    float bcEff = p->bc * sigma;

    float dt  = 0.0005f;
    float grav = 32.174f;
    float K    = 0.0002086f;

    float recM = 1.0f;
    if (p->tr > 1000)      recM = 5.0f;
    else if (p->tr > 500)  recM = 2.0f;
    float recFt = recM * 3.28084f;

    float alpha = atanf(shFt / zrFt);
    for (int iter = 0; iter < 20; iter++) {
        float x = 0, y = -shFt;
        float vx = v0 * cosf(alpha), vy = v0 * sinf(alpha);
        while (x < zrFt) {
            float v = sqrtf(vx * vx + vy * vy);
            if (v < 10.0f) break;
            float cd = getCD(v * sqrtf(tau) / 1116.45f, p->dm);
            float D  = K * cd * v * v / bcEff;
            vx += (-D * vx / v) * dt;
            vy += (-D * vy / v - grav) * dt;
            x  += vx * dt;
            y  += vy * dt;
        }
        float corr = y / zrFt;
        alpha -= corr;
        if (fabsf(corr) < 1e-7f) break;
    }

    float x = 0, y = -shFt;
    float vx = v0 * cosf(alpha), vy = v0 * sinf(alpha);
    float vlat = 0, drift = 0, ft = 0;
    float nextRec = 0;
    int ri = 0;

    while (ri < 500) {
        float v = sqrtf(vx * vx + vy * vy);
        if (v < 10.0f || x > tgtFt + recFt * 10) break;
        float cd = getCD(v * sqrtf(tau) / 1116.45f, p->dm);
        float D  = K * cd * v * v / bcEff;
        float a_lat = D * wCross / v;
        vlat  += a_lat * dt;
        drift += vlat * dt;
        vx += (-D * vx / v) * dt;
        vy += (-D * vy / v - grav) * dt;
        x  += vx * dt;
        y  += vy * dt;
        ft += dt;

        if (x >= nextRec && ri < 500) {
            float vMs = v * 0.3048f;
            o[ri].rng   = x / 3.28084f;
            o[ri].drop  = y * 30.48f;
            o[ri].drift = drift * 30.48f;
            o[ri].vel   = vMs;
            float keJ   = 0.5f * (p->wt / 1000.0f) * vMs * vMs;
            o[ri].ke    = (isnan(keJ) || isinf(keJ)) ? 0.0f : keJ;
            o[ri].time  = ft;
            ri++;
            nextRec += recFt;
        }
    }
    *cnt = ri;
}

// ═══════════════════════════════════════════════════════════
//  HELPERS
// ═══════════════════════════════════════════════════════════

int findNearest(float d) {
    if (RN <= 0) return 0;
    int lo = 0, hi = RN - 1;
    while (lo < hi) { int m = (lo + hi) / 2; if (R[m].rng < d) lo = m + 1; else hi = m; }
    if (lo > 0 && fabsf(R[lo - 1].rng - d) < fabsf(R[lo].rng - d)) return lo - 1;
    return lo;
}

int getInterval() {
    if (P.tr <= 200) return 10;
    if (P.tr <= 500) return 25;
    return 50;
}

float toMOA(float cm, float m) { return (m > 0.5f) ? cm * 34.37f / m : 0; }
float toMIL(float cm, float m) { return (m > 0.5f) ? cm * 10.0f / m  : 0; }

void fmtKE(char* buf, float ke) {
    if (ke >= 10000.0f) sprintf(buf, "%.1fk", ke / 1000.0f);
    else if (ke < 0.5f) sprintf(buf, "0");
    else sprintf(buf, "%d", (int)ke);
}

void fillGrad(int x, int y, int w, int h, uint16_t c1, uint16_t c2) {
    float r1 = ((c1 >> 11) & 0x1F) / 31.0f;
    float g1 = ((c1 >>  5) & 0x3F) / 63.0f;
    float b1 = ( c1        & 0x1F) / 31.0f;
    float r2 = ((c2 >> 11) & 0x1F) / 31.0f;
    float g2 = ((c2 >>  5) & 0x3F) / 63.0f;
    float b2 = ( c2        & 0x1F) / 31.0f;
    for (int i = 0; i < h; i++) {
        float t = (float)i / max(h - 1, 1);
        M5.Display.drawFastHLine(x, y + i, w,
            (uint8_t)((r1 + (r2 - r1) * t) * 31) << 11 |
            (uint8_t)((g1 + (g2 - g1) * t) * 63) << 5  |
            (uint8_t)((b1 + (b2 - b1) * t) * 31));
    }
}

void hdr(const char* t, const char* r = "") {
    fillGrad(0, 0, SW, 16, C_DRK, C_SURF);
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextColor(C_ACC);
    M5.Display.setTextSize(1);
    M5.Display.drawString(t, 4, 8);
    if (r[0]) {
        M5.Display.setTextDatum(middle_right);
        M5.Display.setTextColor(C_DIM);
        M5.Display.drawString(r, SW - 4, 8);
    }
}

// ═══════════════════════════════════════════════════════════
//  SCREENS
// ═══════════════════════════════════════════════════════════

// ── 0. Main Menu ────────────────────────────────────────────
void drawMenu() {
    M5.Display.fillScreen(C_BG);
    hdr("BALLISTIC CALC", "v1.2");

    const char* items[] = {
        "Select Cartridge", "Edit Parameters",
        "Range Card", "Trajectory Graph", "Reset Defaults"
    };
    int my = 20;
    for (int i = 0; i < 5; i++) {
        bool s = (i == mSel);
        if (s) M5.Display.fillRect(4, my, SW - 60, 16, C_SEL);
        M5.Display.setTextDatum(middle_left);
        M5.Display.setTextSize(1);
        char b[4]; sprintf(b, "%d.", i + 1);
        M5.Display.setTextColor(s ? C_ACC : C_DIM);
        M5.Display.drawString(b, 8, my + 8);
        M5.Display.setTextColor(s ? C_WHT : C_DIM);
        M5.Display.drawString(items[i], 22, my + 8);
        my += 16;
    }

    int bx = SW - 56, bw = 54;
    M5.Display.drawRect(bx, 18, bw, 88, C_SURF2);

    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextColor(C_ACC);
    M5.Display.setTextSize(1);
    M5.Display.drawString("LOAD", bx + bw / 2, 24);

    const Cart* c = &CART[P.cartIdx];
    char buf[20];

    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextColor(C_WHT);
    strncpy(buf, c->cal, 12); buf[12] = 0;
    M5.Display.drawString(buf, bx + 3, 34);
    M5.Display.setTextColor(C_DIM);
    strncpy(buf, c->name, 14); buf[14] = 0;
    M5.Display.drawString(buf, bx + 3, 44);

    M5.Display.drawFastHLine(bx + 2, 51, bw - 4, C_GRD);

    M5.Display.setTextColor(C_DIM);
    sprintf(buf, "V0:%d", (int)P.mv);   M5.Display.drawString(buf, bx + 3, 58);
    sprintf(buf, "Z:%dm", P.zr);        M5.Display.drawString(buf, bx + 3, 66);
    sprintf(buf, "BC:%.3f", P.bc);      M5.Display.drawString(buf, bx + 3, 74);

    M5.Display.drawFastHLine(bx + 2, 80, bw - 4, C_GRD);

    sprintf(buf, "G%d Tr:%dm", P.dm, (int)P.tr);
    M5.Display.drawString(buf, bx + 3, 87);

    M5.Display.drawFastHLine(bx + 2, 93, bw - 4, C_GRD);

    sprintf(buf, "Pts:%d", RN);
    M5.Display.drawString(buf, bx + 3, 100);

    M5.Display.setTextDatum(bottom_center);
    M5.Display.setTextColor(C_DIM);
    M5.Display.drawString("UP/DN=Move ENTER=Open", SW / 2, SH - 1);
}

// ── 1. Cartridge Selector ───────────────────────────────────
void drawCartSel() {
    M5.Display.fillScreen(C_BG);

    if (cartLevel == 0) {
        hdr("SELECT CALIBER", "ESC=Back");

        int visCount = 7;

        // Scroll window follows selection
        if (calSel < calOff) calOff = calSel;
        if (calSel >= calOff + visCount) calOff = calSel - visCount + 1;

        int my = 18;
        for (int i = calOff; i < calN && i < calOff + visCount; i++) {
            bool s = (i == calSel);
            if (s) M5.Display.fillRect(4, my, SW - 8, 14, C_SEL);
            M5.Display.setTextDatum(middle_left);
            M5.Display.setTextSize(1);
            M5.Display.setTextColor(s ? C_WHT : C_DIM);
            M5.Display.drawString(CART[calStart[i]].cal, 8, my + 5);
            char buf[16]; sprintf(buf, "%d loads", calLoadN[i]);
            M5.Display.setTextDatum(middle_right);
            M5.Display.setTextColor(s ? C_DIM : C_GRD);
            M5.Display.drawString(buf, SW - 8, my + 5);
            my += 14;
        }

        // Scroll indicator
        if (calN > visCount) {
            int barH = (int)((float)visCount / calN * 80);
            if (barH < 6) barH = 6;
            int barY = 18 + (int)((float)calOff / (calN - visCount) * (80 - barH));
            M5.Display.fillRect(SW - 3, barY, 2, barH, C_SURF2);
        }

        M5.Display.setTextDatum(bottom_center);
        M5.Display.setTextColor(C_DIM);
        M5.Display.drawString("UP/DN=Select ENTER=Open", SW / 2, SH - 1);
    } else {
        int start = calStart[calSel];
        int count = calLoadN[calSel];
        hdr(CART[start].cal, "ESC=Back");
        int my = 18;
        for (int i = 0; i < count; i++) {
            bool s = (i == loadSel);
            if (s) M5.Display.fillRect(4, my, SW - 8, 14, C_SEL);
            M5.Display.setTextDatum(middle_left);
            M5.Display.setTextSize(1);
            M5.Display.setTextColor(s ? C_ACC : C_WHT);
            M5.Display.drawString(CART[start + i].name, 8, my + 5);
            char buf[24]; sprintf(buf, "%dm/s", (int)CART[start + i].mv);
            M5.Display.setTextDatum(middle_right);
            M5.Display.setTextColor(s ? C_WHT : C_DIM);
            M5.Display.drawString(buf, SW - 8, my + 5);
            my += 14;
        }
        if (loadSel < count) {
            const Cart* c = &CART[start + loadSel];
            M5.Display.drawFastHLine(0, my, SW, C_SURF2);
            M5.Display.setTextDatum(middle_left);
            M5.Display.setTextColor(C_GRD);
            M5.Display.setTextSize(1);
            char buf[40];
            sprintf(buf, "BC:%.3f  W:%.1fg  G%d", c->bc, c->wt, c->dm);
            M5.Display.drawString(buf, 4, my + 6);
        }
        M5.Display.setTextDatum(bottom_center);
        M5.Display.setTextColor(C_DIM);
        M5.Display.drawString("UP/DN=Load  ENTER=Select", SW / 2, SH - 1);
    }
}

// ── 2. Editor ───────────────────────────────────────────────
void drawEditor() {
    M5.Display.fillScreen(C_BG);
    hdr("EDIT PARAMETERS", "ESC=Back");

    const char* lb[] = {
        "MUZZLE VEL (m/s)", "ZERO RANGE (m)", "SIGHT HT (cm)",
        "BC", "DRAG MODEL",
        "BULLET WT (g)", "WIND SPEED (m/s)", "WIND ANGLE (deg)",
        "TEMP (C)", "PRESSURE (hPa)", "TARGET RNG (m)"
    };

    int ey = 18;
    for (int i = 0; i < 11; i++) {
        if (i < eCur || i >= eCur + 4) continue;
        bool s = (i == eFld);
        if (s) {
            M5.Display.fillRect(0, ey, SW, 24, C_SEL);
            M5.Display.fillRect(0, ey, 3, 24, C_ACC);
        }
        M5.Display.setTextDatum(middle_left);
        M5.Display.setTextColor(s ? C_ACC : C_DIM);
        M5.Display.setTextSize(1);
        M5.Display.drawString(lb[i], 6, ey + 8);

        M5.Display.setTextDatum(middle_right);
        if (editing && s && eFld != 4) {
            char disp[20];
            bool showCur = (millis() / 500) % 2 == 0;
            snprintf(disp, sizeof(disp), "%s%s", editBuf, showCur ? "_" : " ");
            M5.Display.setTextColor(C_YEL);
            M5.Display.drawString(disp, SW - 8, ey + 8);
        } else {
            char v[24];
            switch (i) {
                case  0: sprintf(v, "%d m/s",   (int)P.mv);  break;
                case  1: sprintf(v, "%d m",     P.zr);       break;
                case  2: sprintf(v, "%.1f cm",  P.sh);       break;
                case  3: sprintf(v, "%.3f",     P.bc);       break;
                case  4: sprintf(v, "G%d",      P.dm);       break;
                case  5: sprintf(v, "%.1f g",   P.wt);       break;
                case  6: sprintf(v, "%.1f m/s", P.ws);       break;
                case  7: sprintf(v, "%.0f deg", P.wa);       break;
                case  8: sprintf(v, "%.1f C",   P.temp);     break;
                case  9: sprintf(v, "%.0f hPa", P.pr);       break;
                case 10: sprintf(v, "%d m",     (int)P.tr);  break;
            }
            M5.Display.setTextColor(s ? C_WHT : C_DIM);
            M5.Display.drawString(v, SW - 8, ey + 8);
        }
        ey += 24;
    }
    M5.Display.setTextDatum(bottom_center);
    M5.Display.setTextColor(C_DIM);
    M5.Display.drawString("0-9=Type FN+DN=Dot BK=Del", SW / 2, SH - 1);
}

// ── 3. Range Card ───────────────────────────────────────────
void drawRangeCard() {
    M5.Display.fillScreen(C_BG);
    if (!computed || RN == 0) {
        hdr("RANGE CARD", "--");
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextColor(C_DIM);
        M5.Display.setTextSize(1);
        M5.Display.drawString("No data.", SW / 2, SH / 2);
        return;
    }

    int interval = getInterval();
    int cardMax  = (int)(P.tr / interval);

    if (cardView == 0) {
        char cs[24]; sprintf(cs, "%dm INT", interval);
        hdr("RANGE CARD", cs);

        M5.Display.setTextDatum(middle_left);
        M5.Display.setTextSize(1);
        M5.Display.setTextColor(C_ACC); M5.Display.drawString("RNG",   4,   18);
        M5.Display.setTextColor(C_WHT); M5.Display.drawString("DROP",  40,  18);
        M5.Display.setTextColor(C_CYA); M5.Display.drawString("DRIFT", 88,  18);
        M5.Display.setTextColor(C_ORG);
        M5.Display.setTextDatum(middle_right);
        M5.Display.drawString("VEL", 168, 18);
        M5.Display.setTextColor(C_YEL); M5.Display.drawString("KE", SW - 4, 18);
        M5.Display.setTextColor(C_GRD);
        M5.Display.setTextDatum(middle_left);
        M5.Display.drawString("m",    8,   25);
        M5.Display.drawString("cm",  46,   25);
        M5.Display.drawString("cm",  94,   25);
        M5.Display.setTextDatum(middle_right);
        M5.Display.drawString("m/s", 168,  25);
        M5.Display.drawString("J",   SW-4, 25);
        M5.Display.drawFastHLine(0, 31, SW, C_GRD);

        int ry = 33;
        for (int r = 0; r < 4; r++) {
            int ci = rOff + r;
            if (ci > cardMax) break;
            float dist = (float)(ci + 1) * interval;
            if (dist > P.tr + 1) break;
            int idx = findNearest(dist);
            bool s = (ci == rSel);
            bool isZ = (fabsf(dist - (float)P.zr) < interval * 0.6f);
            if (s) M5.Display.fillRect(0, ry, SW, 20, C_SEL);
            Res* b = &R[idx];
            char t[16];

            sprintf(t, "%.0f%s", dist, isZ ? "*" : " ");
            M5.Display.setTextDatum(middle_left);
            M5.Display.setTextColor(C_ACC);
            M5.Display.setTextSize(1);
            M5.Display.drawString(t, 4, ry + 7);

            sprintf(t, "%+.2f", b->drop);
            M5.Display.setTextColor(C_WHT);
            M5.Display.drawString(t, 40, ry + 7);

            sprintf(t, "%+.2f", b->drift);
            M5.Display.setTextColor(C_CYA);
            M5.Display.drawString(t, 88, ry + 7);

            sprintf(t, "%d", (int)b->vel);
            M5.Display.setTextDatum(middle_right);
            M5.Display.setTextColor(C_ORG);
            M5.Display.drawString(t, 168, ry + 7);

            fmtKE(t, b->ke);
            M5.Display.setTextColor(C_YEL);
            M5.Display.drawString(t, SW - 4, ry + 7);
            ry += 20;
        }

    } else {
        hdr("RANGE CARD", "ADJUSTMENTS");

        M5.Display.setTextDatum(middle_left);
        M5.Display.setTextSize(1);
        M5.Display.setTextColor(C_ACC); M5.Display.drawString("RNG",   4,   18);
        M5.Display.setTextColor(C_WHT); M5.Display.drawString("eMOA",  36,  18);
        M5.Display.setTextColor(C_YEL); M5.Display.drawString("eMIL",  76,  18);
        M5.Display.setTextColor(C_CYA); M5.Display.drawString("wMOA",  116, 18);
        M5.Display.setTextColor(C_ORG); M5.Display.drawString("wMIL",  160, 18);
        M5.Display.setTextColor(C_GRD);
        M5.Display.drawString("m", 10, 25);
        M5.Display.drawFastHLine(0, 31, SW, C_GRD);

        int ry = 33;
        for (int r = 0; r < 4; r++) {
            int ci = rOff + r;
            if (ci > cardMax) break;
            float dist = (float)(ci + 1) * interval;
            if (dist > P.tr + 1) break;
            int idx = findNearest(dist);
            bool s = (ci == rSel);
            bool isZ = (fabsf(dist - (float)P.zr) < interval * 0.6f);
            if (s) M5.Display.fillRect(0, ry, SW, 20, C_SEL);
            Res* b = &R[idx];
            char t[16];

            sprintf(t, "%.0f%s", dist, isZ ? "*" : " ");
            M5.Display.setTextDatum(middle_left);
            M5.Display.setTextColor(C_ACC);
            M5.Display.setTextSize(1);
            M5.Display.drawString(t, 4, ry + 7);

            float emo = toMOA(b->drop, dist);
            sprintf(t, "%+.1f", emo);
            M5.Display.setTextColor(C_WHT);
            M5.Display.drawString(t, 36, ry + 7);

            float emi = toMIL(b->drop, dist);
            sprintf(t, "%+.1f", emi);
            M5.Display.setTextColor(C_YEL);
            M5.Display.drawString(t, 76, ry + 7);

            float wmo = toMOA(b->drift, dist);
            sprintf(t, "%+.1f", wmo);
            M5.Display.setTextColor(C_CYA);
            M5.Display.drawString(t, 116, ry + 7);

            float wmi = toMIL(b->drift, dist);
            sprintf(t, "%+.1f", wmi);
            M5.Display.setTextColor(C_ORG);
            M5.Display.drawString(t, 160, ry + 7);

            ry += 20;
        }
    }

    M5.Display.setTextDatum(bottom_center);
    M5.Display.setTextColor(C_DIM);
    M5.Display.setTextSize(1);
    M5.Display.drawString("UP/DN=Browse  L/R=View  ESC=Back", SW / 2, SH - 1);
}

// ── 4. Trajectory Graph ─────────────────────────────────────
void drawGraph() {
    M5.Display.fillScreen(C_BG);
    hdr("TRAJECTORY", "L/R=Scroll ESC=Back");
    if (!computed || RN < 2) {
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextColor(C_DIM);
        M5.Display.setTextSize(1);
        M5.Display.drawString("No data", SW / 2, SH / 2);
        return;
    }
    int gx = 30, gy = 16, gw = SW - gx - 4, gh = SH - gy - 12;
    M5.Display.drawRect(gx, gy, gw, gh, C_SURF2);
    int mxR = RN - 1;
    if (mxR < 2) return;
    int si = gOff, ei = min(si + mxR + 1, RN);
    if (ei - si < 2) return;

    float mnD = 0, mxD = 0;
    for (int i = si; i < ei; i++) {
        if (R[i].drop < mnD) mnD = R[i].drop;
        if (R[i].drop > mxD) mxD = R[i].drop;
    }
    float rd = mxD - mnD; if (rd < 1) rd = 1;
    float sx = (float)(gw - 2) / max(ei - si - 1, 1);
    float sy = (float)(gh - 4) / rd;

    float gs = 1;
    if (rd > 100)      gs = 25;
    else if (rd > 50)  gs = 10;
    else if (rd > 10)  gs = 5;
    for (float v = gs; v <= mxD; v += gs) {
        int y = gy + 2 + (int)((mxD - v) * sy);
        if (y > gy + 2 && y < gy + gh - 3)
            M5.Display.drawFastHLine(gx + 1, y, gw - 2, C_GRD);
    }
    for (float v = 0; v >= mnD; v -= gs) {
        int y = gy + 2 + (int)((mxD - v) * sy);
        if (y > gy + 2 && y < gy + gh - 3)
            M5.Display.drawFastHLine(gx + 1, y, gw - 2, C_GRD);
    }

    M5.Display.setTextDatum(middle_right);
    M5.Display.setTextColor(C_GRD);
    M5.Display.setTextSize(1);
    for (float v = 0; v >= mnD; v -= gs) {
        int y = gy + 2 + (int)((mxD - v) * sy);
        if (y > gy + 8 && y < gy + gh - 3) {
            char l[10]; sprintf(l, "%.0f", v);
            M5.Display.drawString(l, gx - 1, y);
        }
    }

    for (int i = 0; i <= 5; i++) {
        int idx = si + (int)((float)i / 5 * (ei - si - 1));
        if (idx >= si && idx < ei) {
            int x = gx + 1 + (int)((idx - si) * sx);
            M5.Display.drawFastVLine(x, gy + gh - 4, 4, C_GRD);
            M5.Display.setTextDatum(bottom_center);
            M5.Display.setTextColor(C_GRD);
            M5.Display.setTextSize(1);
            char l[10]; sprintf(l, "%.0f", R[idx].rng);
            M5.Display.drawString(l, x, gy + gh - 5);
        }
    }

    int zy = constrain(gy + 2 + (int)(mxD * sy), gy + 2, gy + gh - 3);
    M5.Display.drawFastHLine(gx + 1, zy, gw - 2, C_GRN);

    int nP = ei - si, mS = min(nP - 1, 100), step = max(1, nP / mS);
    int lx = gx + 1, ly = constrain(gy + 2 + (int)((mxD - R[si].drop) * sy), gy + 2, gy + gh - 3);
    for (int i = si + step; i < ei; i += step) {
        int px = constrain(gx + 1 + (int)((i - si) * sx), gx + 1, gx + gw - 2);
        int py = constrain(gy + 2 + (int)((mxD - R[i].drop) * sy), gy + 2, gy + gh - 3);
        M5.Display.drawLine(lx, ly, px, py, C_ACC); lx = px; ly = py;
    }
    {
        int last = ei - 1;
        int px = constrain(gx + 1 + (int)((last - si) * sx), gx + 1, gx + gw - 2);
        int py = constrain(gy + 2 + (int)((mxD - R[last].drop) * sy), gy + 2, gy + gh - 3);
        if (px != lx || py != ly) M5.Display.drawLine(lx, ly, px, py, C_ACC);
    }

    float mnW = 0, mxW = 0;
    for (int i = si; i < ei; i++) {
        if (R[i].drift < mnW) mnW = R[i].drift;
        if (R[i].drift > mxW) mxW = R[i].drift;
    }
    float wr = mxW - mnW; if (wr < 0.1f) wr = 0.1f;
    float sW = (float)(gh - 4) / wr;
    lx = gx + 1;
    ly = constrain(gy + 2 + (int)((mxW - R[si].drift) * sW), gy + 2, gy + gh - 3);
    for (int i = si + step; i < ei; i += step) {
        int px = constrain(gx + 1 + (int)((i - si) * sx), gx + 1, gx + gw - 2);
        int py = constrain(gy + 2 + (int)((mxW - R[i].drift) * sW), gy + 2, gy + gh - 3);
        M5.Display.drawLine(lx, ly, px, py, C_CYA); lx = px; ly = py;
    }
    {
        int last = ei - 1;
        int px = constrain(gx + 1 + (int)((last - si) * sx), gx + 1, gx + gw - 2);
        int py = constrain(gy + 2 + (int)((mxW - R[last].drift) * sW), gy + 2, gy + gh - 3);
        if (px != lx || py != ly) M5.Display.drawLine(lx, ly, px, py, C_CYA);
    }

    M5.Display.setTextDatum(bottom_right);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(C_ACC); M5.Display.drawString("DROP", SW - 30, SH - 1);
    M5.Display.setTextColor(C_CYA); M5.Display.drawString("WIND", SW - 4, SH - 1);
}

// ═══════════════════════════════════════════════════════════
//  INPUT
// ═══════════════════════════════════════════════════════════

void loadCartridge(int idx) {
    const Cart* c = &CART[idx];
    P.mv = c->mv; P.bc = c->bc; P.wt = c->wt;
    P.dm = c->dm; P.cartIdx = idx;
    dirty = true;
}

void adjustVal(int dir) {
    switch (eFld) {
        case  0: P.mv   = constrain(P.mv   + dir * 10,    200.0f, 1500.0f); break;
        case  1: P.zr   = constrain(P.zr   + dir * 10,    25,     1000);    break;
        case  2: P.sh   = constrain(P.sh   + dir * 0.5f,  0.5f,   10.0f);  break;
        case  3: P.bc   = constrain(P.bc   + dir * 0.005f,0.01f,  1.0f);   break;
        case  4: P.dm   = (P.dm == 1) ? 7 : 1;                             break;
        case  5: P.wt   = constrain(P.wt   + dir * 0.5f,  2.0f,   50.0f);  break;
        case  6: P.ws   = constrain(P.ws   + dir * 1,     0.0f,   30.0f);  break;
        case  7: P.wa   = fmodf(P.wa + dir * 5 + 360, 360.0f);             break;
        case  8: P.temp = constrain(P.temp + dir * 2,     -40.0f, 60.0f);  break;
        case  9: P.pr   = constrain(P.pr   + dir * 10,    800.0f, 1100.0f);break;
        case 10: P.tr   = constrain(P.tr   + dir * 25,    50.0f,  2000.0f);break;
    }
    dirty = true;
}

void startEdit() {
    editing = true;
    editLen = 0;
    editBuf[0] = '\0';
}

void commitEdit() {
    if (!editing || editLen == 0) { editing = false; return; }
    editBuf[editLen] = '\0';
    float val = atof(editBuf);
    switch (eFld) {
        case  0: P.mv   = constrain(val, 200.0f, 1500.0f);                  break;
        case  1: P.zr   = constrain((int)roundf(val), 25, 1000);            break;
        case  2: P.sh   = constrain(val, 0.5f, 10.0f);                      break;
        case  3: P.bc   = constrain(val, 0.01f, 1.0f);                      break;
        case  4: break;
        case  5: P.wt   = constrain(val, 2.0f, 50.0f);                      break;
        case  6: P.ws   = constrain(val, 0.0f, 30.0f);                      break;
        case  7: P.wa   = fmodf(fabsf(val), 360.0f);                        break;
        case  8: P.temp = constrain(val, -40.0f, 60.0f);                    break;
        case  9: P.pr   = constrain(val, 800.0f, 1100.0f);                  break;
        case 10: P.tr   = constrain(val, 50.0f, 2000.0f);                   break;
    }
    editing = false;
    editLen = 0;
    dirty = true;
}

void handleKey(uint8_t k) {
    switch (scr) {
        case 0: // Menu
            if (k == K_UP) mSel = max(0, mSel - 1);
            if (k == K_DN) mSel = min(4, mSel + 1);
            if (k == K_ENT || k == K_SP) {
                switch (mSel) {
                    case 0: scr = 1; cartLevel = 0; calSel = 0; calOff = 0; loadSel = 0; break;
                    case 1: scr = 2; eFld = 0; eCur = 0;
                            editing = false; editLen = 0; break;
                    case 2: scr = 3; rSel = 0; rOff = 0; cardView = 0; break;
                    case 3: scr = 4; gOff = 0; break;
                    case 4: P = Params(); loadCartridge(8);
                            dirty = true; break;
                }
            }
            if (k >= K_1 && k <= K_5) { mSel = k - K_1; handleKey(K_ENT); }
            break;

        case 1: // Cartridge selector
            if (cartLevel == 0) {
                if (k == K_UP) calSel = max(0, calSel - 1);
                if (k == K_DN) calSel = min(calN - 1, calSel + 1);
                if (k == K_ENT || k == K_SP) { cartLevel = 1; loadSel = 0; }
                if (k == K_ESC) scr = 0;
            } else {
                if (k == K_UP) loadSel = max(0, loadSel - 1);
                if (k == K_DN) loadSel = min(calLoadN[calSel] - 1, loadSel + 1);
                if (k == K_ENT || k == K_SP) {
                    loadCartridge(calStart[calSel] + loadSel);
                    scr = 0;
                }
                if (k == K_ESC) cartLevel = 0;
            }
            break;

        case 2: // Editor
            if (k == K_UP) { commitEdit(); eFld = max(0, eFld - 1); if (eFld < eCur) eCur = eFld; }
            if (k == K_DN) { commitEdit(); eFld = min(10, eFld + 1); if (eFld > eCur + 3) eCur = eFld - 3; }
            if (k == K_LT) { commitEdit(); adjustVal(-1); }
            if (k == K_RT) { commitEdit(); adjustVal(1); }
            if (k == K_ENT || k == K_ESC) { commitEdit(); scr = 0; }

            if (eFld != 4) {
                if (k >= K_1 && k <= K_9) {
                    if (!editing) startEdit();
                    if (editLen < 12) { editBuf[editLen++] = '1' + (k - K_1); editBuf[editLen] = '\0'; }
                }
                if (k == K_0) {
                    if (!editing) startEdit();
                    if (editLen < 12) { editBuf[editLen++] = '0'; editBuf[editLen] = '\0'; }
                }
                if (k == K_MI) {
                    if (!editing) startEdit();
                    if (editLen == 0) { editBuf[editLen++] = '-'; editBuf[editLen] = '\0'; }
                }
                if (k == K_DOT) {
                    if (!editing) startEdit();
                    if (editLen < 12) {
                        bool hasDot = false;
                        for (int i = 0; i < editLen; i++) if (editBuf[i] == '.') { hasDot = true; break; }
                        if (!hasDot) {
                            if (editLen == 0 || editBuf[editLen - 1] == '-')
                                editBuf[editLen++] = '0';
                            editBuf[editLen++] = '.';
                            editBuf[editLen] = '\0';
                        }
                    }
                }
                if (k == K_BK && editing && editLen > 0) {
                    editLen--;
                    editBuf[editLen] = '\0';
                }
            }
            break;

        case 3: // Range card
        {
            int cm = (int)(P.tr / getInterval());
            if (k == K_UP) { rSel = max(0, rSel - 1); if (rSel < rOff) rOff = rSel; }
            if (k == K_DN) { rSel = min(cm, rSel + 1); if (rSel > rOff + 3) rOff = rSel - 3; }
            if (k == K_LT || k == K_RT) cardView = 1 - cardView;
            if (k == K_ESC) scr = 0;
            break;
        }

        case 4: // Graph
            if (k == K_LT) gOff = max(0, gOff - 25);
            if (k == K_RT) gOff = min(max(0, RN - 10), gOff + 25);
            if (k == K_ESC) scr = 0;
            break;
    }
    needsDraw = true;
}

// ═══════════════════════════════════════════════════════════
//  MAIN
// ═══════════════════════════════════════════════════════════

void setup() {
    M5Cardputer.begin();
    M5.Display.setRotation(1);
    buildCalIndex();

    M5.Display.fillScreen(C_BG);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextColor(C_ACC);
    M5.Display.setTextSize(2);
    M5.Display.drawString("BALLISTIC Calc", SW / 2, SH / 2 - 20);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(C_WHT);
    M5.Display.drawString("made by lunarc3", SW / 2, SH / 2 - 4);
    M5.Display.setTextColor(C_DIM);
    M5.Display.drawString("v1.2 for Cardputer ADV", SW / 2, SH / 2 + 10);

    loadCartridge(8);
    compute(&P, R, &RN);
    computed = true; dirty = false;

    M5.Display.setTextColor(C_GRN);
    char buf[32]; sprintf(buf, "%d cartridges loaded", CART_N);
    M5.Display.drawString(buf, SW / 2, SH / 2 + 26);

    unsigned long t0 = millis();
    while (millis() - t0 < 2500) {
        M5Cardputer.update();
        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) break;
        delay(10);
    }
    scr = 0; needsDraw = true;
}

void loop() {
    M5Cardputer.update();

    uint8_t key = 0;
    if (M5Cardputer.Keyboard.isChange()) {
        if (M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            if (st.fn) {
                for (auto k : st.hid_keys) {
                    if (k == K_DN)            { key = K_DOT;            break; }
                    if (k >= K_1 && k <= K_9) { key = 0x3A + (k - K_1); break; }
                    if (k == K_0)             { key = 0x43;             break; }
                }
            }
            if (!key) {
                for (auto k : st.hid_keys) {
                    if (k == K_UP  || k == K_DN  || k == K_LT  || k == K_RT ||
                        k == K_ESC || k == K_ENT || k == K_BK  || k == K_SP ||
                        k == K_MI  || k == K_EQ  ||
                        (k >= K_1  && k <= K_0)) {
                        key = k; break;
                    }
                }
            }
        } else {
            _rk = 0;
        }
    }

    if (key) { _rk = key; _rkt = millis(); _rrt = millis(); }
    else if (_rk) {
        unsigned long n = millis();
        bool cr = (_rk == K_UP || _rk == K_DN || _rk == K_LT || _rk == K_RT || _rk == K_BK);
        if (cr && n - _rkt > 300 && n - _rrt > 80) { key = _rk; _rrt = n; }
        if (n - _rkt > 2000) _rk = 0;
    }

    if (key) handleKey(key);

    if (dirty) {
        compute(&P, R, &RN);
        computed = true; dirty = false; needsDraw = true;
    }

    if (needsDraw) {
        switch (scr) {
            case 0: drawMenu();     break;
            case 1: drawCartSel();  break;
            case 2: drawEditor();   break;
            case 3: drawRangeCard(); break;
            case 4: drawGraph();    break;
        }
        needsDraw = false;
    }

    delay(10);
}
