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

#include <QFile>
#include "HexLoader.h"
#include "Bootloader.h"


HexLoader::HexLoader(void)
{
}

HexLoader::~HexLoader(void)
{
}

/*
 * This function reads a Intel formatted hex file and stores the parsed data into a
 * PICDevice::MemoryRegion buffer, for programming onto the onboard PIC18F46J50.
 */
HexLoader::ErrorCode HexLoader::ImportHexFile(QString fileName, PICData* pData, Bootloader* device)
{
    QFile hexfile(fileName);
    endOfFileRecordPresent = false;
    fileExceedsFlash = false;
	bool ok;
    bool includedInProgrammableRange;
    bool addressWasEndofRange;
    unsigned int segmentAddress = 0;
    unsigned int byteCount;
    unsigned int lineAddress;
    unsigned int deviceAddress;
    unsigned int i;
    unsigned int endDeviceAddressofRegion;
    unsigned int bytesPerAddressAndType;
    unsigned char* pcBuffer = 0;
    bool importedAtLeastOneByte = false;
    unsigned char type;
    hexRecord recordType;
    QString hexByte;
    unsigned int wordByte;
	    
    if (!hexfile.open(QIODevice::ReadOnly | QIODevice::Text))
		return CouldNotOpenFile;

    // Parse the entire hex file, line by line.
    while (!hexfile.atEnd())
    {
        //Fetch a line from the file.
        QByteArray line = hexfile.readLine();

        // Do some error checking on the .hex file contents, to make sure the file is
        // formatted like a legitimate Intel 32-bit formatted .hex file.
        if ((line[0] != ':') || (line.size() < 11))
        {
            // Abort the operation if there's any error
            if(hexfile.isOpen())
                hexfile.close();
            return ErrorInHexFile;
        }
		        
        // Extract info prefix fields
		byteCount = line.mid(1, 2).toInt(&ok, 16);
        lineAddress = segmentAddress + line.mid(3, 4).toInt(&ok, 16);
        recordType = (hexRecord)line.mid(7, 2).toInt(&ok, 16);

        //Error check: Verify checksum byte at the end of the line is valid.
        unsigned int hexLineChecksum = 0;
        for(i = 0; i < (byteCount+4); i++)  // +4 correction is for info prefix bytes
        {
            hexByte = line.mid(1 + (2 * i), 2); // Fetch two bytes
            wordByte = hexByte.toInt(&ok, 16);  //  Merge bytes            
            hexLineChecksum += (unsigned char)wordByte; // Add to checksum
        }
        //Now get the two's complement of the hexLineChecksum.
        hexLineChecksum = 0 - hexLineChecksum;
        hexLineChecksum &= 0xFF;

        //Fetch checksum byte from the .hex file
        hexByte = line.mid(1 + (2 * i), 2);
        wordByte = hexByte.toInt(&ok, 16);
        wordByte &= 0xFF;

        // Compare computed checkum to file checksum
        if(hexLineChecksum != wordByte)
        {            
            //abort the operation if the bytes don't match
            if(hexfile.isOpen())
                hexfile.close();
            return ErrorInHexFile;
        }



        //Check the record type of the hex line, to determine how to continue parsing the data.
        if (recordType == END_OF_FILE)                        // end of file record
        {
            endOfFileRecordPresent = true;
            break;
        }
        else if ((recordType == EXT_SEGMENT) || (recordType == EXT_LINEAR)) // Segment address
        {
            //Error check: Make sure the line contains the correct number of bytes for the specified record type
            if((unsigned int)line.size() >= (11 + (2 * byteCount)))
            {
                //Fetch the payload, which is the upper 4 or 16-bits of the 20-bit or 32-bit hex file address
                segmentAddress = line.mid(9, 4).toInt(&ok, 16);

                //Load the upper bits of the address
                if (recordType == EXT_SEGMENT)
                {
                    segmentAddress <<= 4;
                }
                else
                {
                    segmentAddress <<= 16;
                }

                //Update the line address, now that we know the upper bits are something new.
                lineAddress = segmentAddress + line.mid(3, 4).toInt(&ok, 16);
            }
            else
            {
                //Length appears to be wrong in hex line entry.
                //If an error is detected in the hex file formatting, the safest approach is to
                //abort the operation and force the user to supply a properly formatted hex file.
                if(hexfile.isOpen())
                    hexfile.close();
                return ErrorInHexFile;
            }

        } // end if ((recordType == EXT_SEGMENT) || (recordType == EXT_LINEAR)) // Segment address
        else if (recordType == DATA)                        // Data Record
        {
            //Error check to make sure line is long enough to be consistent with the specified record type
            if ((unsigned int)line.size() < (11 + (2 * byteCount)))
            {
                //If an error is detected in the hex file formatting, the safest approach is to
                //abort the operation and force the user to supply a proper hex file.
                if(hexfile.isOpen())
                    hexfile.close();
                return ErrorInHexFile;
            }


            //For each data payload byte we find in the hex file line, check if it is contained within
            //a progammable region inside the microcontroller.  If so save it.  If not, discard it.
            for(i = 0; i < byteCount; i++)
            {
                //Use the hex file linear byte address, to compute other imformation about the
                //byte/location.  The GetDeviceAddress() function gives us a pointer to
                //the PC RAM buffer byte that will get programmed into the microcontroller, which corresponds
                //to the specified .hex file extended address.
                //The function also returns a boolean letting us know if the address is part of a programmable memory region on the device.
                deviceAddress = GetDeviceAddress(lineAddress + i, device, pData, type, includedInProgrammableRange, addressWasEndofRange, bytesPerAddressAndType, endDeviceAddressofRegion, pcBuffer);
                //Check if the just parsed hex byte was included in one of the microcontroller reported programmable memory regions.
                //If so, save the byte into the proper location in the PC RAM buffer, so it can be programmed later.
                if((includedInProgrammableRange == true) && (pcBuffer != 0)) //Make sure pcBuffer pointer is valid before using it.
                {
                    //Print debug output text to debug window
                    if(i == 0)
                    {
                        qDebug(QString("Importing .hex file line with device address: 0x" + QString::number(deviceAddress, 16)).toLatin1());
                    }

                    //Fetch ASCII encoded payload byte from .hex file and save the byte to our temporary RAM buffer.
                    hexByte = line.mid(9 + (2 * i), 2);  //Fetch two ASCII data payload bytes from the .hex file
                    wordByte = hexByte.toInt(&ok, 16);   //Re-format the above two ASCII bytes into a single binary encoded byte (0x00-0xFF)

                    *pcBuffer = (unsigned char)wordByte;    //Save the .hex file data byte into the PC RAM buffer that holds the data to be programmed
                    importedAtLeastOneByte = true;       //Set flag so we know we imported something successfully.					                    
                }
                else if((includedInProgrammableRange == true) && (pcBuffer == 0))
                {
                    //Previous memory allocation must have failed, or otherwise pcBuffer would not be = 0.
                    //Since the memory allocation failed, we should bug out and let the user know.
                    if(hexfile.isOpen())
                        hexfile.close();
                    return InsufficientMemory;
                }
            }//for(i = 0; i < byteCount; i++)
        } // end else if (recordType == DATA)
    }//while (!hexfile.atEnd())

    //If we get to here, that means we reached the end of the hex file, or we found a END_OF_FILE record in the .hex file.
    if(hexfile.isOpen())
    {
        hexfile.close();
    }

    //Check if we imported any data from the .hex file.
    if(importedAtLeastOneByte == true)
    {
        qDebug(QString("Hex File imported successfully.").toLatin1());
        return Success;
    }
    else
    {
        //If we get to here, we didn't import anything.  The hex file must have been empty or otherwise didn't
        //contain any data that overlaps a device programmable region.  We should let the user know they should
        //supply a better hex file designed for their device.
        return NoneInRange;
    }
}

/* This function checks if the address in the hex file line is contained in one of the
 * programmable regions. If the address from the file is contained in the programmable
 * range, this function returns includedInProgrammableRange = true. This function also
 * returns the effective device address the data should be loaded too.
 *
 * This also returns addressWasEndofRange = true, if the input address coressponded to
 * the very last byte in the programmable range.
 */
unsigned int HexLoader::GetDeviceAddress(unsigned int hexAddress,  Bootloader* bootDevice, PICData* pData, unsigned char& type, bool& includedInProgrammableRange, bool& addressWasEndofRange, unsigned int& bytesPerAddressAndType, unsigned int& endDeviceAddressofRegion, unsigned char*& pcBuffer)
{
    PICData::MemoryRange range;
    unsigned int flashAddress = hexAddress / bootDevice->bytesPerAddressFLASH;
    unsigned int eepromAddress = hexAddress / bootDevice->bytesPerAddressEEPROM;
    unsigned char* pRAMDataBuffer;
    unsigned int byteOffset;

    foreach(range, pData->ranges)
    {
        // Find what address range the hex address seems to contained within (if any, could be none)

		// Check for program data
        if((range.type == PROGRAM_MEM) && (flashAddress >= range.start) && (flashAddress < range.end))
        {
            includedInProgrammableRange = true;
            if(range.start != 0)
            {
                byteOffset = ((flashAddress - range.start) * bootDevice->bytesPerAddressFLASH) + (hexAddress % bootDevice->bytesPerAddressFLASH);
                pRAMDataBuffer = range.pDataBuffer + byteOffset;
                pcBuffer = pRAMDataBuffer;
            }
            else
                pcBuffer = 0;

            type = PROGRAM_MEM;
            bytesPerAddressAndType = bootDevice->bytesPerAddressFLASH;
            endDeviceAddressofRegion = range.end;

            //Check if this was the very last byte of the very last address of the region.
            //We can determine this, using the below check.
            if((flashAddress == (range.end - 1)) && ((hexAddress % bootDevice->bytesPerAddressFLASH) == (bootDevice->bytesPerAddressFLASH - 1)))
                addressWasEndofRange = true;
            else
                addressWasEndofRange = false;

            return flashAddress;
        }

		// Check for EEPROM data
        if((range.type == EEPROM_MEM) && (eepromAddress >= range.start) && (eepromAddress < range.end))
        {
            includedInProgrammableRange = true;
            if(range.start != 0)
            {
                byteOffset = ((eepromAddress - range.start) * bootDevice->bytesPerAddressEEPROM)  + (hexAddress % bootDevice->bytesPerAddressEEPROM);
                pRAMDataBuffer = range.pDataBuffer + byteOffset;
                pcBuffer = pRAMDataBuffer;
            }
            else
                pcBuffer = 0;
            type = EEPROM_MEM;
            bytesPerAddressAndType = bootDevice->bytesPerAddressEEPROM;
            endDeviceAddressofRegion = range.end;

            //Check if this was the very last byte of the very last address of the region.
            //We can determine this, using the below check.
            if((eepromAddress == (range.end - 1)) && ((eepromAddress % bootDevice->bytesPerAddressEEPROM) == (bootDevice->bytesPerAddressEEPROM - 1)))
                addressWasEndofRange = true;
            else
                addressWasEndofRange = false;

            return eepromAddress;
        }
    }
	
    //If we get to here, that means the hex file address that was passed in was not included in any of the
    //device's reported programmable memory regions.
    includedInProgrammableRange = false;
    addressWasEndofRange = false;
    pcBuffer = 0;
    return 0;
}





