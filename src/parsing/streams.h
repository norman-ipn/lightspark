/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2013  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef PARSING_STREAMS_H
#define PARSING_STREAMS_H 1

#include "compat.h"
#include "abctypes.h"
#include "swftypes.h"
#include <streambuf>
#include <fstream>
#include <cinttypes>
#include <zlib.h>
#include <lzma.h>

class uncompressing_filter: public std::streambuf
{
protected:
	static const unsigned int BUFFER_LENGTH = 4096;

	// The compressed input data stream
	std::streambuf* backend;
	// Current uncompressed bytes (accessible input sequence in
	// std::streambuf terminology)
	char buffer[BUFFER_LENGTH];
	// Total number of uncompressed read bytes not including the
	// bytes read from the current buffer.
	int consumed;
	bool eof;
	virtual int underflow();
	virtual std::streampos seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode);
	// Abstract function that fills buffer by uncompressing bytes
	// from backend. buffer is empty when this is called. Returns
	// number of bytes written to buffer.
	virtual int fillBuffer()=0;
public:
	uncompressing_filter(std::streambuf* b);
};


class zlib_filter: public uncompressing_filter
{
private:
	z_stream strm;
	// Temporary buffer for data before it is uncompressed
	char compressed_buffer[BUFFER_LENGTH];
protected:
	virtual int fillBuffer();
public:
	zlib_filter(std::streambuf* b);
	~zlib_filter();
};

class liblzma_filter: public uncompressing_filter
{
private:
	lzma_stream strm;
	// Temporary buffer for data before it is uncompressed
	uint8_t compressed_buffer[BUFFER_LENGTH];
protected:
	virtual int fillBuffer();
public:
	liblzma_filter(std::streambuf* b);
	~liblzma_filter();
};

class bytes_buf:public std::streambuf
{
private:
	const uint8_t* buf;
	int len;
public:
	bytes_buf(const uint8_t* b, int l);
	virtual pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode);
};

// A lightweight, istream-like interface for reading from a memory
// buffer.
// 
// This is used in the interpreter for reading bytecode because this
// is faster than istringstream.
class memorystream
{
private:
	const char* const code;
	unsigned int len;
	unsigned int pos;
public:
	lightspark::method_body_info_cache* codecache;
	// Create a stream from a buffer b.
	//
	// The buffer is not copied, so b must continue to exists for
	// the life-time of this memorystream instance.
	memorystream(const char* const b, unsigned int l,lightspark::method_body_info_cache* cc): code(b), len(l), pos(0),codecache(cc) {};
	static void handleError(const char *msg);
	inline unsigned int size() const
	{
		return len;
	}
	
	inline unsigned int tellg() const
	{
		return pos;
	}
	
	inline void seekg(unsigned int offset)
	{
		if (offset > len)
			pos = len;
		else
			pos = offset;
	}
	
	inline void read(char *out, unsigned int nbytes)
	{
		if (pos+nbytes > len)
		{
			memcpy(out, code+pos, len-pos);
			pos = len;
		}
		else
		{
			memcpy(out, code+pos, nbytes);
			pos += nbytes;
		}
	}
	
	inline uint8_t readbyte()
	{
		if (pos < len)
		{
			pos++;
			return code[pos-1];
		}
		else
		{
			pos = len;
			return 0;
		}
	}
	inline uint32_t readu30()
	{
		unsigned int currpos = pos;
		if (codecache[currpos].type == lightspark::method_body_info_cache::CACHE_TYPE_UINTEGER)
		{
			pos = codecache[currpos].nextpos;
			return codecache[currpos].uvalue;
		}
		uint32_t val = readu32();
		if(val&0xc0000000)
			memorystream::handleError("Invalid u30");
		return val;
	}
	inline uint32_t readu32()
	{
		unsigned int currpos = pos;
		
		int i=0;
		uint32_t val=0;
		uint8_t t;
		do
		{
			t = readbyte();
			//No more than 5 bytes should be read
			if(i==28)
			{
				//Only the first 4 bits should be used to reach 32 bits
				if((t&0xf0))
					LOG(LOG_ERROR,"Error in u32");
				val|=((t&0xf)<<i);
				break;
			}
			else
			{
				val|=((t&0x7f)<<i);
				i+=7;
			}
		}
		while(t&0x80);
		codecache[currpos].type = lightspark::method_body_info_cache::CACHE_TYPE_UINTEGER;
		codecache[currpos].uvalue = val;
		codecache[currpos].nextpos = pos;
		return val;
	}
	inline int32_t reads24()
	{
		unsigned int currpos = pos;
		if (codecache[currpos].type == lightspark::method_body_info_cache::CACHE_TYPE_INTEGER)
		{
			pos = codecache[currpos].nextpos;
			return codecache[currpos].ivalue;
		}
		uint32_t val=0;
		read((char*)&val,3);
		int32_t ret = LittleEndianToSignedHost24(val);
		codecache[currpos].type = lightspark::method_body_info_cache::CACHE_TYPE_INTEGER;
		codecache[currpos].ivalue = ret;
		codecache[currpos].nextpos = pos;
		return ret;
	}
};
#endif /* PARSING_STREAMS_H */
