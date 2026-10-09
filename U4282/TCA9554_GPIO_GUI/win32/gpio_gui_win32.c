/*-----------------------------------------------------------------------------
 *  TCA9554 GPIO 测试程序 - 原生 Win32 版本 (不依赖 Qt)
 *
 *  功能与 Qt 版一致 :
 *     DO / DI 的值都显示在方框里 , 点击 DO 的方框取反并写入 ; DI 只读
 *     状态用颜色表达(高=绿 低=灰) , 顶部显示连接状态 , 底部显示寄存器原始值
 *     SMBus 地址 0x40 ; DI1..DI4 = bit7..bit4 , DO1..DO4 = bit3..bit0
 *
 *  GPIO 读写通过厂商动态库 SvApiLibx64.dll (信步/拓朗 SvApiLib) 完成 ,
 *  运行前请把 SvApiLibx64.dll 放到本程序同一目录 , 并安装厂商驱动。
 *---------------------------------------------------------------------------*/
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <wchar.h>

/*=============================================================================
 *  厂商库 SvApiLibx64.dll
 *===========================================================================*/
typedef BOOL (WINAPI *FnInit)(void);
typedef void (WINAPI *FnUninit)(void);
typedef unsigned char (WINAPI *FnSmbRead)(unsigned char addr, unsigned char offset, unsigned char *data);
typedef unsigned char (WINAPI *FnSmbWrite)(unsigned char addr, unsigned char offset, unsigned char value);

static HMODULE    g_hLib;
static FnInit     g_init;
static FnUninit   g_uninit;
static FnSmbRead  g_read;
static FnSmbWrite g_write;
static int        g_open;
static wchar_t    g_errMsg[256];      /* 打开失败的原因 */

#define TCA_ADDR   0x40
#define REG_INPUT  0x00
#define REG_OUTPUT 0x01
#define REG_CONFIG 0x03

static const int DI_BIT[4] = { 7, 6, 5, 4 };   /* DI1..DI4 */
static const int DO_BIT[4] = { 3, 2, 1, 0 };   /* DO1..DO4 */

static unsigned char reg_read(unsigned char reg)
{
    unsigned char d = 0;
    if (g_read)
        g_read(TCA_ADDR, reg, &d);
    return d;
}

static void reg_write(unsigned char reg, unsigned char value)
{
    if (g_write)
        g_write(TCA_ADDR, reg, value);
}

static int tca_read_reg(int reg)
{
    if (!g_open)
        return -1;
    return (int)reg_read((unsigned char)reg);
}

static int tca_open(void)
{
    DWORD e;

    if (g_open)
        return 1;

    g_errMsg[0] = 0;

    g_hLib = LoadLibraryW(L"SvApiLibx64.dll");
    if (!g_hLib)
        g_hLib = LoadLibraryA("SvApiLib.dll");
    if (!g_hLib) {
        e = GetLastError();
        if (e == ERROR_MOD_NOT_FOUND)
            wcscpy(g_errMsg, L"找不到 SvApiLibx64.dll：请把该 dll 和本程序放在同一目录");
        else if (e == ERROR_BAD_EXE_FORMAT)
            wcscpy(g_errMsg, L"SvApiLibx64.dll 位数不对：本程序是 64 位，需要 64 位的 dll");
        else
            wsprintfW(g_errMsg, L"加载 SvApiLibx64.dll 失败 (错误码 %u)", (unsigned)e);
        return 0;
    }

    g_init   = (FnInit)   GetProcAddress(g_hLib, "SvApiLibInitialize");
    g_uninit = (FnUninit) GetProcAddress(g_hLib, "SvApiLibUnInitialize");
    g_read   = (FnSmbRead)GetProcAddress(g_hLib, "SvSmbReadByte");
    g_write  = (FnSmbWrite)GetProcAddress(g_hLib, "SvSmbWriteByte");
    if (!g_init || !g_uninit || !g_read || !g_write) {
        wcscpy(g_errMsg, L"SvApiLibx64.dll 里找不到导出函数："
                         L"需要 SvApiLibInitialize / SvSmbReadByte / SvSmbWriteByte");
        return 0;
    }

    if (!g_init()) {
        wcscpy(g_errMsg, L"SvApiLibInitialize() 失败：请用「以管理员身份运行」启动本程序，"
                         L"并确认厂商驱动 SvIoCtrlx64.sys 已安装");
        return 0;
    }

    g_open = 1;
    /* DI1..DI4 = 输入 , DO1..DO4 = 输出 */
    reg_write(REG_CONFIG, 0xF0);
    g_errMsg[0] = 0;
    return 1;
}

static void tca_close(void)
{
    if (g_open) {
        if (g_uninit)
            g_uninit();
        g_open = 0;
    }
}

static void tca_write_do(int i, int value)
{
    int bit;
    unsigned char cfg, out;
    if (i < 0 || i > 3)
        return;

    bit = DO_BIT[i];

    /* 确保该脚是输出 */
    cfg = reg_read(REG_CONFIG);
    if (cfg & (1 << bit))
        reg_write(REG_CONFIG, (unsigned char)(cfg & ~(1 << bit)));

    /* 设置输出锁存 */
    out = reg_read(REG_OUTPUT);
    if (value)
        out |= (unsigned char)(1 << bit);
    else
        out &= (unsigned char)~(1 << bit);
    reg_write(REG_OUTPUT, out);
}

static int tca_read_do(int i)
{
    if (i < 0 || i > 3)
        return -1;
    return (reg_read(REG_OUTPUT) >> DO_BIT[i]) & 1;
}

static int tca_read_di(int i)
{
    int bit;
    unsigned char cfg;
    if (i < 0 || i > 3)
        return -1;

    bit = DI_BIT[i];
    /* 确保该脚是输入 */
    cfg = reg_read(REG_CONFIG);
    if (!(cfg & (1 << bit)))
        reg_write(REG_CONFIG, (unsigned char)(cfg | (1 << bit)));

    return (reg_read(REG_INPUT) >> bit) & 1;
}

/*=============================================================================
 *  界面
 *===========================================================================*/
#define IDC_DO_BOX(i)   (1000 + (i))
#define IDC_DI_BOX(i)   (1100 + (i))
#define IDC_CONN        1200
#define IDC_STATUS      1210
#define IDC_REG         1220
#define IDC_ALLOFF      1300
#define IDC_CLOSE       1310
#define IDT_REFRESH     1

#define COL_HIGH RGB(0x1a, 0x7f, 0x37)   /* 高电平: 绿 */
#define COL_LOW  RGB(0x9e, 0x9e, 0x9e)   /* 低电平: 灰 */
#define COL_UNK  RGB(0xc8, 0xc8, 0xc8)
#define COL_OK   RGB(0x1a, 0x7f, 0x37)
#define COL_ERR  RGB(0xc6, 0x28, 0x28)
#define COL_DIM  RGB(0x7a, 0x7a, 0x7a)

static HWND  g_doBox[4];
static HWND  g_diBox[4];
static HWND  g_conn;
static HWND  g_status;
static HWND  g_reg;
static HWND  g_allOff;
static int   g_doState[4];
static int   g_diState[4] = { -1, -1, -1, -1 };
static HFONT g_font, g_fontBold, g_fontValue, g_fontMono;
static HBRUSH g_boxBrush;

static COLORREF state_color(int v)
{
    if (v == 1) return COL_HIGH;
    if (v == 0) return COL_LOW;
    return COL_UNK;
}

static HWND mkctrl(const wchar_t *cls, const wchar_t *text, DWORD style,
                   int x, int y, int w, int h, int id, HWND parent, HFONT font)
{
    HWND hw = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style,
                              x, y, w, h, parent, (HMENU)(INT_PTR)id,
                              GetModuleHandleW(NULL), NULL);
    SendMessageW(hw, WM_SETFONT, (WPARAM)font, TRUE);
    return hw;
}

/* 更新 DO 值框(值 + 颜色) */
static void apply_do(int i, int value)
{
    wchar_t b[8];
    value = value ? 1 : 0;
    g_doState[i] = value;
    wsprintfW(b, L"%d", value);
    SetWindowTextW(g_doBox[i], b);
    InvalidateRect(g_doBox[i], NULL, TRUE);
}

static void refresh_io(void)
{
    int i;

    for (i = 0; i < 4; ++i) {
        int v = g_open ? tca_read_di(i) : -1;
        wchar_t buf[8];
        g_diState[i] = v;
        if (v < 0)
            wcscpy(buf, L"--");
        else
            wsprintfW(buf, L"%d", v);
        SetWindowTextW(g_diBox[i], buf);
        InvalidateRect(g_diBox[i], NULL, TRUE);
    }

    if (g_open) {
        wchar_t r[128];
        for (i = 0; i < 4; ++i) {
            int v = tca_read_do(i);
            if (v >= 0)
                apply_do(i, v);
        }
        wsprintfW(r, L"IN=0x%02X    OUT=0x%02X    CFG=0x%02X",
                  (unsigned)tca_read_reg(0x00),
                  (unsigned)tca_read_reg(0x01),
                  (unsigned)tca_read_reg(0x03));
        SetWindowTextW(g_reg, r);
    } else {
        SetWindowTextW(g_reg, L"IN=0x--    OUT=0x--    CFG=0x--");
    }
}

static void all_off(void)
{
    int i;
    if (!g_open)
        return;
    for (i = 0; i < 4; ++i) {
        tca_write_do(i, 0);
        apply_do(i, 0);
    }
    SetWindowTextW(g_status, L"已全部输出清零");
    refresh_io();
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        int i, rowY[4];

        g_font = CreateFontW(-20, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH, L"Microsoft YaHei UI");
        if (!g_font)
            g_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        g_fontBold = CreateFontW(-20, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
                                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 DEFAULT_PITCH, L"Microsoft YaHei UI");
        g_fontValue = CreateFontW(-18, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                  DEFAULT_PITCH, L"Microsoft YaHei UI");
        g_fontMono = CreateFontW(-16, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET,
                                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 FIXED_PITCH, L"Consolas");
        if (!g_fontBold)  g_fontBold  = g_font;
        if (!g_fontValue) g_fontValue = g_fontBold;
        if (!g_fontMono)  g_fontMono  = g_font;
        g_boxBrush = CreateSolidBrush(RGB(255, 255, 255));

        /* 顶部: 连接状态 + 最近一次操作 */
        g_conn   = mkctrl(L"STATIC", L"\u25CF 未连接", SS_LEFT, 20, 14, 560, 30,
                          IDC_CONN, hwnd, g_fontBold);
        g_status = mkctrl(L"STATIC", L"", SS_RIGHT, 590, 14, 290, 30,
                          IDC_STATUS, hwnd, g_font);

        mkctrl(L"BUTTON", L"输出", BS_GROUPBOX, 20, 52, 430, 388, -1, hwnd, g_fontBold);
        mkctrl(L"BUTTON", L"输入", BS_GROUPBOX, 466, 52, 414, 388, -1, hwnd, g_fontBold);

        for (i = 0; i < 4; ++i)
            rowY[i] = 130 + i * 78;

        for (i = 0; i < 4; ++i) {
            wchar_t lbl[64];

            wsprintfW(lbl, L"OUT%d  (DO%d  bit%d)", i + 1, i + 1, DO_BIT[i]);
            mkctrl(L"STATIC", lbl, SS_LEFT, 44, rowY[i] + 4, 200, 34, -1, hwnd, g_font);

            /* DO 值框: 点击取反 */
            g_doBox[i] = mkctrl(L"STATIC", L"0",
                                SS_CENTER | SS_CENTERIMAGE | SS_SUNKEN | SS_NOTIFY,
                                250, rowY[i], 59, 34, IDC_DO_BOX(i), hwnd, g_fontValue);

            wsprintfW(lbl, L"IN%d  (DI%d  bit%d)", i + 1, i + 1, DI_BIT[i]);
            mkctrl(L"STATIC", lbl, SS_LEFT, 490, rowY[i] + 4, 200, 34, -1, hwnd, g_font);

            /* DI 值框: 只读(显示当前输入值) */
            g_diBox[i] = mkctrl(L"STATIC", L"--",
                                SS_CENTER | SS_CENTERIMAGE | SS_SUNKEN,
                                696, rowY[i], 59, 34, IDC_DI_BOX(i), hwnd, g_fontValue);
        }

        g_reg = mkctrl(L"STATIC", L"IN=0x--    OUT=0x--    CFG=0x--", SS_LEFT,
                       20, 452, 560, 30, IDC_REG, hwnd, g_fontMono);
        g_allOff = mkctrl(L"BUTTON", L"全部输出清零", BS_PUSHBUTTON | WS_TABSTOP,
                          596, 446, 140, 46, IDC_ALLOFF, hwnd, g_font);
        mkctrl(L"BUTTON", L"关闭", BS_PUSHBUTTON | WS_TABSTOP,
               752, 446, 128, 46, IDC_CLOSE, hwnd, g_font);

        if (tca_open()) {
            for (i = 0; i < 4; ++i) {
                int v = tca_read_do(i);
                if (v >= 0)
                    apply_do(i, v);
            }
            SetWindowTextW(g_conn, L"\u25CF 已连接 · SMBus 地址 0x40");
            SetWindowTextW(g_status, L"");
            SetTimer(hwnd, IDT_REFRESH, 500, NULL);
            refresh_io();
        } else {
            for (i = 0; i < 4; ++i)
                SetWindowTextW(g_doBox[i], L"?");
            EnableWindow(g_allOff, FALSE);
            SetWindowTextW(g_status, g_errMsg[0] ? g_errMsg : L"打开 TCA9554 失败");
            refresh_io();
        }
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wp);
        if (id >= IDC_DO_BOX(0) && id < IDC_DO_BOX(0) + 4) {
            if (HIWORD(wp) == STN_CLICKED && g_open) {
                int i = id - IDC_DO_BOX(0);
                int v = g_doState[i] ? 0 : 1;      /* 点击取反 */
                wchar_t s[128];
                tca_write_do(i, v);
                apply_do(i, v);
                wsprintfW(s, L"DO%d (bit%d) = %d", i + 1, DO_BIT[i], v);
                SetWindowTextW(g_status, s);
            }
        } else if (id == IDC_ALLOFF) {
            all_off();
        } else if (id == IDC_CLOSE) {
            DestroyWindow(hwnd);
        }
        return 0;
    }

    /* 给数值框/连接状态上色 */
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wp;
        int id = GetDlgCtrlID((HWND)lp);
        int known = 1;
        int isBox = 0;
        COLORREF c = RGB(0, 0, 0);

        if (id >= IDC_DO_BOX(0) && id < IDC_DO_BOX(0) + 4) {
            c = state_color(g_doState[id - IDC_DO_BOX(0)]);
            isBox = 1;
        } else if (id >= IDC_DI_BOX(0) && id < IDC_DI_BOX(0) + 4) {
            c = state_color(g_diState[id - IDC_DI_BOX(0)]);
            isBox = 1;
        } else if (id == IDC_CONN)
            c = g_open ? COL_OK : COL_ERR;
        else if (id == IDC_REG)
            c = COL_DIM;
        else if (id == IDC_STATUS)
            c = g_open ? COL_DIM : COL_ERR;
        else
            known = 0;

        SetBkMode(hdc, TRANSPARENT);
        if (known)
            SetTextColor(hdc, c);
        return (LRESULT)(isBox ? g_boxBrush : GetSysColorBrush(COLOR_BTNFACE));
    }

    case WM_TIMER:
        if (wp == IDT_REFRESH)
            refresh_io();
        return 0;

    case WM_KEYDOWN:
        if (wp == VK_ESCAPE)
            DestroyWindow(hwnd);
        return 0;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, IDT_REFRESH);
        tca_close();
        if (g_boxBrush) DeleteObject(g_boxBrush);
        if (g_fontMono && g_fontMono != g_font) DeleteObject(g_fontMono);
        if (g_fontValue && g_fontValue != g_font && g_fontValue != g_fontBold) DeleteObject(g_fontValue);
        if (g_fontBold && g_fontBold != g_font) DeleteObject(g_fontBold);
        if (g_font) DeleteObject(g_font);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmdLine, int nShow)
{
    WNDCLASSEXW wc;
    RECT rc;
    HWND hwnd;
    MSG msg;
    DWORD style = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;

    (void)hPrev;
    (void)cmdLine;

    InitCommonControls();

    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"TCA9554GpioGui";
    wc.hIcon         = LoadIconW(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    rc.left = 0; rc.top = 0; rc.right = 900; rc.bottom = 520;
    AdjustWindowRect(&rc, style, FALSE);

    hwnd = CreateWindowExW(
        0, wc.lpszClassName,
        L"东莞市拓朗工控设备有限公司  GPIO测试程序 (TCA9554 @0x40)",
        style, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInst, NULL);

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
