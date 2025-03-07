/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __SIF_H__
#define __SIF_H__

#define NOMINMAX

#include "Utilities/MemcpyFast.h"

#include <algorithm>

#undef min
#undef max

#define FIFO_SIF_W 128

// Despite its name, this is actually the IOP's DMAtag, which itself also contains
// the EE's DMAtag in its upper 64 bits.  Note that only the lower 24 bits of 'data' is
// the IOP's chain transfer address (loaded into MADR).  Bits 30 and 31 are transfer stop
// bits of some sort.
struct sifData
{
	s32 data;
	s32 words;

	tDMA_TAG	tag_lo;		// EE DMA tag
	tDMA_TAG	tag_hi;		// EE DMA tag
};

struct sifFifo
{
	u32 data[FIFO_SIF_W];
	u32 junk[4];
	s32 readPos;
	s32 writePos;
	s32 size;

	s32 sif_free()
	{
		return FIFO_SIF_W - size;
	}

	void write(u32 *from, int words)
	{
		if (words > 0)
		{
			const int wP0 = std::min((FIFO_SIF_W - writePos), words);
			const int wP1 = words - wP0;
			if (size < 4)
				memcpy(&junk[size], from, (4 - size) << 2);
			memcpy(&data[writePos], from, wP0 << 2);
			memcpy(&data[0], &from[wP0], wP1 << 2);

			writePos = (writePos + words) & (FIFO_SIF_W - 1);
			size += words;
		}
	}

	void writeJunk(int words)
	{
		if (words > 0)
		{
			const int wP0 = std::min((FIFO_SIF_W - writePos), words);
			const int wP1 = words - wP0;

			memcpy(&data[writePos], &junk[4-words], wP0 << 2);
			memcpy(&data[0], &junk[(4 - words)+wP0], wP1 << 2);

			writePos = (writePos + words) & (FIFO_SIF_W - 1);
			size += words;
		}
	}

	void read(u32 *to, int words)
	{
		if (words > 0)
		{
			const int wP0 = std::min((FIFO_SIF_W - readPos), words);
			const int wP1 = words - wP0;

			memcpy(to, &data[readPos], wP0 << 2);
			memcpy(&to[wP0], &data[0], wP1 << 2);

			readPos = (readPos + words) & (FIFO_SIF_W - 1);
			size -= words;
		}
	}
	void clear()
	{
		memzero(data);
		readPos = 0;
		writePos = 0;
		size = 0;
	}
};

struct sif_ee
{
	bool end; // Only used for EE.
	bool busy;

	s32 cycles;
};

struct sif_iop
{
	bool end;
	bool busy;

	s32 cycles;
	u32 writeJunk;

	s32 counter; // Used to keep track of how much is left in IOP.
	struct sifData data; // Only used in IOP.
};

struct _sif
{
	sifFifo fifo; // Used in both.
	sif_ee ee;
	sif_iop iop;
};

extern _sif sif0, sif1, sif2;

extern void sifReset();

extern void SIF0Dma();
extern void SIF1Dma();
extern void SIF2Dma();

extern void dmaSIF0();
extern void dmaSIF1();
extern void dmaSIF2();

extern void EEsif0Interrupt();
extern void EEsif1Interrupt();
extern void EEsif2Interrupt();

extern void sif0Interrupt();
extern void sif1Interrupt();
extern void sif2Interrupt();

extern bool ReadFifoSingleWord();

#define sif0data sif0.iop.data.data
#define sif1data sif1.iop.data.data
#define sif2data sif2.iop.data.data

#define sif0words sif0.iop.data.words
#define sif1words sif1.iop.data.words
#define sif2words sif2.iop.data.words

#define sif0tag DMA_TAG(sif0data)
#define sif1tag DMA_TAG(sif1data)
#define sif2tag DMA_TAG(sif2data)

#endif /* __SIF_H__ */
