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

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <QString>
#include <QVariant>
#include <QLinkedList>
#include <QList>

#include "PICData.h"

// Provides microcontroller device specific parameters
class Bootloader
{
public:
	// Constructor	
    Bootloader(PICData* data);
	
	// Members	
    static const unsigned int bytesPerPacket = 58;
    static const unsigned int bytesPerWordFLASH = 2;
    static const unsigned int bytesPerWordEEPROM = 1;
    static const unsigned int bytesPerAddressFLASH = 1;
    static const unsigned int bytesPerAddressEEPROM = 1;

	// Methdos
	// See CPP file for descriptions on Methods
    bool EepromPresent(void);	

protected:
	// Members
    PICData *picData;
};

#endif // DEVICE_H
