/*
 * ============================================================
 *  Ballistic Calculator v2.0.2 / 弹道计算器 v2.0.2
 *  M5Stack Cardputer (ESP32-S3FN8)
 *  Based on Big Ballistics v1.2 (W.J. Jurens 1983)
 *  
 *  Chinese font support: Place "chinese.ttf" on SD card root
 * ============================================================
 */
#include <M5Cardputer.h>
#include <math.h>
#include <string.h>

#define SW 240
#define SH 135

// Colors
#define C_BG    0x08C4
#define C_SURF  0x10E6
#define C_SURF2 0x1928
#define C_ACC   0xF4E0
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
#define C_HDR   0x2124  // Header gray color

// Keys
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
#define K_DOT 0x56

// ═══════════════════════════════════════════════════════════
//  Language System
// ═══════════════════════════════════════════════════════════

#define LANG_EN 0
#define LANG_CN 1
uint8_t lang = LANG_EN;

// Font support flag - set to false since efont is not available
// Chinese will use English fallback
bool cnFontAvailable = false;

// Helper function to get string based on language
// Note: Chinese characters require efont CN support in M5GFX
// Currently set to English only since efont is not available
const char* STR(const char* en, const char* cn) {
    if (lang == LANG_CN && cnFontAvailable) {
        return cn;
    }
    return en;
}

// ═══════════════════════════════════════════════════════════
//  Data Structures
// ═══════════════════════════════════════════════════════════

struct Weapon {
    const char* name;
    uint8_t category;
    float diameter;
    float mass;
    float muzzleVel;
    float formFactor;
    uint8_t dragModel;
};

struct TrajPoint {
    float time, range, altitude, angle, velocity;
};

struct PenResult {
    float deckArmor, sideArmor, armor30deg, armor55deg;
};

struct RangeTableRow {
    float range, angle, time, velocity, penetration;
};

// ═══════════════════════════════════════════════════════════
//  Drag Tables (PROGMEM)
// ═══════════════════════════════════════════════════════════

#define DRAG_TABLE_SIZE 61

static const float G1_MACH[] PROGMEM = {
    0.00,0.05,0.10,0.15,0.20,0.25,0.30,0.35,0.40,0.45,0.50,0.55,
    0.60,0.65,0.70,0.75,0.80,0.85,0.90,0.95,1.00,1.05,1.10,1.15,
    1.20,1.25,1.30,1.35,1.40,1.45,1.50,1.55,1.60,1.65,1.70,1.75,
    1.80,1.85,1.90,1.95,2.00,2.05,2.10,2.15,2.20,2.25,2.30,2.35,
    2.40,2.45,2.50,2.55,2.60,2.65,2.70,2.75,2.80,2.85,2.90,2.95,3.00
};
static const float G1_DRAG[] PROGMEM = {
    0.103240,0.100450,0.097660,0.094760,0.092050,0.089460,0.086940,0.084630,0.082620,0.080940,0.079800,0.079330,
    0.079880,0.082450,0.085020,0.090830,0.099980,0.113920,0.134110,0.160380,0.188690,0.213120,0.231030,0.243120,
    0.251050,0.255960,0.258750,0.260010,0.260160,0.259460,0.258120,0.256350,0.254230,0.251840,0.249250,0.246620,
    0.243870,0.241160,0.238450,0.235740,0.233030,0.230400,0.227920,0.225530,0.223250,0.221090,0.219010,0.217050,
    0.215240,0.213550,0.211940,0.210525,0.209110,0.207915,0.206720,0.205680,0.204640,0.203795,0.202950,0.202260,0.201570
};
static const float G7_MACH[] PROGMEM = {
    0.00,0.05,0.10,0.15,0.20,0.25,0.30,0.35,0.40,0.45,0.50,0.55,
    0.60,0.65,0.70,0.75,0.80,0.85,0.90,0.95,1.00,1.05,1.10,1.15,
    1.20,1.25,1.30,1.35,1.40,1.45,1.50,1.55,1.60,1.65,1.70,1.75,
    1.80,1.85,1.90,1.95,2.00,2.05,2.10,2.15,2.20,2.25,2.30,2.35,
    2.40,2.45,2.50,2.55,2.60,2.65,2.70,2.75,2.80,2.85,2.90,2.95,3.00
};
static const float G7_DRAG[] PROGMEM = {
    0.0470500,0.0470100,0.0469700,0.0468900,0.0468500,0.0468900,0.0468900,0.0468900,0.0468500,0.0468500,0.0468900,0.0468500,
    0.0468900,0.0470100,0.0472000,0.0477100,0.0487700,0.0512900,0.0574900,0.0806600,0.1493400,0.1587700,0.1576300,0.1553100,
    0.1525200,0.1496200,0.1465600,0.1436100,0.1405900,0.1378400,0.1350900,0.1325800,0.1301800,0.1280200,0.1260200,0.1240900,
    0.1224000,0.1208700,0.1194600,0.1182000,0.1170200,0.1158900,0.1147500,0.1135700,0.1124700,0.1113300,0.1102300,0.1091300,
    0.1080700,0.1070100,0.1059100,0.1048500,0.1037900,0.1026900,0.1016300,0.1005700,0.0994700,0.0984100,0.0973500,0.0962500,0.0951900
};
static const float G8_MACH[] PROGMEM = {
    0.00,0.05,0.10,0.15,0.20,0.25,0.30,0.35,0.40,0.45,0.50,0.55,
    0.60,0.65,0.70,0.75,0.80,0.85,0.90,0.95,1.00,1.05,1.10,1.15,
    1.20,1.25,1.30,1.35,1.40,1.45,1.50,1.55,1.60,1.65,1.70,1.75,
    1.80,1.85,1.90,1.95,2.00,2.05,2.10,2.15,2.20,2.25,2.30,2.35,
    2.40,2.45,2.50,2.55,2.60,2.65,2.70,2.75,2.80,2.85,2.90,2.95,3.00
};
static const float G8_DRAG[] PROGMEM = {
    0.0826600,0.0826600,0.0826200,0.0826200,0.0825800,0.0825800,0.0825800,0.0825800,0.0825800,0.0825500,0.0825500,0.0825500,
    0.0825500,0.0825500,0.0825800,0.0825800,0.0826200,0.0826600,0.0828200,0.1009600,0.1597500,0.1757700,0.1758100,0.1735300,
    0.1709400,0.1681900,0.1652500,0.1623000,0.1594000,0.1565300,0.1537400,0.1509900,0.1483200,0.1456900,0.1431400,0.1406300,
    0.1381900,0.1358000,0.1335200,0.1312800,0.1291200,0.1270000,0.1249600,0.1229500,0.1209900,0.1190700,0.1171400,0.1153400,
    0.1135300,0.1117200,0.1100300,0.1084200,0.1068100,0.1052800,0.1037500,0.1023150,0.1008800,0.0995100,0.0981400,0.0968200,0.0955000
};

// ═══════════════════════════════════════════════════════════
//  Weapon Database
// ═══════════════════════════════════════════════════════════

#define CAT_LIGHT    0
#define CAT_US_TANK  1
#define CAT_DE_TANK  2
#define CAT_UK_TANK  3
#define CAT_AA       4
#define CAT_NAVAL    5

static const Weapon WEAPONS[] PROGMEM = {
    {"5.56mm M193",      CAT_LIGHT,   5.69,   0.00356,  993,    1.2869, 7},
    {"7.62mm M80",       CAT_LIGHT,   7.82,   0.00966,  713.2,  1.105,  7},
    {"9mm Luger",        CAT_LIGHT,   9.00,   0.00745,  359.7,  0.120,  1},
    {"12.7mm M2",        CAT_LIGHT,  12.70,   0.04594,  856,    0.86,   7},
    {"14.5mm KPV",       CAT_LIGHT,  14.88,   0.06400, 1006,    0.95,   7},
    {"37mm M5 APC",      CAT_US_TANK, 37.0,   0.8709,  870,    0.92,   8},
    {"75mm M3 APC",      CAT_US_TANK, 76.0,   6.7585,  618.7,  0.985,  8},
    {"76mm M1 APC",      CAT_US_TANK, 76.0,   6.9853,  792.5,  1.052,  8},
    {"76mm M1 HVAP",     CAT_US_TANK, 76.2,   3.4000, 1000,    1.165,  8},
    {"90mm M1 APC",      CAT_US_TANK, 90.0,  10.9361,  853.4,  0.90,   8},
    {"90mm M1 AP",       CAT_US_TANK, 90.0,  10.9134,  853.4,  0.97,   7},
    {"90mm M1 HVAP",     CAT_US_TANK, 90.0,   7.5750, 1021.1,  1.16,   8},
    {"3.7cm KwK36",      CAT_DE_TANK, 37.0,   0.658,  745,    1.15,   8},
    {"5cm KwK39 L/50",   CAT_DE_TANK, 50.0,   2.06,   835,    1.15,   8},
    {"7.5cm KwK40",      CAT_DE_TANK, 75.0,   6.80,   750,    1.15,   8},
    {"7.5cm KwK42",      CAT_DE_TANK, 75.0,   7.20,   935,    1.15,   8},
    {"8.8cm KwK36",      CAT_DE_TANK, 88.0,  10.20,   773,    1.15,   8},
    {"8.8cm KwK43",      CAT_DE_TANK, 88.0,  10.40,  1000,    1.15,   8},
    {"12.8cm Pak44",     CAT_DE_TANK,128.0,  28.30,   950,    1.15,   8},
    {"2pdr L/52",        CAT_UK_TANK, 40.0,   1.08,   792,    1.13,   8},
    {"6pdr L/50",        CAT_UK_TANK, 57.0,   2.86,   884,    1.13,   8},
    {"17pdr APCBC",      CAT_UK_TANK, 76.2,   7.70,   900,    1.13,   8},
    {"17pdr APDS",       CAT_UK_TANK, 50.0,   3.50,  1200,    1.13,   8},
    {"37mm Flak18",      CAT_AA,      37.0,   0.62,   820,    0.925,  8},
    {"88mm Flak36",      CAT_AA,      88.0,   9.099,  819.9,  0.925,  8},
    {"105mm Flak38",     CAT_AA,     105.0,  15.10,   881,    0.925,  8},
    {"128mm Flak40",     CAT_AA,     128.0,  26.00,   880,    0.925,  8},
    {"5\"/38 127mm",     CAT_NAVAL,  127.0,  25.00,   792,    0.95,   8},
    {"6\"/47 152mm",     CAT_NAVAL,  152.0,  59.00,   762,    0.9605, 8},
    {"8\"/55 203mm",     CAT_NAVAL,  203.0, 152.00,   762,    0.9605, 8},
    {"14\"/45 356mm",    CAT_NAVAL,  356.0, 721.00,   757,    0.9605, 8},
    {"38cm SK C/34",     CAT_NAVAL,  380.0, 800.00,   820,    0.9605, 8},
    {"16\"/50 406mm",    CAT_NAVAL,  406.0,1225.00,   762,    1.03275,8},
    {"46cm Type94",      CAT_NAVAL,  460.0,1460.00,   780,    0.9605, 8}
};
#define WEAPON_COUNT (sizeof(WEAPONS) / sizeof(WEAPONS[0]))

// ═══════════════════════════════════════════════════════════
//  Global State
// ═══════════════════════════════════════════════════════════

Weapon currentWeapon = {"7.62mm M80", CAT_LIGHT, 7.82, 0.00966, 713.2, 1.105, 7};

// Ballistic parameters
float fireAngle = 0.0f, fireAlt = 0.0f, targetAlt = 0.0f, targetRange = 1000.0f;

// Atmospheric parameters (customizable)
float atmTemperature = 15.0f;    // Temperature in Celsius (standard: 15°C)
float atmPressure = 1013.25f;    // Pressure in hPa (standard: 1013.25 hPa)
float atmHumidity = 0.0f;        // Humidity % (affects air density slightly)

TrajPoint trajPoints[200];
int trajCount = 0;
PenResult penResult;
float maxAlt = 0;

RangeTableRow rangeTable[50];
int rangeTableCount = 0;

// Range Solver results
float solverAngle = 0;
float solverRange = 0;
float solverTOF = 0;
float solverVel = 0;
float solverMaxAlt = 0;
bool solverDone = false;

// Max Range results
float maxRange = 0;
float maxRangeAngle = 0;
float maxRangeTOF = 0;
float maxRangeVel = 0;
float maxRangeAlt = 0;
bool maxRangeDone = false;

// UI state
int currentScreen = 0;
int menuSel = 0, menuOff = 0;  // menu selection and scroll offset
int catSel = 0, weaponSel = 0;
int fieldSel = 0, settingsSel = 0;

char editBuf[16];
int editLen = 0;
bool editing = false;

uint8_t lastKey = 0;
unsigned long keyTime = 0, repeatTime = 0;
bool needsDraw = true, computed = false;

// ═══════════════════════════════════════════════════════════
//  Physics Engine
// ═══════════════════════════════════════════════════════════

float readFloat(const float* addr) {
    float val;
    memcpy_P(&val, addr, sizeof(float));
    return val;
}

float lerp(float x, float x0, float y0, float x1, float y1) {
    if (fabsf(x1 - x0) < 1e-10f) return y0;
    return (y0 * (x1 - x) + y1 * (x - x0)) / (x1 - x0);
}

float closestVal(float num, const float* arr, int size) {
    float minDiff = 1e10f, ans = readFloat(&arr[0]);
    for (int i = 0; i < size; i++) {
        float val = readFloat(&arr[i]);
        float diff = fabsf(num - val);
        if (diff < minDiff) { minDiff = diff; ans = val; }
    }
    return ans;
}

int findIndex(float val, const float* arr, int size) {
    for (int i = 0; i < size; i++)
        if (fabsf(readFloat(&arr[i]) - val) < 0.001f) return i;
    return 0;
}

// High-precision atmospheric model using double
// Supports custom temperature, pressure, and humidity
double atmDensity1976_hp(double alt) {
    const double REARTH = 6369000.0;
    const double GMR = 34.163195;
    const double htab[] = {0,11,20,32,47,51,71,84.852};
    const double ttab[] = {288.15,216.65,216.65,228.65,270.65,270.65,214.65,186.946};
    const double ptab[] = {1.0,2.233611e-1,5.403295e-2,8.5666784e-3,1.0945601e-3,6.6063531e-4,3.9046834e-5,3.68501e-6};
    const double gtab[] = {-6.5,0,1.0,2.8,0,-2.8,-2.0,0};
    
    // Calculate base density using 1976 Standard Atmosphere
    double h = (alt/1000.0) * REARTH / ((alt/1000.0) + REARTH);
    int i = 0, j = 8;
    while (j > i + 1) { int k = (i+j)/2; if (h < htab[k]) j=k; else i=k; }
    
    double tlocal = ttab[i] + gtab[i] * (h - htab[i]);
    double theta = tlocal / ttab[0];
    double delta = (gtab[i]==0) ? ptab[i]*exp(-GMR*(h-htab[i])/ttab[i]) : ptab[i]*pow(ttab[i]/tlocal, GMR/gtab[i]);
    
    double baseDensity = 1.225 * delta / theta;  // Standard density at this altitude
    
    // Apply custom atmospheric conditions using Tetens formula
    // This gives more accurate results for non-standard conditions
    double customDensity = getCurrentAirDensity();
    
    // Scale the base density by the ratio of custom to standard density
    // Standard conditions: 15°C, 1013.25 hPa, 0% humidity
    double standardDensity = calcAirDensity(15.0, 1013.25, 0.0);
    double ratio = customDensity / standardDensity;
    
    return baseDensity * ratio;
}

// Air density calculation using Tetens formula (with humidity support)
// Inputs: Temperature (°C), Pressure (hPa), Humidity (%)
// Output: Air density (kg/m³)
double calcAirDensity(double T_c, double P_hPa, double RH) {
    const double R_d = 287.058;   // Gas constant for dry air (J/(kg·K))
    const double R_v = 461.495;   // Gas constant for water vapor (J/(kg·K))
    
    double T_k = T_c + 273.15;   // Convert to Kelvin
    
    // Tetens formula for Saturation Vapor Pressure (hPa)
    double P_sat = 6.1078 * pow(10.0, (7.5 * T_c) / (T_c + 237.3));
    
    // Actual Vapor Pressure (hPa)
    double P_v = P_sat * (RH / 100.0);
    
    // Dry Air Pressure (hPa)
    double P_d = P_hPa - P_v;
    
    // Convert hPa to Pascals
    double P_v_Pa = P_v * 100.0;
    double P_d_Pa = P_d * 100.0;
    
    // Total density (moist air)
    double rho = (P_d_Pa / (R_d * T_k)) + (P_v_Pa / (R_v * T_k));
    
    return rho;
}

// Get current air density based on user settings
double getCurrentAirDensity() {
    return calcAirDensity((double)atmTemperature, (double)atmPressure, (double)atmHumidity);
}

// High-precision speed of sound using double
double soundSpeed1976_hp(double alt) {
    const double REARTH = 6369000.0;
    const double htab[] = {0,11,20,32,47,51,71,84.852};
    const double ttab[] = {288.15,216.65,216.65,228.65,270.65,270.65,214.65,186.946};
    const double gtab[] = {-6.5,0,1.0,2.8,0,-2.8,-2.0,0};
    
    double h = (alt/1000.0) * REARTH / ((alt/1000.0) + REARTH);
    int i = 0, j = 8;
    while (j > i + 1) { int k = (i+j)/2; if (h < htab[k]) j=k; else i=k; }
    return sqrt(1.4 * 287.058 * (ttab[i] + gtab[i]*(h-htab[i])));  // R = 287.058 J/(kg·K) for dry air
}

// High-precision gravity using double
double gravity1976_hp(double alt) {
    return 9.80665 * pow(1.0 + alt/6356766.0, -2.0);
}

// Keep float versions for backward compatibility (used by other functions)
float atmDensity1976(float alt) { return (float)atmDensity1976_hp((double)alt); }
float soundSpeed1976(float alt) { return (float)soundSpeed1976_hp((double)alt); }
float gravity1976(float alt) { return (float)gravity1976_hp((double)alt); }

float getDragCoeff(float mach, uint8_t model) {
    const float *mArr, *dArr;
    switch (model) {
        case 1: mArr=G1_MACH; dArr=G1_DRAG; break;
        case 7: mArr=G7_MACH; dArr=G7_DRAG; break;
        default: mArr=G8_MACH; dArr=G8_DRAG; break;
    }
    float cl = closestVal(mach, mArr, DRAG_TABLE_SIZE);
    int idx = findIndex(cl, mArr, DRAG_TABLE_SIZE);
    float cd = readFloat(&dArr[idx]);
    if (idx+1 < DRAG_TABLE_SIZE)
        cd = lerp(mach, cl, cd, readFloat(&mArr[idx+1]), readFloat(&dArr[idx+1]));
    return cd;
}

void computeBallistic() {
    // Use double precision for critical calculations
    double d_cm = (double)currentWeapon.diameter / 10.0;
    double m_kg = (double)currentWeapon.mass;
    double ff = (double)currentWeapon.formFactor;
    double v0 = (double)currentWeapon.muzzleVel;
    double angleRad = (double)fireAngle * M_PI / 180.0;
    double bc = m_kg / (ff * d_cm * d_cm);
    double interval = 50.0 / v0;  // Adaptive interval based on velocity
    
    double tt=0, rg=0, a0=(double)fireAlt, ld=angleRad;
    trajCount = 0; maxAlt = 0;
    
    while (a0 >= 0 && trajCount < 200) {
        // High-precision atmospheric calculations
        double cs = soundSpeed1976_hp(a0);
        double d0 = atmDensity1976_hp(a0);
        double g = gravity1976_hp(a0);
        
        // Velocity components
        double x0 = v0*cos(ld), y0 = v0*sin(ld);
        
        // Mach number and drag coefficient (from float lookup table)
        double kk = (cs>0) ? v0/cs : 0;
        double kd = (double)getDragCoeff((float)kk, currentWeapon.dragModel);
        
        // Drag force and retardation
        double r0 = kd * 0.0001 * v0*v0;  // kd * (air_density_ref/10000) * v^2
        double e0 = (d0>0) ? r0/(bc/d0) : 0;
        
        // First prediction
        double h0=e0*cos(ld), j0=e0*sin(ld)+g;
        double x1=x0-h0*interval, y1=y0-j0*interval;
        double v1=sqrt(x1*x1+y1*y1), l1=atan2(y1,x1);
        
        // Second prediction (corrected)
        double m1=(y0+y1)/2, a1_new=m1*interval, y=a1_new+a0;
        double d1=atmDensity1976_hp(y);
        kk=v1/cs; kd=(double)getDragCoeff((float)kk,currentWeapon.dragModel);
        double r1=kd*0.0001*v1*v1, e1=(d1>0)?r1/(bc/d1):0;
        double h1=e1*cos(l1), j1=e1*sin(l1)+g;
        double h2=(h0+h1)/2, j2=(j0+j1)/2;
        
        // Third prediction (final)
        double x2=x0-h2*interval, y2v=y0-j2*interval;
        double v2=sqrt(x2*x2+y2v*y2v), l2=atan2(y2v,x2);
        
        double m2=(y0+y2v)/2, a2=m2*interval, y2=a2+a0;
        double d2=atmDensity1976_hp(y2);
        kk=v2/cs; kd=(double)getDragCoeff((float)kk,currentWeapon.dragModel);
        double r2=kd*0.0001*v2*v2, e2=(d2>0)?r2/(bc/d2):0;
        double h3=e2*cos(l2), j3=e2*sin(l2)+g;
        double h4=(h0+h3)/2, j4=(j0+j3)/2;
        
        // Final velocity and position update
        double x3=x0-h4*interval, y3=y0-j4*interval;
        double v3=sqrt(x3*x3+y3*y3);
        ld=atan2(y3,x3);
        
        double m3=(y0+y3)/2, m4=(x0+x3)/2;
        v0=v3; a0+=m3*interval; rg+=m4*interval; tt+=interval;
        
        // Store results (convert to float for storage)
        trajPoints[trajCount++] = {(float)tt, (float)rg, (float)a0, (float)(ld*180.0/M_PI), (float)v0};
        if (a0 > maxAlt) maxAlt = (float)a0;
    }
    computed = true;
    
    // Calculate penetration with high precision
    if (trajCount > 0) {
        int last = trajCount-1;
        float vel = trajPoints[last].velocity;
        float ang = fabsf(trajPoints[last].angle);
        penResult.sideArmor = thompsonPen(currentWeapon.diameter, m_kg, vel, ang);
        penResult.deckArmor = thompsonPen(currentWeapon.diameter, m_kg, vel, 90-ang);
        penResult.armor30deg = thompsonPen(currentWeapon.diameter, m_kg, vel, 30);
        penResult.armor55deg = thompsonPen(currentWeapon.diameter, m_kg, vel, 55);
    }
}

float thompsonPen(float d_mm, float m_kg, float v_ms, float angleDeg) {
    // Use double precision for penetration calculation
    double d_m = (double)d_mm / 1000.0;
    double angleRad = (double)angleDeg * M_PI / 180.0;
    double ke = 0.5 * (double)m_kg * (double)v_ms * (double)v_ms;
    double armor = 0.00025;
    
    for (int i=0; i<10000; i++) {
        double fc = 1.8288 * ((armor/d_m) - 0.45) * ((double)angleDeg*(double)angleDeg + 2000.0) + 12192.0;
        double cosAngle = cos(angleRad);
        double ken = 8.025 * (armor * d_m * d_m * fc * fc / (cosAngle * cosAngle));
        if (ken > ke) break;
        armor += 0.00025;
    }
    return (float)(armor * 1000.0);
}

void generateRangeTable() {
    rangeTableCount = 0;
    for (float r = 100; r <= targetRange && rangeTableCount < 50; r += 100) {
        float angle = -3, bRange = 0;
        // Faster iteration with larger steps initially
        for (int iter=0; iter<2000 && bRange<r; iter++) {
            angle += 0.05f;  // Larger step for speed
            float oldAngle = fireAngle, oldTarget = targetRange;
            fireAngle = angle; targetRange = r;
            computeBallistic();
            if (trajCount > 0) bRange = trajPoints[trajCount-1].range;
            fireAngle = oldAngle; targetRange = oldTarget;
        }
        // Fine-tune with smaller steps
        angle -= 0.05f;
        for (int iter=0; iter<200 && bRange<r; iter++) {
            angle += 0.005f;
            float oldAngle = fireAngle, oldTarget = targetRange;
            fireAngle = angle; targetRange = r;
            computeBallistic();
            if (trajCount > 0) bRange = trajPoints[trajCount-1].range;
            fireAngle = oldAngle; targetRange = oldTarget;
        }
        
        if (trajCount > 0) {
            int last = trajCount-1;
            rangeTable[rangeTableCount++] = {r, angle, trajPoints[last].time, trajPoints[last].velocity, penResult.sideArmor};
        }
    }
    computeBallistic();
}

// Range Solver: Given target range, find required firing angle
void solveRange() {
    solverDone = false;
    float targetR = targetRange;
    float angle = -3.0;
    float bRange = 0;
    
    // Coarse search with larger steps
    for (int iter = 0; iter < 3000 && bRange < targetR; iter++) {
        angle += 0.05f;
        float oldAngle = fireAngle;
        fireAngle = angle;
        computeBallistic();
        if (trajCount > 0) bRange = trajPoints[trajCount-1].range;
        fireAngle = oldAngle;
    }
    
    // Fine-tune with smaller steps
    angle -= 0.05f;
    for (int iter = 0; iter < 500 && bRange < targetR; iter++) {
        angle += 0.001f;
        float oldAngle = fireAngle;
        fireAngle = angle;
        computeBallistic();
        if (trajCount > 0) bRange = trajPoints[trajCount-1].range;
        fireAngle = oldAngle;
    }
    
    // Final result
    if (trajCount > 0) {
        int last = trajCount-1;
        solverAngle = angle;
        solverRange = bRange;
        solverTOF = trajPoints[last].time;
        solverVel = trajPoints[last].velocity;
        solverMaxAlt = maxAlt;
        solverDone = true;
    }
    
    // Restore original computation
    computeBallistic();
}

// Max Range: Find maximum range at optimal angle (around 45 degrees)
void findMaxRange() {
    maxRangeDone = false;
    float bestRange = 0;
    float bestAngle = 0;
    
    // Search around 45 degrees (optimal for max range)
    for (float angle = 30.0f; angle <= 55.0f; angle += 0.5f) {
        float oldAngle = fireAngle;
        fireAngle = angle;
        computeBallistic();
        
        if (trajCount > 0) {
            float r = trajPoints[trajCount-1].range;
            if (r > bestRange) {
                bestRange = r;
                bestAngle = angle;
            }
        }
        fireAngle = oldAngle;
    }
    
    // Fine-tune around the best angle
    float fineStart = bestAngle - 1.0f;
    float fineEnd = bestAngle + 1.0f;
    for (float angle = fineStart; angle <= fineEnd; angle += 0.1f) {
        float oldAngle = fireAngle;
        fireAngle = angle;
        computeBallistic();
        
        if (trajCount > 0) {
            float r = trajPoints[trajCount-1].range;
            if (r > bestRange) {
                bestRange = r;
                bestAngle = angle;
            }
        }
        fireAngle = oldAngle;
    }
    
    // Final computation with best angle
    float oldAngle = fireAngle;
    fireAngle = bestAngle;
    computeBallistic();
    
    if (trajCount > 0) {
        int last = trajCount-1;
        maxRange = bestRange;
        maxRangeAngle = bestAngle;
        maxRangeTOF = trajPoints[last].time;
        maxRangeVel = trajPoints[last].velocity;
        maxRangeAlt = maxAlt;
        maxRangeDone = true;
    }
    
    fireAngle = oldAngle;
    computeBallistic();
}

// ═══════════════════════════════════════════════════════════
//  UI Drawing
// ═══════════════════════════════════════════════════════════

// Fixed header - solid gray, 14px tall
void drawHeader(const char* title, const char* right = "") {
    M5.Display.fillRect(0, 0, SW, 14, C_HDR);
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextColor(C_ACC);
    M5.Display.setTextSize(1);
    M5.Display.drawString(title, 4, 7);
    if (strlen(right) > 0) {
        M5.Display.setTextDatum(middle_right);
        M5.Display.setTextColor(C_DIM);
        M5.Display.drawString(right, SW-4, 7);
    }
}

// Category names
const char* getCatName(int idx) {
    switch(idx) {
        case 0: return STR("Light Arms", "轻武器");
        case 1: return STR("US Tanks", "美系坦克");
        case 2: return STR("German Tanks", "德系坦克");
        case 3: return STR("UK Tanks", "英系坦克");
        case 4: return STR("AA Guns", "高射炮");
        case 5: return STR("Naval Guns", "舰炮");
        default: return "";
    }
}

void drawMainMenu(bool fullRedraw = true) {
    if (fullRedraw) {
        M5.Display.fillScreen(C_BG);
        // Use English for header to ensure it displays
        if (lang == LANG_CN && cnFontAvailable) {
            drawHeader("弹道计算器 v2.0.2", "中文");
        } else {
            drawHeader("Ballistic Calc v2.0.2", "EN");
        }
    } else {
        // Only clear menu area, not header
        M5.Display.fillRect(0, 14, SW, SH-14, C_BG);
    }
    
    const char* items[10];
    if (lang == LANG_CN && cnFontAvailable) {
        items[0] = "1. 选择武器";
        items[1] = "2. 编辑参数";
        items[2] = "3. 弹道轨迹";
        items[3] = "4. 射程表";
        items[4] = "5. 距离求解";
        items[5] = "6. 最大射程";
        items[6] = "7. 穿甲分析";
        items[7] = "8. 防空有效范围";
        items[8] = "9. 设置";
        items[9] = "0. 重置默认";
    } else {
        items[0] = "1. Select Weapon";
        items[1] = "2. Edit Parameters";
        items[2] = "3. Trajectory";
        items[3] = "4. Range Table";
        items[4] = "5. Range Solver";
        items[5] = "6. Max Range";
        items[6] = "7. Penetration";
        items[7] = "8. AA Envelope";
        items[8] = "9. Settings";
        items[9] = "0. Reset";
    }
    
    // Calculate visible range (6 items visible at a time - leaves room for status bar)
    int visCount = 6;
    if (menuSel < menuOff) menuOff = menuSel;
    if (menuSel >= menuOff + visCount) menuOff = menuSel - visCount + 1;
    
    int y = 16;
    for (int i = menuOff; i < 10 && i < menuOff + visCount; i++) {
        bool sel = (i == menuSel);
        if (sel) M5.Display.fillRect(4, y, SW-8, 14, C_SEL);
        M5.Display.setTextDatum(middle_left);
        M5.Display.setTextSize(1);
        M5.Display.setTextColor(sel ? C_ACC : C_DIM);
        M5.Display.drawString(items[i], 8, y+7);
        y += 14;
    }
    
    // Status bar at bottom - fixed position
    int statusY = 102;  // Fixed position for status bar
    M5.Display.drawFastHLine(0, statusY, SW, C_SURF2);
    
    char buf[40];
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextSize(1);
    
    // Weapon name
    M5.Display.setTextColor(C_WHT);
    snprintf(buf, sizeof(buf), "Weapon: %s", currentWeapon.name);
    M5.Display.drawString(buf, 4, statusY + 8);
    
    // Weapon specs - larger and more visible
    M5.Display.setTextColor(C_ACC);
    snprintf(buf, sizeof(buf), "%.0fmm  %.1fkg  V:%.0fm/s", currentWeapon.diameter, currentWeapon.mass, currentWeapon.muzzleVel);
    M5.Display.drawString(buf, 4, statusY + 20);
    
    // Scroll indicator
    if (10 > visCount) {
        int barH = visCount * 14 * visCount / 10;
        int barY = 16 + (menuOff * 14 * (visCount*14 - barH) / ((10-visCount)*14));
        M5.Display.fillRect(SW-3, barY, 2, barH, C_SURF2);
    }
}

void drawCategorySelect(bool fullRedraw = true) {
    if (fullRedraw) {
        M5.Display.fillScreen(C_BG);
        drawHeader("Weapon Type / 武器分类", "ESC");
    } else {
        M5.Display.fillRect(0, 14, SW, SH-14, C_BG);
    }
    
    int count[6] = {0};
    for (int i=0; i<WEAPON_COUNT; i++) {
        Weapon w; memcpy_P(&w, &WEAPONS[i], sizeof(Weapon));
        count[w.category]++;
    }
    
    int y = 16;
    for (int i=0; i<6; i++) {
        bool sel = (i == catSel);
        if (sel) M5.Display.fillRect(4, y, SW-8, 14, C_SEL);
        M5.Display.setTextDatum(middle_left);
        M5.Display.setTextSize(1);
        M5.Display.setTextColor(sel ? C_WHT : C_DIM);
        
        // Show both English and Chinese category names
        const char* catNamesEN[] = {"Light Arms", "US Tanks", "German Tanks", "UK Tanks", "AA Guns", "Naval Guns"};
        const char* catNamesCN[] = {"轻武器", "美系坦克", "德系坦克", "英系坦克", "高射炮", "舰炮"};
        
        char buf[40];
        if (lang == LANG_CN && cnFontAvailable) {
            snprintf(buf, sizeof(buf), "[%d] %s (%d)", i+1, catNamesCN[i], count[i]);
        } else {
            snprintf(buf, sizeof(buf), "[%d] %s (%d)", i+1, catNamesEN[i], count[i]);
        }
        M5.Display.drawString(buf, 8, y+7);
        y += 14;
    }
}

void drawWeaponSelect(bool fullRedraw = true) {
    if (fullRedraw) {
        M5.Display.fillScreen(C_BG);
        // Use category name based on language
        const char* catNamesEN[] = {"Light Arms", "US Tanks", "German Tanks", "UK Tanks", "AA Guns", "Naval Guns"};
        const char* catNamesCN[] = {"轻武器", "美系坦克", "德系坦克", "英系坦克", "高射炮", "舰炮"};
        const char* title = (lang == LANG_CN && cnFontAvailable) ? catNamesCN[catSel] : catNamesEN[catSel];
        drawHeader(title, "ESC");
    } else {
        M5.Display.fillRect(0, 14, SW, SH-14, C_BG);
    }
    
    int startIdx = -1, count = 0;
    for (int i=0; i<WEAPON_COUNT; i++) {
        Weapon w; memcpy_P(&w, &WEAPONS[i], sizeof(Weapon));
        if (w.category == catSel) { if (startIdx<0) startIdx=i; count++; }
    }
    
    int y = 16, shown = 0;
    for (int i=startIdx; i<WEAPON_COUNT && shown<7; i++) {
        Weapon w; memcpy_P(&w, &WEAPONS[i], sizeof(Weapon));
        if (w.category != catSel) continue;
        
        bool sel = (shown == weaponSel);
        if (sel) M5.Display.fillRect(4, y, SW-8, 14, C_SEL);
        M5.Display.setTextDatum(middle_left);
        M5.Display.setTextSize(1);
        M5.Display.setTextColor(sel ? C_ACC : C_WHT);
        M5.Display.drawString(w.name, 8, y+7);
        
        M5.Display.setTextDatum(middle_right);
        M5.Display.setTextColor(sel ? C_WHT : C_DIM);
        char buf[16]; snprintf(buf, sizeof(buf), "%.0fm/s", w.muzzleVel);
        M5.Display.drawString(buf, SW-8, y+7);
        y += 14; shown++;
    }
}

void drawEditor(bool fullRedraw = true) {
    if (fullRedraw) {
        M5.Display.fillScreen(C_BG);
        drawHeader(STR("Edit Parameters", "编辑参数"), STR("ESC", "返回"));
    } else {
        M5.Display.fillRect(0, 14, SW, SH-14, C_BG);
    }
    
    const char* labels[6];
    if (lang == LANG_CN && cnFontAvailable) {
        labels[0] = "发射角 (度)";
        labels[1] = "发射高度 (m)";
        labels[2] = "目标高度 (m)";
        labels[3] = "目标射程 (m)";
        labels[4] = "温度 (C)";
        labels[5] = "气压 (hPa)";
    } else {
        labels[0] = "Fire Angle (deg)";
        labels[1] = "Fire Altitude (m)";
        labels[2] = "Target Altitude (m)";
        labels[3] = "Target Range (m)";
        labels[4] = "Temperature (C)";
        labels[5] = "Pressure (hPa)";
    }
    float values[] = {fireAngle, fireAlt, targetAlt, targetRange, atmTemperature, atmPressure};
    
    int y = 16;
    for (int i=0; i<6; i++) {
        bool sel = (i == fieldSel);
        if (sel) { M5.Display.fillRect(0, y, SW, 18, C_SEL); M5.Display.fillRect(0, y, 3, 18, C_ACC); }
        
        M5.Display.setTextDatum(middle_left);
        M5.Display.setTextSize(1);
        M5.Display.setTextColor(sel ? C_ACC : C_DIM);
        M5.Display.drawString(labels[i], 6, y+9);
        
        M5.Display.setTextDatum(middle_right);
        if (editing && sel) {
            char disp[20];
            bool showCur = (millis()/500)%2==0;
            snprintf(disp, sizeof(disp), "%s%s", editBuf, showCur ? "_" : " ");
            M5.Display.setTextColor(C_YEL);
            M5.Display.drawString(disp, SW-8, y+9);
        } else {
            char v[16];
            // Dynamic precision based on field type
            if (i == 4) {  // Temperature
                snprintf(v, sizeof(v), "%.1f", values[i]);
            } else if (i == 5) {  // Pressure
                snprintf(v, sizeof(v), "%.1f", values[i]);
            } else {
                snprintf(v, sizeof(v), "%.2f", values[i]);
            }
            M5.Display.setTextColor(sel ? C_WHT : C_DIM);
            M5.Display.drawString(v, SW-8, y+9);
        }
        y += 18;
    }
    
    // Weapon info
    M5.Display.drawFastHLine(0, y+2, SW, C_SURF2);
    char buf[40];
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextColor(C_DIM);
    if (lang == LANG_CN && cnFontAvailable) {
        snprintf(buf, sizeof(buf), "武器: %s", currentWeapon.name);
    } else {
        snprintf(buf, sizeof(buf), "Weapon: %s", currentWeapon.name);
    }
    M5.Display.drawString(buf, 4, y+10);
    snprintf(buf, sizeof(buf), "%.0fmm %.2fkg FF:%.3f", currentWeapon.diameter, currentWeapon.mass, currentWeapon.formFactor);
    M5.Display.drawString(buf, 4, y+20);
    
    // Show atmospheric conditions
    snprintf(buf, sizeof(buf), "Atm: %.0fC %.0fhPa", atmTemperature, atmPressure);
    M5.Display.drawString(buf, 4, y+30);
}

void drawTrajectory(bool fullRedraw = true) {
    if (fullRedraw) {
        M5.Display.fillScreen(C_BG);
        drawHeader(STR("Trajectory", "弹道轨迹"), STR("ESC", "返回"));
    } else {
        M5.Display.fillRect(0, 14, SW, SH-14, C_BG);
    }
    
    if (!computed || trajCount==0) {
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextColor(C_DIM);
        M5.Display.drawString(STR("No data", "无数据"), SW/2, SH/2);
        return;
    }
    
    int last = trajCount-1;
    char buf[32];
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextSize(1);
    
    M5.Display.setTextColor(C_ACC);
    M5.Display.drawString(lang == LANG_CN && cnFontAvailable ? "落点数据:" : "Impact Data:", 4, 18);
    
    M5.Display.setTextColor(C_WHT);
    if (lang == LANG_CN && cnFontAvailable) {
        snprintf(buf, sizeof(buf), "射程: %.1f m", trajPoints[last].range);
        M5.Display.drawString(buf, 4, 30);
        snprintf(buf, sizeof(buf), "飞行: %.2f 秒", trajPoints[last].time);
        M5.Display.drawString(buf, 4, 42);
        snprintf(buf, sizeof(buf), "着速: %.1f m/s", trajPoints[last].velocity);
        M5.Display.drawString(buf, 4, 54);
        snprintf(buf, sizeof(buf), "着角: %.2f deg", trajPoints[last].angle);
        M5.Display.drawString(buf, 4, 66);
        snprintf(buf, sizeof(buf), "最大弹道高: %.1f m", maxAlt);
        M5.Display.drawString(buf, 4, 78);
        double ke_cn = 0.5 * (double)currentWeapon.mass * (double)trajPoints[last].velocity * (double)trajPoints[last].velocity;
        M5.Display.setTextColor(C_YEL);
        snprintf(buf, sizeof(buf), "动能: %.0f J", ke_cn);
        M5.Display.drawString(buf, 4, 90);
        M5.Display.setTextColor(C_ORG);
        snprintf(buf, sizeof(buf), "穿甲: 侧:%.0f 甲板:%.0f mm", penResult.sideArmor, penResult.deckArmor);
        M5.Display.drawString(buf, 4, 102);
    } else {
        snprintf(buf, sizeof(buf), "Range: %.1f m", trajPoints[last].range);
        M5.Display.drawString(buf, 4, 30);
        snprintf(buf, sizeof(buf), "TOF: %.2f sec", trajPoints[last].time);
        M5.Display.drawString(buf, 4, 42);
        snprintf(buf, sizeof(buf), "Vel: %.1f m/s", trajPoints[last].velocity);
        M5.Display.drawString(buf, 4, 54);
        snprintf(buf, sizeof(buf), "Angle: %.2f deg", trajPoints[last].angle);
        M5.Display.drawString(buf, 4, 66);
        snprintf(buf, sizeof(buf), "Max Alt: %.1f m", maxAlt);
        M5.Display.drawString(buf, 4, 78);
        double ke_en = 0.5 * (double)currentWeapon.mass * (double)trajPoints[last].velocity * (double)trajPoints[last].velocity;
        M5.Display.setTextColor(C_YEL);
        snprintf(buf, sizeof(buf), "KE: %.0f J", ke_en);
        M5.Display.drawString(buf, 4, 90);
        M5.Display.setTextColor(C_ORG);
        snprintf(buf, sizeof(buf), "Pen: S:%.0f D:%.0f mm", penResult.sideArmor, penResult.deckArmor);
        M5.Display.drawString(buf, 4, 102);
    }
    
    // Trajectory curve
    int gx=150, gy=16, gw=85, gh=105;
    M5.Display.drawRect(gx, gy, gw, gh, C_SURF2);
    if (trajCount > 1) {
        float maxR = trajPoints[last].range, maxA = maxAlt;
        if (maxA<1) maxA=1;
        int step = max(1, trajCount/20);
        int px=gx+1, py=gy+gh-1-(int)(trajPoints[0].altitude/maxA*(gh-2));
        for (int i=step; i<trajCount; i+=step) {
            int nx = constrain(gx+1+(int)(trajPoints[i].range/maxR*(gw-2)), gx+1, gx+gw-2);
            int ny = constrain(gy+gh-1-(int)(trajPoints[i].altitude/maxA*(gh-2)), gy+1, gy+gh-2);
            M5.Display.drawLine(px, py, nx, ny, C_ACC);
            px=nx; py=ny;
        }
    }
}

void drawRangeTable(bool fullRedraw = true) {
    if (fullRedraw) {
        M5.Display.fillScreen(C_BG);
        drawHeader(STR("Range Table", "射程表"), STR("ESC", "返回"));
    } else {
        M5.Display.fillRect(0, 14, SW, SH-14, C_BG);
    }
    
    if (rangeTableCount==0) {
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextColor(C_DIM);
        M5.Display.drawString(STR("No data", "无数据"), SW/2, SH/2);
        return;
    }
    
    // Headers with units
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextSize(1);
    // Column headers: Range(m), Angle(deg), Time(sec), Velocity(m/s), Penetration(mm)
    M5.Display.setTextColor(C_ACC); M5.Display.drawString("RNG(m)", 4, 18);
    M5.Display.setTextColor(C_WHT); M5.Display.drawString("ANG(d)", 52, 18);
    M5.Display.setTextColor(C_CYA); M5.Display.drawString("TOF(s)", 98, 18);
    M5.Display.setTextColor(C_ORG); M5.Display.drawString("VEL(m/s)", 145, 18);
    M5.Display.setTextColor(C_YEL); M5.Display.drawString("PEN(mm)", 200, 18);
    M5.Display.drawFastHLine(0, 25, SW, C_SURF2);
    
    int y = 28;
    int startRow = max(0, rangeTableCount - 8);
    for (int i=startRow; i<rangeTableCount && y<SH-5; i++) {
        char buf[20];
        M5.Display.setTextDatum(middle_left);
        
        float range = rangeTable[i].range;
        float angle = rangeTable[i].angle;
        float tof = rangeTable[i].time;
        
        // Range display - always integer for clean look
        M5.Display.setTextColor(C_ACC);
        snprintf(buf, sizeof(buf), "%.0f", range);
        M5.Display.drawString(buf, 4, y);
        
        // Angle display - always 3 decimal places for precision
        M5.Display.setTextColor(C_WHT);
        snprintf(buf, sizeof(buf), "%.3f", angle);
        M5.Display.drawString(buf, 52, y);
        
        // Time display - always 3 decimal places for precision
        M5.Display.setTextColor(C_CYA);
        snprintf(buf, sizeof(buf), "%.3f", tof);
        M5.Display.drawString(buf, 98, y);
        
        // Velocity display
        M5.Display.setTextColor(C_ORG);
        // Velocity display - integer for clean look
        M5.Display.setTextColor(C_ORG);
        snprintf(buf, sizeof(buf), "%.0f", rangeTable[i].velocity);
        M5.Display.drawString(buf, 145, y);
        
        // Penetration display - integer for clean look
        M5.Display.setTextColor(C_YEL);
        snprintf(buf, sizeof(buf), "%.0f", rangeTable[i].penetration);
        M5.Display.drawString(buf, 200, y);
        y += 12;
    }
}

void drawPenetration(bool fullRedraw = true) {
    if (fullRedraw) {
        M5.Display.fillScreen(C_BG);
        drawHeader(STR("Penetration", "穿甲分析"), STR("ESC", "返回"));
    } else {
        M5.Display.fillRect(0, 14, SW, SH-14, C_BG);
    }
    
    if (!computed || trajCount==0) {
        M5.Display.setTextDatum(middle_center);
        M5.Display.setTextColor(C_DIM);
        M5.Display.drawString(STR("No data", "无数据"), SW/2, SH/2);
        return;
    }
    
    char buf[32];
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextSize(1);
    
    M5.Display.setTextColor(C_ACC);
    if (lang == LANG_CN && cnFontAvailable) {
        M5.Display.drawString("Thompson F公式:", 4, 18);
        M5.Display.setTextColor(C_DIM);
        M5.Display.drawString("(USN Class B装甲)", 4, 28);
    } else {
        M5.Display.drawString("Thompson F Formula:", 4, 18);
        M5.Display.setTextColor(C_DIM);
        M5.Display.drawString("(USN Class B Armor)", 4, 28);
    }
    M5.Display.drawFastHLine(0, 36, SW, C_SURF2);
    
    int y = 40;
    M5.Display.setTextColor(C_WHT);
    if (lang == LANG_CN && cnFontAvailable) {
        snprintf(buf, sizeof(buf), "Side (0 deg): %.1f mm", penResult.sideArmor);
        M5.Display.drawString(buf, 4, y); y += 14;
        snprintf(buf, sizeof(buf), "Deck (90 deg): %.1f mm", penResult.deckArmor);
        M5.Display.drawString(buf, 4, y); y += 14;
        snprintf(buf, sizeof(buf), "30 deg Slope: %.1f mm", penResult.armor30deg);
        M5.Display.drawString(buf, 4, y); y += 14;
        snprintf(buf, sizeof(buf), "55 deg Slope: %.1f mm", penResult.armor55deg);
        M5.Display.drawString(buf, 4, y); y += 20;
    } else {
        snprintf(buf, sizeof(buf), "Side (0 deg): %.1f mm", penResult.sideArmor);
        M5.Display.drawString(buf, 4, y); y += 14;
        snprintf(buf, sizeof(buf), "Deck (90 deg): %.1f mm", penResult.deckArmor);
        M5.Display.drawString(buf, 4, y); y += 14;
        snprintf(buf, sizeof(buf), "30 deg Slope: %.1f mm", penResult.armor30deg);
        M5.Display.drawString(buf, 4, y); y += 14;
        snprintf(buf, sizeof(buf), "55 deg Slope: %.1f mm", penResult.armor55deg);
        M5.Display.drawString(buf, 4, y); y += 20;
    }
    
    float vel = trajPoints[trajCount-1].velocity;
    double ke_pen = 0.5 * (double)currentWeapon.mass * (double)vel * (double)vel;
    M5.Display.setTextColor(C_YEL);
    snprintf(buf, sizeof(buf), STR("KE: %.0f J (%.2f MJ)", "动能: %.0f J (%.2f MJ)"), ke_pen, ke_pen/1e6);
    M5.Display.drawString(buf, 4, y);
}

void drawAAEnvelope(bool fullRedraw = true) {
    if (fullRedraw) {
        M5.Display.fillScreen(C_BG);
        drawHeader(STR("AA Envelope", "防空包络"), STR("ESC", "返回"));
    } else {
        M5.Display.fillRect(0, 14, SW, SH-14, C_BG);
    }
    
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(C_DIM);
    M5.Display.drawString(lang == LANG_CN && cnFontAvailable ? "计算中..." : "Calculating...", 4, 18);
    
    float angles[] = {87.5f,80,70,60,50,40,30,20,10,5};
    float alts[10], rngs[10];
    float oldAngle = fireAngle;
    
    for (int i=0; i<10; i++) {
        fireAngle = angles[i];
        computeBallistic();
        alts[i] = (trajCount>0) ? maxAlt : 0;
        rngs[i] = (trajCount>0) ? trajPoints[trajCount-1].range : 0;
    }
    fireAngle = oldAngle;
    computeBallistic();
    
    M5.Display.fillScreen(C_BG);
    drawHeader(STR("AA Envelope", "防空包络"), STR("ESC", "返回"));
    
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextColor(C_WHT);
    if (lang == LANG_CN && cnFontAvailable) {
        M5.Display.drawString("角度", 4, 18);
        M5.Display.drawString("最大射高", 55, 18);
        M5.Display.drawString("最大射程", 145, 18);
    } else {
        M5.Display.drawString("ANG", 4, 18);
        M5.Display.drawString("MAX ALT", 55, 18);
        M5.Display.drawString("MAX RNG", 145, 18);
    }
    M5.Display.drawFastHLine(0, 25, SW, C_SURF2);
    
    int y = 28;
    for (int i=0; i<7 && y<SH-10; i++) {
        char buf[24];
        M5.Display.setTextColor(C_ACC);
        snprintf(buf, sizeof(buf), "%.0f", angles[i]);
        M5.Display.drawString(buf, 4, y);
        M5.Display.setTextColor(C_WHT);
        snprintf(buf, sizeof(buf), "%.0f m", alts[i]);
        M5.Display.drawString(buf, 55, y);
        M5.Display.setTextColor(C_CYA);
        snprintf(buf, sizeof(buf), "%.0f m", rngs[i]);
        M5.Display.drawString(buf, 145, y);
        y += 12;
    }
    
    float maxAltAA = 0;
    for (int i=0; i<10; i++) if (alts[i]>maxAltAA) maxAltAA=alts[i];
    M5.Display.setTextColor(C_YEL);
    char buf[32];
    if (lang == LANG_CN && cnFontAvailable) {
        snprintf(buf, sizeof(buf), "最大射高: %.0f m", maxAltAA);
    } else {
        snprintf(buf, sizeof(buf), "Max Alt: %.0f m", maxAltAA);
    }
    M5.Display.drawString(buf, 4, y+4);
}

// Range Solver screen states
int solverState = 0;  // 0=input, 1=calculating, 2=done
int solverScroll = 0; // Scroll offset for results

void drawRangeSolver(bool fullRedraw = true) {
    if (fullRedraw) {
        M5.Display.fillScreen(C_BG);
        // No header - use full screen to avoid scroll overlap issues
        solverState = 0;  // Start in input mode
        solverScroll = 0;
    } else {
        M5.Display.fillRect(0, 0, SW, SH, C_BG);
    }
    
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextSize(1);
    char buf[40];
    
    // Input mode - user enters target range
    if (solverState == 0) {
        M5.Display.setTextColor(C_DIM);
        M5.Display.drawString(STR("Range Solver", "距离求解"), 4, 4);
        
        M5.Display.setTextColor(C_WHT);
        M5.Display.drawString(STR("Target Range:", "目标距离:"), 4, 18);
        
        // Show current input with cursor
        M5.Display.setTextColor(C_YEL);
        bool showCursor = (millis()/500)%2==0;
        if (editing) {
            snprintf(buf, sizeof(buf), "%s%s m", editBuf, showCursor ? "_" : " ");
        } else {
            snprintf(buf, sizeof(buf), "%.0f m", targetRange);
        }
        M5.Display.drawString(buf, 4, 32);
        
        M5.Display.setTextColor(C_DIM);
        M5.Display.drawString(STR("0-9: Type number", "0-9: 输入数字"), 4, 48);
        M5.Display.drawString(STR("FN+.: Decimal point", "FN+.: 小数点"), 4, 60);
        M5.Display.drawString(STR("ENTER: Solve", "ENTER: 求解"), 4, 72);
        M5.Display.drawString(STR("ESC: Back", "ESC: 返回"), 4, 84);
        return;
    }
    
    // Calculating mode
    if (solverState == 1) {
        M5.Display.setTextColor(C_YEL);
        M5.Display.setTextDatum(middle_center);
        M5.Display.drawString(STR("Solving...", "计算中..."), SW/2, SH/2);
        M5.Display.setTextDatum(middle_left);
        solveRange();
        solverState = 2;
        M5.Display.fillScreen(C_BG);
    }
    
    // Results mode
    if (solverState == 2 && solverDone) {
        int y = 4 - solverScroll;
        
        // Title - dimmer color
        M5.Display.setTextColor(C_DIM);
        M5.Display.drawString(STR("Solver Result", "求解结果"), 4, y); y += 12;
        
        // Target range
        M5.Display.setTextColor(C_GRN);
        M5.Display.drawString(STR("Target:", "目标:"), 4, y);
        M5.Display.setTextColor(C_WHT);
        snprintf(buf, sizeof(buf), "%.0f m", targetRange);
        M5.Display.drawString(buf, 60, y); y += 12;
        
        // Firing angle
        M5.Display.setTextColor(C_GRN);
        M5.Display.drawString(STR("Angle:", "角度:"), 4, y);
        M5.Display.setTextColor(C_WHT);
        snprintf(buf, sizeof(buf), "%.3f deg", solverAngle);
        M5.Display.drawString(buf, 60, y); y += 12;
        
        M5.Display.setTextColor(C_GRN);
        M5.Display.drawString(STR("Mils:", "密位:"), 4, y);
        M5.Display.setTextColor(C_WHT);
        snprintf(buf, sizeof(buf), "%.1f", solverAngle*17.778);
        M5.Display.drawString(buf, 60, y); y += 14;
        
        // Other data
        M5.Display.setTextColor(C_DIM);
        snprintf(buf, sizeof(buf), "Actual: %.1f m", solverRange);
        M5.Display.drawString(buf, 4, y); y += 11;
        snprintf(buf, sizeof(buf), "TOF: %.3f s", solverTOF);
        M5.Display.drawString(buf, 4, y); y += 11;
        snprintf(buf, sizeof(buf), "Vel: %.1f m/s", solverVel);
        M5.Display.drawString(buf, 4, y); y += 11;
        snprintf(buf, sizeof(buf), "Max Alt: %.1f m", solverMaxAlt);
        M5.Display.drawString(buf, 4, y); y += 14;
        
        // Penetration
        float penAngle = fabsf(solverAngle);
        float penSide = thompsonPen(currentWeapon.diameter, currentWeapon.mass, solverVel, penAngle);
        float penDeck = thompsonPen(currentWeapon.diameter, currentWeapon.mass, solverVel, 90-penAngle);
        M5.Display.setTextColor(C_YEL);
        snprintf(buf, sizeof(buf), "Pen Side: %.1f mm", penSide);
        M5.Display.drawString(buf, 4, y); y += 11;
        snprintf(buf, sizeof(buf), "Pen Deck: %.1f mm", penDeck);
        M5.Display.drawString(buf, 4, y);
        
    } else if (solverState == 2 && !solverDone) {
        M5.Display.setTextColor(C_RED);
        M5.Display.drawString(STR("Cannot reach target!", "无法到达目标!"), 4, 30);
    }
}

// Max Range scroll offset
int maxRangeScroll = 0;

void drawMaxRange(bool fullRedraw = true) {
    if (fullRedraw) {
        M5.Display.fillScreen(C_BG);
        // No header - use full screen
        maxRangeScroll = 0;
        
        // Show calculating message before computation
        if (!maxRangeDone) {
            M5.Display.setTextDatum(middle_center);
            M5.Display.setTextSize(1);
            M5.Display.setTextColor(C_YEL);
            M5.Display.drawString(STR("Max Range", "最大射程"), SW/2, SH/2 - 20);
            M5.Display.setTextColor(C_WHT);
            M5.Display.drawString(STR("Calculating...", "计算中..."), SW/2, SH/2);
            M5.Display.setTextDatum(middle_left);
            
            findMaxRange();  // This takes a moment
            
            M5.Display.fillScreen(C_BG);
        }
    } else {
        M5.Display.fillRect(0, 0, SW, SH, C_BG);
    }
    
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextSize(1);
    char buf[40];
    
    if (maxRangeDone) {
        int y = 4 - maxRangeScroll;
        
        // Title - dimmer color
        M5.Display.setTextColor(C_DIM);
        M5.Display.drawString(STR("Max Range Result", "最大射程结果"), 4, y); y += 12;
        
        // Max Range result
        M5.Display.setTextColor(C_GRN);
        M5.Display.drawString(STR("Range:", "射程:"), 4, y);
        M5.Display.setTextColor(C_WHT);
        snprintf(buf, sizeof(buf), "%.0f m", maxRange);
        M5.Display.drawString(buf, 60, y); y += 12;
        
        M5.Display.setTextColor(C_GRN);
        M5.Display.drawString(STR("Yards:", "码:"), 4, y);
        M5.Display.setTextColor(C_WHT);
        snprintf(buf, sizeof(buf), "%.0f", maxRange*1.0936);
        M5.Display.drawString(buf, 60, y); y += 14;
        
        // Optimal angle
        M5.Display.setTextColor(C_GRN);
        M5.Display.drawString(STR("Angle:", "角度:"), 4, y);
        M5.Display.setTextColor(C_WHT);
        snprintf(buf, sizeof(buf), "%.1f deg", maxRangeAngle);
        M5.Display.drawString(buf, 60, y); y += 14;
        
        // Details
        M5.Display.setTextColor(C_DIM);
        snprintf(buf, sizeof(buf), "TOF: %.2f sec", maxRangeTOF);
        M5.Display.drawString(buf, 4, y); y += 11;
        snprintf(buf, sizeof(buf), "Vel: %.1f m/s", maxRangeVel);
        M5.Display.drawString(buf, 4, y); y += 11;
        snprintf(buf, sizeof(buf), "Max Alt: %.0f m", maxRangeAlt);
        M5.Display.drawString(buf, 4, y); y += 11;
        snprintf(buf, sizeof(buf), "(%.0f ft)", maxRangeAlt*3.281);
        M5.Display.drawString(buf, 4, y); y += 14;
        
        // Penetration
        float penSide = thompsonPen(currentWeapon.diameter, currentWeapon.mass, maxRangeVel, maxRangeAngle);
        float penDeck = thompsonPen(currentWeapon.diameter, currentWeapon.mass, maxRangeVel, 90-maxRangeAngle);
        M5.Display.setTextColor(C_YEL);
        snprintf(buf, sizeof(buf), "Pen Side: %.1f mm", penSide);
        M5.Display.drawString(buf, 4, y); y += 11;
        snprintf(buf, sizeof(buf), "Pen Deck: %.1f mm", penDeck);
        M5.Display.drawString(buf, 4, y);
    } else {
        M5.Display.setTextColor(C_RED);
        M5.Display.drawString(STR("Calculation failed!", "计算失败!"), 4, 30);
    }
}

void drawSettings(bool fullRedraw = true) {
    if (fullRedraw) {
        M5.Display.fillScreen(C_BG);
        drawHeader("Settings", "ESC");
    } else {
        M5.Display.fillRect(0, 14, SW, SH-14, C_BG);
    }
    
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextSize(1);
    char buf[40];
    
    // Language setting
    int y = 16;
    bool sel = (settingsSel == 0);
    if (sel) M5.Display.fillRect(4, y, SW-8, 14, C_SEL);
    M5.Display.setTextColor(sel ? C_WHT : C_DIM);
    M5.Display.drawString("Language", 8, y+7);
    M5.Display.setTextDatum(middle_right);
    M5.Display.setTextColor(sel ? C_ACC : C_WHT);
    M5.Display.drawString(lang == LANG_EN ? "English" : "中文", SW-8, y+7);
    M5.Display.setTextDatum(middle_left);
    y += 16;
    
    // Atmospheric settings header
    M5.Display.setTextColor(C_GRN);
    M5.Display.drawString("Atmosphere:", 4, y); y += 14;
    
    // Temperature
    sel = (settingsSel == 1);
    if (sel) M5.Display.fillRect(4, y, SW-8, 14, C_SEL);
    M5.Display.setTextColor(sel ? C_WHT : C_DIM);
    M5.Display.drawString("Temp (C)", 8, y+7);
    M5.Display.setTextDatum(middle_right);
    if (editing && sel) {
        bool showCur = (millis()/500)%2==0;
        snprintf(buf, sizeof(buf), "%s%s", editBuf, showCur ? "_" : " ");
        M5.Display.setTextColor(C_YEL);
    } else {
        snprintf(buf, sizeof(buf), "%.1f", atmTemperature);
        M5.Display.setTextColor(sel ? C_ACC : C_WHT);
    }
    M5.Display.drawString(buf, SW-8, y+7);
    M5.Display.setTextDatum(middle_left);
    y += 16;
    
    // Pressure
    sel = (settingsSel == 2);
    if (sel) M5.Display.fillRect(4, y, SW-8, 14, C_SEL);
    M5.Display.setTextColor(sel ? C_WHT : C_DIM);
    M5.Display.drawString("Press (hPa)", 8, y+7);
    M5.Display.setTextDatum(middle_right);
    if (editing && sel) {
        bool showCur = (millis()/500)%2==0;
        snprintf(buf, sizeof(buf), "%s%s", editBuf, showCur ? "_" : " ");
        M5.Display.setTextColor(C_YEL);
    } else {
        snprintf(buf, sizeof(buf), "%.1f", atmPressure);
        M5.Display.setTextColor(sel ? C_ACC : C_WHT);
    }
    M5.Display.drawString(buf, SW-8, y+7);
    M5.Display.setTextDatum(middle_left);
    y += 16;
    
    // Humidity
    sel = (settingsSel == 3);
    if (sel) M5.Display.fillRect(4, y, SW-8, 14, C_SEL);
    M5.Display.setTextColor(sel ? C_WHT : C_DIM);
    M5.Display.drawString("RH (%)", 8, y+7);
    M5.Display.setTextDatum(middle_right);
    if (editing && sel) {
        bool showCur = (millis()/500)%2==0;
        snprintf(buf, sizeof(buf), "%s%s", editBuf, showCur ? "_" : " ");
        M5.Display.setTextColor(C_YEL);
    } else {
        snprintf(buf, sizeof(buf), "%.0f", atmHumidity);
        M5.Display.setTextColor(sel ? C_ACC : C_WHT);
    }
    M5.Display.drawString(buf, SW-8, y+7);
    M5.Display.setTextDatum(middle_left);
    y += 18;
    
    // Calculated air density
    M5.Display.setTextColor(C_GRN);
    M5.Display.drawString("Air Density:", 4, y); y += 12;
    M5.Display.setTextColor(C_WHT);
    double density = getCurrentAirDensity();
    snprintf(buf, sizeof(buf), "%.4f kg/m3", density);
    M5.Display.drawString(buf, 8, y); y += 12;
    
    // Relative to standard
    M5.Display.setTextColor(C_DIM);
    double stdDensity = calcAirDensity(15.0, 1013.25, 0.0);
    double relDensity = (density / stdDensity) * 100.0;
    snprintf(buf, sizeof(buf), "RAD: %.1f%%", relDensity);
    M5.Display.drawString(buf, 8, y);
}

// ═══════════════════════════════════════════════════════════
//  Input Handling
// ═══════════════════════════════════════════════════════════

void loadWeapon(int idx) {
    if (idx>=0 && idx<WEAPON_COUNT) memcpy_P(&currentWeapon, &WEAPONS[idx], sizeof(Weapon));
}

void resetDefaults() {
    fireAngle=0; fireAlt=0; targetAlt=0; targetRange=1000;
    atmTemperature=15.0f; atmPressure=1013.25f; atmHumidity=0.0f;
    loadWeapon(8);
    computed=false;
}

void startEdit() { editing=true; editLen=0; editBuf[0]='\0'; }

void commitSettingsEdit() {
    if (!editing || editLen==0) { editing=false; return; }
    editBuf[editLen] = '\0';
    float val = atof(editBuf);
    switch (settingsSel) {
        case 1: atmTemperature = constrain(val, -45, 100); break;   // -45°C to 100°C
        case 2: atmPressure = constrain(val, 300, 1250); break;    // 300-1250 hPa
        case 3: atmHumidity = constrain(val, 0, 100); break;       // 0-100%
    }
    editing=false; editLen=0; computed=false;
}

void commitEdit() {
    if (!editing || editLen==0) { editing=false; return; }
    editBuf[editLen] = '\0';
    float val = atof(editBuf);
    switch (fieldSel) {
        case 0: fireAngle = constrain(val, -90, 90); break;
        case 1: fireAlt = constrain(val, 0, 30000); break;
        case 2: targetAlt = constrain(val, 0, 30000); break;
        case 3: targetRange = constrain(val, 10, 50000); break;
        case 4: atmTemperature = constrain(val, -45, 100); break;  // -45°C to 100°C
        case 5: atmPressure = constrain(val, 300, 1250); break;   // 300-1250 hPa
    }
    editing=false; editLen=0; computed=false;
}

void handleKey(uint8_t k) {
    switch (currentScreen) {
        case 0:  // Main menu - with wrap-around (10 items)
            if (k == K_UP) menuSel = (menuSel > 0) ? menuSel-1 : 9;
            if (k == K_DN) menuSel = (menuSel < 9) ? menuSel+1 : 0;
            if (k == K_ENT || k == K_SP) {
                switch (menuSel) {
                    case 0: currentScreen=1; catSel=0; break;          // Select Weapon
                    case 1: currentScreen=2; fieldSel=0; editing=false; break; // Edit Parameters
                    case 2: currentScreen=3; computeBallistic(); break; // Trajectory
                    case 3: currentScreen=4; generateRangeTable(); break; // Range Table
                    case 4: currentScreen=5; solverDone=false; break;  // Range Solver
                    case 5: currentScreen=6; maxRangeDone=false; break; // Max Range
                    case 6: currentScreen=7; computeBallistic(); break; // Penetration
                    case 7: currentScreen=8; break;                    // AA Envelope
                    case 8: currentScreen=9; settingsSel=0; break;     // Settings
                    case 9: resetDefaults(); break;                    // Reset
                }
            }
            // Quick select: 1-9 for items 0-8, 0 for item 9
            if (k >= K_1 && k <= K_9) { menuSel = k-K_1; handleKey(K_ENT); }
            if (k == K_0) { menuSel = 9; handleKey(K_ENT); }
            break;
            
        case 1:  // Category - with wrap-around
            if (k == K_UP) catSel = (catSel > 0) ? catSel-1 : 5;
            if (k == K_DN) catSel = (catSel < 5) ? catSel+1 : 0;
            if (k == K_ENT || k == K_SP) { currentScreen=10; weaponSel=0; }
            if (k == K_ESC) currentScreen = 0;
            if (k >= K_1 && k <= K_6) { catSel = k-K_1; handleKey(K_ENT); }
            break;
            
        case 2:  // Editor (6 fields: angle, fire alt, target alt, range, temp, pressure)
            if (k == K_UP) { commitEdit(); fieldSel = max(0, fieldSel-1); }
            if (k == K_DN) { commitEdit(); fieldSel = min(5, fieldSel+1); }
            if (k == K_ENT || k == K_ESC) { commitEdit(); currentScreen=0; }
            if (k >= K_1 && k <= K_9) { if (!editing) startEdit(); if (editLen<12) { editBuf[editLen++]='1'+(k-K_1); editBuf[editLen]='\0'; } }
            if (k == K_0) { if (!editing) startEdit(); if (editLen<12) { editBuf[editLen++]='0'; editBuf[editLen]='\0'; } }
            if (k == K_MI) { if (!editing) startEdit(); if (editLen==0) { editBuf[editLen++]='-'; editBuf[editLen]='\0'; } }
            if (k == K_DOT) {
                if (!editing) startEdit();
                if (editLen<12) {
                    bool hasDot=false;
                    for (int i=0; i<editLen; i++) if (editBuf[i]=='.') { hasDot=true; break; }
                    if (!hasDot) {
                        if (editLen==0 || editBuf[editLen-1]=='-') editBuf[editLen++]='0';
                        editBuf[editLen++]='.'; editBuf[editLen]='\0';
                    }
                }
            }
            if (k == K_BK && editing && editLen>0) { editLen--; editBuf[editLen]='\0'; }
            break;
            
        case 3: case 4: case 7: case 8:  // Simple results screens
            if (k == K_ESC) currentScreen = 0;
            break;
            
        case 5:  // Range Solver - has input mode and scrolling
            if (solverState == 0) {
                // Input mode - same as editor
                if (k == K_ENT) {
                    if (editing && editLen > 0) {
                        editBuf[editLen] = '\0';
                        targetRange = constrain(atof(editBuf), 10, 50000);
                        editing = false;
                        editLen = 0;
                    }
                    solverState = 1;  // Start calculating
                    needsDraw = true;
                }
                if (k == K_ESC) { editing = false; editLen = 0; currentScreen = 0; }
                // Number input
                if (k >= K_1 && k <= K_9) { if (!editing) startEdit(); if (editLen<12) { editBuf[editLen++]='1'+(k-K_1); editBuf[editLen]='\0'; } }
                if (k == K_0) { if (!editing) startEdit(); if (editLen<12) { editBuf[editLen++]='0'; editBuf[editLen]='\0'; } }
                if (k == K_MI) { if (!editing) startEdit(); if (editLen==0) { editBuf[editLen++]='-'; editBuf[editLen]='\0'; } }
                if (k == K_DOT) {
                    if (!editing) startEdit();
                    if (editLen<12) {
                        bool hasDot=false;
                        for (int i=0; i<editLen; i++) if (editBuf[i]=='.') { hasDot=true; break; }
                        if (!hasDot) {
                            if (editLen==0 || editBuf[editLen-1]=='-') editBuf[editLen++]='0';
                            editBuf[editLen++]='.'; editBuf[editLen]='\0';
                        }
                    }
                }
                if (k == K_BK && editing && editLen>0) { editLen--; editBuf[editLen]='\0'; }
            } else {
                // Results mode - scrolling (no header, so just prevent negative)
                if (k == K_UP) solverScroll = max(0, solverScroll - 20);
                if (k == K_DN) solverScroll += 20;
                if (k == K_ESC) { solverState = 0; solverDone = false; currentScreen = 0; }
            }
            break;
            
        case 6:  // Max Range - scrolling (no header)
            if (k == K_UP) maxRangeScroll = max(0, maxRangeScroll - 20);
            if (k == K_DN) maxRangeScroll += 20;
            if (k == K_ESC) currentScreen = 0;
            break;
            
        case 9:  // Settings (4 items: language, temp, pressure, humidity)
            if (k == K_UP) { if (editing) commitSettingsEdit(); settingsSel = max(0, settingsSel-1); }
            if (k == K_DN) { if (editing) commitSettingsEdit(); settingsSel = min(3, settingsSel+1); }
            if (k == K_ENT || k == K_SP) {
                if (settingsSel == 0) {
                    // Toggle language
                    lang = (lang == LANG_EN) ? LANG_CN : LANG_EN;
                } else {
                    // Start editing atmospheric parameters
                    if (!editing) startEdit();
                }
            }
            if (k == K_ESC) { 
                if (editing) { commitSettingsEdit(); } 
                else { currentScreen = 0; }
            }
            // Number input for atmospheric parameters
            if (settingsSel > 0) {
                if (k >= K_1 && k <= K_9) { if (!editing) startEdit(); if (editLen<12) { editBuf[editLen++]='1'+(k-K_1); editBuf[editLen]='\0'; } }
                if (k == K_0) { if (!editing) startEdit(); if (editLen<12) { editBuf[editLen++]='0'; editBuf[editLen]='\0'; } }
                if (k == K_MI) { if (!editing) startEdit(); if (editLen==0) { editBuf[editLen++]='-'; editBuf[editLen]='\0'; } }
                if (k == K_DOT) {
                    if (!editing) startEdit();
                    if (editLen<12) {
                        bool hasDot=false;
                        for (int i=0; i<editLen; i++) if (editBuf[i]=='.') { hasDot=true; break; }
                        if (!hasDot) {
                            if (editLen==0 || editBuf[editLen-1]=='-') editBuf[editLen++]='0';
                            editBuf[editLen++]='.'; editBuf[editLen]='\0';
                        }
                    }
                }
                if (k == K_BK && editing && editLen>0) { editLen--; editBuf[editLen]='\0'; }
            }
            break;
            
        case 10:  // Weapon select - with wrap-around
            {
                int count = 0;
                for (int i=0; i<WEAPON_COUNT; i++) {
                    Weapon w; memcpy_P(&w, &WEAPONS[i], sizeof(Weapon));
                    if (w.category == catSel) count++;
                }
                if (k == K_UP) weaponSel = (weaponSel > 0) ? weaponSel-1 : count-1;
                if (k == K_DN) weaponSel = (weaponSel < count-1) ? weaponSel+1 : 0;
                if (k == K_ENT || k == K_SP) {
                    int idx = 0;
                    for (int i=0; i<WEAPON_COUNT; i++) {
                        Weapon w; memcpy_P(&w, &WEAPONS[i], sizeof(Weapon));
                        if (w.category == catSel) {
                            if (idx == weaponSel) { loadWeapon(i); computed=false; break; }
                            idx++;
                        }
                    }
                    currentScreen = 0;
                }
                if (k == K_ESC) currentScreen = 1;
            }
            break;
    }
    needsDraw = true;
}

// ═══════════════════════════════════════════════════════════
//  Main
// ═══════════════════════════════════════════════════════════

// Track previous screen for smart refresh
int prevScreen = -1;

void setup() {
    M5Cardputer.begin();
    M5.Display.setRotation(1);
    
    // Cardputer Adv has only 512KB SRAM, no PSRAM
    // Cannot load TTF fonts (too large for available memory)
    // Using English only with built-in font
    cnFontAvailable = false;
    
    resetDefaults();
    currentScreen = 0; 
    prevScreen = -1;  // Force full redraw
    needsDraw = true;
}

void loop() {
    M5Cardputer.update();
    
    uint8_t key = 0;
    if (M5Cardputer.Keyboard.isChange()) {
        if (M5Cardputer.Keyboard.isPressed()) {
            auto st = M5Cardputer.Keyboard.keysState();
            if (st.fn) {
                for (auto k : st.hid_keys) { if (k == K_DN) { key = K_DOT; break; } }
            }
            if (!key) {
                for (auto k : st.hid_keys) {
                    if (k==K_UP||k==K_DN||k==K_LT||k==K_RT||k==K_ESC||k==K_ENT||k==K_BK||k==K_SP||k==K_MI||k==K_DOT||(k>=K_1&&k<=K_0))
                    { key=k; break; }
                }
            }
        } else { lastKey = 0; }
    }
    
    if (key) { lastKey=key; keyTime=millis(); repeatTime=millis(); }
    else if (lastKey) {
        unsigned long now = millis();
        bool cr = (lastKey==K_UP||lastKey==K_DN||lastKey==K_LT||lastKey==K_RT||lastKey==K_BK);
        if (cr && now-keyTime>300 && now-repeatTime>80) { key=lastKey; repeatTime=now; }
        if (now-keyTime>2000) lastKey=0;
    }
    
    if (key) handleKey(key);
    
    if (needsDraw) {
        // Smart refresh: only clear screen when switching screens
        bool screenChanged = (currentScreen != prevScreen);
        
        switch (currentScreen) {
            case 0: drawMainMenu(screenChanged); break;
            case 1: drawCategorySelect(screenChanged); break;
            case 2: drawEditor(screenChanged); break;
            case 3: drawTrajectory(screenChanged); break;
            case 4: drawRangeTable(screenChanged); break;
            case 5: drawRangeSolver(screenChanged); break;    // NEW
            case 6: drawMaxRange(screenChanged); break;        // NEW
            case 7: drawPenetration(screenChanged); break;
            case 8: drawAAEnvelope(screenChanged); break;
            case 9: drawSettings(screenChanged); break;
            case 10: drawWeaponSelect(screenChanged); break;
        }
        
        prevScreen = currentScreen;
        needsDraw = false;
    }
    delay(10);
}
