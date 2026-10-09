//-----------------------------------------------------------------------------
//  TCA9554 / TCA9554A 8-bit I2C GPIO expander helper
//
//  Address : 0x40                       (see GPIO.txt)
//  Bit map : DI1=7  DI2=6  DI3=5  DI4=4 (input  pin, read only)
//            DO1=3  DO2=2  DO3=1  DO4=0 (output pin, controllable)
//
//  The chip is reached through the vendor library "SvApiLib" (Seavo / 信步):
//    - Linux   : SvApiLibInit / SvApiLibUnInit / SvSmbReadByte / SvSmbWriteByte
//    - Windows : SvApiLibx64.dll (SvApiLibInitialize / SvSmbReadByte / ...)
//-----------------------------------------------------------------------------
#ifndef TCA9554_H
#define TCA9554_H

#include <QString>

class Tca9554
{
public:
    static const int kDiCount = 4;   // DI1..DI4
    static const int kDoCount = 4;   // DO1..DO4
    static const unsigned char kDefaultAddress = 0x40;

    static int diBit(int index);     // index 0..3  ->  7,6,5,4
    static int doBit(int index);     // index 0..3  ->  3,2,1,0

    explicit Tca9554(unsigned char address = kDefaultAddress);
    ~Tca9554();

    // Load the vendor driver / library and initialise the SMBus.
    bool open(QString *errorMessage = nullptr);
    void close();

    bool isOpen() const { return m_open; }
    unsigned char address() const { return m_address; }

    // Configure DI pins as inputs and DO pins as outputs (config register).
    bool configureDirections(QString *errorMessage = nullptr);

    //---- DO : controllable output ------------------------------------------
    bool writeDo(int index, int value, QString *errorMessage = nullptr);
    int  readDo(int index);          // 0 / 1, or -1 on error

    //---- DI : read-only input ----------------------------------------------
    int  readDi(int index);          // 0 / 1, or -1 on error

    //---- raw register access (for the debug read-out) -----------------------
    int  readRegister(int reg);      // 0..255, or -1 if closed

private:
    // TCA9554 register addresses
    enum Reg { RegInput = 0x00, RegOutput = 0x01, RegPolarity = 0x02, RegConfig = 0x03 };

    unsigned char readReg(unsigned char reg);
    void          writeReg(unsigned char reg, unsigned char value);
    int           readBit(unsigned char reg, int bit);

    unsigned char m_address;
    bool          m_open;
};

#endif // TCA9554_H
