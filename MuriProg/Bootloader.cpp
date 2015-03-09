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

#include "Bootloader.h"

// Constructor
Bootloader::Bootloader(PICData *data)
{
	// Initialize variables, we know these so we don't
	// have to wait for a query
    picData = data;
}

// Loops through the PICData ranges array and returns true if any of those ranges are EEPROM_MEM
bool Bootloader::EepromPresent(void)
{
    PICData::MemoryRange range;
    foreach(range, picData->ranges)
        if(range.type == EEPROM_MEM) // Check if this range is an EEPROM range
            return true;

    return false;
}
