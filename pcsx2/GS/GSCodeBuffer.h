/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "Pcsx2Defs.h"
#include <vector>

class GSCodeBuffer
{
	std::vector<void*> m_buffers;
	size_t m_blocksize;
	size_t m_pos;
	u8* m_ptr;

public:
	GSCodeBuffer(size_t blocksize = 4096 * 64); // 256k
	virtual ~GSCodeBuffer();

	void* GetBuffer(size_t size);
	void ReleaseBuffer(size_t size);
};
