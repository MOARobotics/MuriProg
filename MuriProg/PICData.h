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

#ifndef PICDEVICE_H
#define PICDEVICE_H

#include <QVector>

// Types of PIC memory regions
#define PROGRAM_MEM			0x01
#define EEPROM_MEM			0x02
#define CONFIG_MEM       0x03
#define USERID_MEM       0x04
#define END_OF_TYPES_LIST   0xFF

// Provides a representation of microcontroller device memory contents.
class PICData
{
    public:
        PICData();
        ~PICData();

        struct MemoryRange
        {
            unsigned char type;
            unsigned int start;
            unsigned int end;
            unsigned int dataBufferLength;
            unsigned char* pDataBuffer;
        };

        QList<PICData::MemoryRange> ranges;

		void PICData::QuickAdd(unsigned char type, unsigned int size, unsigned int startAddress);
};

#endif // end PICDATA_H
