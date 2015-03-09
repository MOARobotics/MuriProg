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

#include "PICData.h"
#include "Bootloader.h"

PICData::PICData() {
	// PIC18F46J50 data ranges
	QuickAdd(PROGRAM_MEM, 60416, 4096);
	QuickAdd(CONFIG_MEM, 8, 65528);	
}

// Simple way to add an item to the ranges array
void PICData::QuickAdd(unsigned char type, unsigned int size, unsigned int startAddress)
{
	unsigned int i;
	PICData::MemoryRange tmp;
	tmp.type = type;
	tmp.dataBufferLength = size * Bootloader::bytesPerAddressFLASH;
	tmp.pDataBuffer = new unsigned char[tmp.dataBufferLength];
	memset(&tmp.pDataBuffer[0], 0xFF, tmp.dataBufferLength);
	tmp.start = startAddress;
	tmp.end = tmp.start + tmp.dataBufferLength;

	for(i = 0; i < tmp.dataBufferLength; i++)
		tmp.pDataBuffer[i] = 0xFF;

	ranges.append(tmp);
}
PICData::~PICData() {}
