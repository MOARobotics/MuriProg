/*
 * This file is part of MuriProg.
 *
 * MuriProg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MuriProg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with MuriProg.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef USB_H
#define USB_H

#include <stdint.h>
#include <QThread>
#include <QTimer>
#include "../HidAPi/hidapi.h"
#include "Bootloader.h"

// Bootloader Vendor and Product IDs
#define VID 0x04d8
#define PID 0x003C

#define USB_PACKET_SIZE 64
#define USB_PACKET_SIZE_WITH_REPORT_ID (USB_PACKET_SIZE + 1)

// Packet commands
#define UNLOCK_CONFIG       0x03
#define ERASE_DEVICE        0x04
#define PROGRAM_DEVICE      0x05
#define PROGRAM_COMPLETE    0x06
#define GET_DATA            0x07
#define RESET_DEVICE        0x08
#define SIGN_FLASH			0x09	//The host PC application should send this command after the verify operation has completed successfully.  If checksums are used instead of a true verify (due to ALLOW_GET_DATA_COMMAND being commented), then the host PC application should send SIGN_FLASH command after is has verified the checksums are as exected. The firmware will then program the SIGNATURE_WORD into flash at the SIGNATURE_ADDRESS.
#define ENGAGE_BOOTLOADER	0x0A
#define FIRMWARE_INFO		0x0C    //Used by host PC app to get additional info about the device, beyond the basic NVM layout provided by the query device command

// Maximum number of memory regions that can be bootloaded
#define MAX_DATA_REGIONS    0x02
#define MAX_ERASE_BLOCK_SIZE 8196   //Increase this in the future if any microcontrollers with bigger than 8196 byte erase block is implemented

// Provides low level HID bootloader communication.
class USB : public QObject
{
	// Qt Macro
    Q_OBJECT

signals:
    void SetProgressBar(int newValue);

protected:
    hid_device *usb_device;
    bool connected;

public:

    explicit USB();
    ~USB();

	// Members
    static const int SyncWaitTime;

	// Enums and structs
    enum ErrorCode
    {
        Success = 0, NotConnected, Fail, IncorrectCommand, Timeout, Other = 0xFF
    };

    QString ErrorString(ErrorCode errorCode) const;

	// http://stackoverflow.com/questions/3318410/pragma-pack-effect
    #pragma pack(1)
    struct MemoryRegion
    {
        unsigned char type;
        uint32_t address;
        uint32_t size;
    };
	
    //Structure for the response to the GetFirmwareInfo command
    union FirmwareInfo
    {
        unsigned char command;
        struct
        {
            unsigned char command;
            uint16_t bootloaderVersion;
            uint16_t applicationVersion;
            uint32_t signatureAddress;
            uint16_t signatureValue;
            uint32_t erasePageSize;
            unsigned char pad[USB_PACKET_SIZE_WITH_REPORT_ID - 15];
        };
    };

	// Packet written to the onboard MCU
    struct WritePacket
    {
        unsigned char report;
        unsigned char command;
        union {
            uint32_t address;
            unsigned char LockedValue;
        };
        unsigned char bytesPerPacket;
        unsigned char data[58];
    };

	// Packet read from the onboard MCU
    struct ReadPacket
    {
        unsigned char command;
        uint32_t address;
        unsigned char bytesPerPacket;
        unsigned char data[59];
    };

    #pragma pack()

	// Methods
	ErrorCode EngageBootloader(void);
    void PollUSB(void);
    ErrorCode open(void);
    void close(void);
    bool isConnected(void);
    void Reset(void);
    ErrorCode GetData(uint32_t address, unsigned char bytesPerPacket, unsigned char bytesPerAddress, unsigned char bytesPerWord, uint32_t endAddress, unsigned char *data);
    ErrorCode Program(uint32_t address, unsigned char bytesPerPacket, unsigned char bytesPerAddress, unsigned char bytesPerWord, uint32_t endAddress, unsigned char *data);	
    ErrorCode Erase(void);
    //ErrorCode LockUnlockConfig(bool lock);
    ErrorCode ReadFirmwareInfo(FirmwareInfo* firmwareInfo);
    ErrorCode SignFlash(void);
    ErrorCode SendPacket(unsigned char *data, int size);
    ErrorCode ReceivePacket(unsigned char *data, int size);
};

#endif // COMM_H
