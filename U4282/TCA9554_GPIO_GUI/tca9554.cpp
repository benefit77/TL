#include "tca9554.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include "SvApiLib.h"
#  include <unistd.h>
#endif

// Bit position of every DI/DO channel (see GPIO.txt).
static const int kDiBits[Tca9554::kDiCount] = { 7, 6, 5, 4 };  // DI1..DI4
static const int kDoBits[Tca9554::kDoCount] = { 3, 2, 1, 0 };  // DO1..DO4

int Tca9554::diBit(int index)
{
    return (index >= 0 && index < kDiCount) ? kDiBits[index] : -1;
}

int Tca9554::doBit(int index)
{
    return (index >= 0 && index < kDoCount) ? kDoBits[index] : -1;
}

//=============================================================================
//  Platform glue to the vendor SvApiLib
//=============================================================================
#ifdef _WIN32
namespace {
typedef BOOL (WINAPI *FnInit)();
typedef void (WINAPI *FnUninit)();
typedef unsigned char (WINAPI *FnSmbRead)(unsigned char addr, unsigned char offset, unsigned char *data);
typedef unsigned char (WINAPI *FnSmbWrite)(unsigned char addr, unsigned char offset, unsigned char value);

HMODULE   g_hLib   = nullptr;
FnInit    g_init   = nullptr;
FnUninit  g_uninit = nullptr;
FnSmbRead g_read   = nullptr;
FnSmbWrite g_write = nullptr;

bool loadVendorLib(QString *err)
{
    if (g_hLib)
        return true;

    g_hLib = LoadLibraryW(L"SvApiLibx64.dll");
    if (!g_hLib)
        g_hLib = LoadLibraryA("SvApiLib.dll");
    if (!g_hLib) {
        if (err) *err = QStringLiteral("无法加载 SvApiLibx64.dll (请与程序放在同一目录)");
        return false;
    }

    g_init   = (FnInit)   GetProcAddress(g_hLib, "SvApiLibInitialize");
    g_uninit = (FnUninit) GetProcAddress(g_hLib, "SvApiLibUnInitialize");
    g_read   = (FnSmbRead)GetProcAddress(g_hLib, "SvSmbReadByte");
    g_write  = (FnSmbWrite)GetProcAddress(g_hLib, "SvSmbWriteByte");

    if (!g_init || !g_uninit || !g_read || !g_write) {
        if (err) *err = QStringLiteral("SvApiLib 缺少导出函数 (SvSmbReadByte / SvSmbWriteByte)");
        return false;
    }
    return true;
}
} // namespace

static unsigned char smbRead(unsigned char addr, unsigned char reg)
{
    unsigned char data = 0;
    g_read(addr, reg, &data);
    return data;
}

static void smbWrite(unsigned char addr, unsigned char reg, unsigned char value)
{
    g_write(addr, reg, value);
}

#else  // ---------------- Linux ----------------

static unsigned char smbRead(unsigned char addr, unsigned char reg)
{
    return SvSmbReadByte(addr, reg);
}

static void smbWrite(unsigned char addr, unsigned char reg, unsigned char value)
{
    SvSmbWriteByte(addr, reg, value);
}

#endif

//=============================================================================
//  Tca9554
//=============================================================================
Tca9554::Tca9554(unsigned char address)
    : m_address(address), m_open(false)
{
}

Tca9554::~Tca9554()
{
    close();
}

bool Tca9554::open(QString *errorMessage)
{
    if (m_open)
        return true;

#ifdef _WIN32
    if (!loadVendorLib(errorMessage))
        return false;
    if (!g_init()) {
        if (errorMessage) *errorMessage = QStringLiteral("初始化驱动失败 (请以管理员身份运行)");
        return false;
    }
#else
    // The vendor library touches /dev/mem and I/O ports; without root
    // privileges SvApiLibInit() crashes, so guard it here.
    if (::geteuid() != 0) {
        if (errorMessage) *errorMessage = QStringLiteral("需要 root 权限访问 SMBus, 请用 sudo 运行");
        return false;
    }
    if (!SvApiLibInit()) {
        if (errorMessage) *errorMessage = QStringLiteral("初始化驱动失败 (请用 root / sudo 运行)");
        return false;
    }
#endif

    m_open = true;
    return true;
}

void Tca9554::close()
{
    if (!m_open)
        return;

#ifdef _WIN32
    if (g_uninit)
        g_uninit();
#else
    SvApiLibUnInit();
#endif

    m_open = false;
}

unsigned char Tca9554::readReg(unsigned char reg)
{
    return smbRead(m_address, reg);
}

void Tca9554::writeReg(unsigned char reg, unsigned char value)
{
    smbWrite(m_address, reg, value);
}

int Tca9554::readBit(unsigned char reg, int bit)
{
    if (!m_open || bit < 0 || bit > 7)
        return -1;
    return (readReg(reg) >> bit) & 0x01;
}

bool Tca9554::configureDirections(QString *errorMessage)
{
    if (!m_open) {
        if (errorMessage) *errorMessage = QStringLiteral("设备未打开");
        return false;
    }

    // DI1..DI4 (bit7..bit4) = 1 -> input
    // DO1..DO4 (bit3..bit0) = 0 -> output
    unsigned char config = 0xF0;
    writeReg(RegConfig, config);
    return true;
}

bool Tca9554::writeDo(int index, int value, QString *errorMessage)
{
    int bit = doBit(index);
    if (!m_open || bit < 0) {
        if (errorMessage) *errorMessage = QStringLiteral("无效的 DO 通道或设备未打开");
        return false;
    }

    // 1) make sure the pin is an output
    unsigned char config = readReg(RegConfig);
    if (config & (1 << bit)) {
        config &= ~(1 << bit);
        writeReg(RegConfig, config);
    }

    // 2) set / clear the output latch
    unsigned char out = readReg(RegOutput);
    if (value)
        out |= (unsigned char)(1 << bit);
    else
        out &= (unsigned char)~(1 << bit);
    writeReg(RegOutput, out);
    return true;
}

int Tca9554::readDo(int index)
{
    int bit = doBit(index);
    if (!m_open || bit < 0)
        return -1;
    return readBit(RegOutput, bit);
}

int Tca9554::readDi(int index)
{
    int bit = diBit(index);
    if (!m_open || bit < 0)
        return -1;

    // Defensive: make sure the pin is configured as an input.
    unsigned char config = readReg(RegConfig);
    if (!(config & (1 << bit))) {
        config |= (unsigned char)(1 << bit);
        writeReg(RegConfig, config);
    }
    return readBit(RegInput, bit);
}

int Tca9554::readRegister(int reg)
{
    if (!m_open || reg < 0 || reg > 0xFF)
        return -1;
    return readReg((unsigned char)reg);
}
