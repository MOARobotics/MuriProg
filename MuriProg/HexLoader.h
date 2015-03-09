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

#ifndef HEXLOADER_H
#define HEXLOADER_H

// Includes
#include <QString>
#include "PICData.h"
#include "Bootloader.h"

/*!
 * Reads HEX files into an in-memory PICData object.
 */
class HexLoader
{
public:
	// Enums
	// Error codes related to loading hex files.
	// Taken from Device.h in Microchip Libraries for Applications v2013_12_20
    enum ErrorCode { 
		Success = 0,
		CouldNotOpenFile,
		NoneInRange,
		ErrorInHexFile,
		InsufficientMemory
	};

    // Intel 32-bit Hex File Record Types
	// http://en.wikipedia.org/wiki/Intel_HEX#Record_types
    enum hexRecord
    {
        DATA = 0x00,
        END_OF_FILE = 0x01,
        EXT_SEGMENT = 0x02,
        EXT_LINEAR = 0x04
    };
	
	// Constructor/Destructor
    HexLoader(void);
    ~HexLoader(void);    

	// Members
    bool endOfFileRecordPresent;    // hex file does have an end of file record    
    bool fileExceedsFlash;      // hex file records exceed device memory constraints	

	// Methods
	ErrorCode ImportHexFile(QString fileName, PICData* pData, Bootloader* bootDevice);
	unsigned int GetDeviceAddress(unsigned int hexAddress, Bootloader* bootDevice, PICData* pData, unsigned char& type, bool& includedInProgrammableRange, bool& addressWasEndofRange, unsigned int& bytesPerAddressAndType, unsigned int& endDeviceAddressofRegion, unsigned char*& pcBuffer);
};

#endif // IMPORTEXPORTHEX_H
