/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

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

#include "abc.h"
#include "flashutils.h"
#include "asobject.h"
#include "class.h"
#include "compat.h"
#include "parsing/amf3_generator.h"
#include "argconv.h"
#include "flash/errors/flasherrors.h"
#include <sstream>
#include <zlib.h>
#include <glib.h>

using namespace std;
using namespace lightspark;

SET_NAMESPACE("flash.utils");

REGISTER_CLASS_NAME(Endian);
REGISTER_CLASS_NAME(IDataInput);
REGISTER_CLASS_NAME(IDataOutput);
REGISTER_CLASS_NAME(ByteArray);
REGISTER_CLASS_NAME(Timer);
REGISTER_CLASS_NAME(Dictionary);
REGISTER_CLASS_NAME(Proxy);

#define BA_CHUNK_SIZE 4096

const char* Endian::littleEndian = "littleEndian";
const char* Endian::bigEndian = "bigEndian";

void Endian::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setVariableByQName("LITTLE_ENDIAN","",Class<ASString>::getInstanceS(littleEndian),DECLARED_TRAIT);
	c->setVariableByQName("BIG_ENDIAN","",Class<ASString>::getInstanceS(bigEndian),DECLARED_TRAIT);
}

void IDataInput::linkTraits(Class_base* c)
{
	lookupAndLink(c,"bytesAvailable","flash.utils:IDataInput");
	lookupAndLink(c,"endian","flash.utils:IDataInput");
	lookupAndLink(c,"objectEncoding","flash.utils:IDataInput");
	lookupAndLink(c,"readBoolean","flash.utils:IDataInput");
	lookupAndLink(c,"readByte","flash.utils:IDataInput");
	lookupAndLink(c,"readBytes","flash.utils:IDataInput");
	lookupAndLink(c,"readDouble","flash.utils:IDataInput");
	lookupAndLink(c,"readFloat","flash.utils:IDataInput");
	lookupAndLink(c,"readInt","flash.utils:IDataInput");
	lookupAndLink(c,"readMultiByte","flash.utils:IDataInput");
	lookupAndLink(c,"readObject","flash.utils:IDataInput");
	lookupAndLink(c,"readShort","flash.utils:IDataInput");
	lookupAndLink(c,"readUnsignedByte","flash.utils:IDataInput");
	lookupAndLink(c,"readUnsignedInt","flash.utils:IDataInput");
	lookupAndLink(c,"readUnsignedShort","flash.utils:IDataInput");
	lookupAndLink(c,"readUTF","flash.utils:IDataInput");
	lookupAndLink(c,"readUTFBytes","flash.utils:IDataInput");
}

void IDataOutput::linkTraits(Class_base* c)
{
	lookupAndLink(c,"endian","flash.utils:IDataOutput");
	lookupAndLink(c,"objectEncoding","flash.utils:IDataOutput");
	lookupAndLink(c,"writeBoolean","flash.utils:IDataOutput");
	lookupAndLink(c,"writeByte","flash.utils:IDataOutput");
	lookupAndLink(c,"writeBytes","flash.utils:IDataOutput");
	lookupAndLink(c,"writeDouble","flash.utils:IDataOutput");
	lookupAndLink(c,"writeFloat","flash.utils:IDataOutput");
	lookupAndLink(c,"writeInt","flash.utils:IDataOutput");
	lookupAndLink(c,"writeMultiByte","flash.utils:IDataOutput");
	lookupAndLink(c,"writeObject","flash.utils:IDataOutput");
	lookupAndLink(c,"writeShort","flash.utils:IDataOutput");
	lookupAndLink(c,"writeUnsignedInt","flash.utils:IDataOutput");
	lookupAndLink(c,"writeUTF","flash.utils:IDataOutput");
	lookupAndLink(c,"writeUTFBytes","flash.utils:IDataOutput");
}

ByteArray::ByteArray(uint8_t* b, uint32_t l):bytes(b),real_len(l),len(l),position(0),littleEndian(false),objectEncoding(ObjectEncoding::AMF3)
{
}

ByteArray::ByteArray(const ByteArray& b):ASObject(b),real_len(b.len),len(b.len),position(b.position),littleEndian(b.littleEndian),objectEncoding(b.objectEncoding)
{
	assert_and_throw(position==0);
	bytes = (uint8_t*) malloc(len);
	assert_and_throw(bytes);
	memcpy(bytes,b.bytes,len);
}

ByteArray::~ByteArray()
{
	if(bytes)
		free(bytes);
}

void ByteArray::sinit(Class_base* c)
{
	c->setConstructor(NULL);
	c->setSuper(Class<ASObject>::getRef());

	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(_getLength),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(_setLength),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("bytesAvailable","",Class<IFunction>::getFunction(_getBytesAvailable),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("position","",Class<IFunction>::getFunction(_getPosition),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("position","",Class<IFunction>::getFunction(_setPosition),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("endian","",Class<IFunction>::getFunction(_getEndian),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("endian","",Class<IFunction>::getFunction(_setEndian),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("objectEncoding","",Class<IFunction>::getFunction(_getObjectEncoding),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("objectEncoding","",Class<IFunction>::getFunction(_setObjectEncoding),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("defaultObjectEncoding","",Class<IFunction>::getFunction(_getDefaultObjectEncoding),GETTER_METHOD,false);
	c->setDeclaredMethodByQName("defaultObjectEncoding","",Class<IFunction>::getFunction(_setDefaultObjectEncoding),SETTER_METHOD,false);
	getSys()->staticByteArrayDefaultObjectEncoding = ObjectEncoding::DEFAULT;
	c->setDeclaredMethodByQName("clear","",Class<IFunction>::getFunction(clear),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("compress","",Class<IFunction>::getFunction(_compress),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uncompress","",Class<IFunction>::getFunction(_uncompress),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("deflate","",Class<IFunction>::getFunction(_deflate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("inflate","",Class<IFunction>::getFunction(_inflate),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readBoolean","",Class<IFunction>::getFunction(readBoolean),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readBytes","",Class<IFunction>::getFunction(readBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readByte","",Class<IFunction>::getFunction(readByte),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readDouble","",Class<IFunction>::getFunction(readDouble),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readFloat","",Class<IFunction>::getFunction(readFloat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readInt","",Class<IFunction>::getFunction(readInt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readMultiByte","",Class<IFunction>::getFunction(readMultiByte),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readShort","",Class<IFunction>::getFunction(readShort),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUnsignedByte","",Class<IFunction>::getFunction(readUnsignedByte),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUnsignedInt","",Class<IFunction>::getFunction(readUnsignedInt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUnsignedShort","",Class<IFunction>::getFunction(readUnsignedShort),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readObject","",Class<IFunction>::getFunction(readObject),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUTF","",Class<IFunction>::getFunction(readUTF),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("readUTFBytes","",Class<IFunction>::getFunction(readUTFBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeBoolean","",Class<IFunction>::getFunction(writeBoolean),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeUTF","",Class<IFunction>::getFunction(writeUTF),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeUTFBytes","",Class<IFunction>::getFunction(writeUTFBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeBytes","",Class<IFunction>::getFunction(writeBytes),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeByte","",Class<IFunction>::getFunction(writeByte),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeDouble","",Class<IFunction>::getFunction(writeDouble),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeFloat","",Class<IFunction>::getFunction(writeFloat),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeInt","",Class<IFunction>::getFunction(writeInt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeMultiByte","",Class<IFunction>::getFunction(writeMultiByte),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeUnsignedInt","",Class<IFunction>::getFunction(writeUnsignedInt),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeObject","",Class<IFunction>::getFunction(writeObject),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("writeShort","",Class<IFunction>::getFunction(writeShort),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(ByteArray::_toString),DYNAMIC_TRAIT);

	c->addImplementedInterface(InterfaceClass<IDataInput>::getClass());
	IDataInput::linkTraits(c);
	c->addImplementedInterface(InterfaceClass<IDataOutput>::getClass());
	IDataOutput::linkTraits(c);
}

void ByteArray::buildTraits(ASObject* o)
{
}

uint8_t* ByteArray::getBuffer(unsigned int size, bool enableResize)
{
	// The first allocation is exactly the size we need,
	// the subsequent reallocations happen in increments of BA_CHUNK_SIZE bytes
	if(bytes==NULL)
	{
		len=size;
		real_len=len;
		bytes = (uint8_t*) malloc(len);
	}
	else if(enableResize==false)
	{
		assert_and_throw(size<=len);
	}
	else if(real_len<size) // && enableResize==true
	{
		while(real_len < size)
			real_len += BA_CHUNK_SIZE;
		// Reallocate the buffer, in chunks of BA_CHUNK_SIZE bytes
		uint8_t* bytes2 = (uint8_t*) realloc(bytes, real_len);
		assert_and_throw(bytes2);
		bytes = bytes2;
		len=size;
		bytes=bytes2;
	}
	else if(len<size)
	{
		len=size;
	}
	return bytes;
}

uint16_t ByteArray::endianIn(uint16_t value)
{
	if(littleEndian)
		return GUINT16_TO_LE(value);
	else
		return GUINT16_TO_BE(value);
}

uint32_t ByteArray::endianIn(uint32_t value)
{
	if(littleEndian)
		return GUINT32_TO_LE(value);
	else
		return GUINT32_TO_BE(value);
}

uint64_t ByteArray::endianIn(uint64_t value)
{
	if(littleEndian)
		return GUINT64_TO_LE(value);
	else
		return GUINT64_TO_BE(value);
}

uint16_t ByteArray::endianOut(uint16_t value)
{
	if(littleEndian)
		return GUINT16_FROM_LE(value);
	else
		return GUINT16_FROM_BE(value);
}

uint32_t ByteArray::endianOut(uint32_t value)
{
	if(littleEndian)
		return GUINT32_FROM_LE(value);
	else
		return GUINT32_FROM_BE(value);
}

uint64_t ByteArray::endianOut(uint64_t value)
{
	if(littleEndian)
		return GUINT64_FROM_LE(value);
	else
		return GUINT64_FROM_BE(value);
}

uint32_t ByteArray::getPosition() const
{
	return position;
}

ASFUNCTIONBODY(ByteArray,_getPosition)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	return abstract_i(th->getPosition());
}

void ByteArray::setPosition(uint32_t p)
{
	position=p;
}

ASFUNCTIONBODY(ByteArray,_setPosition)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	uint32_t pos=args[0]->toUInt();
	th->setPosition(pos);
	return NULL;
}

ASFUNCTIONBODY(ByteArray,_getEndian)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	if(th->littleEndian)
		return Class<ASString>::getInstanceS(Endian::littleEndian);
	else
		return Class<ASString>::getInstanceS(Endian::bigEndian);
}

ASFUNCTIONBODY(ByteArray,_setEndian)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	if(args[0]->toString() == Endian::littleEndian)
		th->littleEndian = true;
	else if(args[0]->toString() == Endian::bigEndian)
		th->littleEndian = false;
	return NULL;
}

ASFUNCTIONBODY(ByteArray,_getObjectEncoding)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	return abstract_ui(th->objectEncoding);
}

ASFUNCTIONBODY(ByteArray,_setObjectEncoding)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	uint32_t value;
	ARG_UNPACK(value);
	if(value!=ObjectEncoding::AMF0 && value!=ObjectEncoding::AMF3)
		throw Class<ArgumentError>::getInstanceS("Error #2008: Parameter objectEncoding must be one of the accepted values.");

	th->objectEncoding=value;
	return NULL;
}

ASFUNCTIONBODY(ByteArray,_getDefaultObjectEncoding)
{
	return abstract_i(getSys()->staticNetConnectionDefaultObjectEncoding);
}

ASFUNCTIONBODY(ByteArray,_setDefaultObjectEncoding)
{
	assert_and_throw(argslen == 1);
	int32_t value = args[0]->toInt();
	if(value == 0)
		getSys()->staticByteArrayDefaultObjectEncoding = ObjectEncoding::AMF0;
	else if(value == 3)
		getSys()->staticByteArrayDefaultObjectEncoding = ObjectEncoding::AMF3;
	else
		throw RunTimeException("Invalid object encoding");
	return NULL;
}

ASFUNCTIONBODY(ByteArray,_setLength)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==1);
	uint32_t newLen=args[0]->toInt();
	if(newLen==th->len) //Nothing to do
		return NULL;
	uint32_t prevLen = th->len;
	uint8_t* newBytes= (uint8_t*) realloc(th->bytes, newLen);
	assert_and_throw(newBytes);
	th->bytes = newBytes;
	th->len = newLen;
	th->real_len = newLen;
	if(prevLen<newLen)
	{
		//Extend
		memset(th->bytes+prevLen,0,newLen-prevLen);
	}
	return NULL;
}

ASFUNCTIONBODY(ByteArray,_getLength)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	return abstract_i(th->len);
}

ASFUNCTIONBODY(ByteArray,_getBytesAvailable)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	return abstract_i(th->len-th->position);
}

ASFUNCTIONBODY(ByteArray,readBoolean)
{
	ByteArray* th=static_cast<ByteArray*>(obj);

	uint8_t ret;
	if(!th->readByte(ret))
	{
		throw Class<EOFError>::getInstanceS("Error #2030: End of file was encountered.");
	}

	return abstract_b(ret!=0);
}

ASFUNCTIONBODY(ByteArray,readBytes)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	//Validate parameters
	assert_and_throw(argslen>=1 && argslen<=3);
	assert_and_throw(args[0]->getClass()->isSubClass(Class<ByteArray>::getClass()));

	ByteArray* out=Class<ByteArray>::cast(args[0]);
	uint32_t offset=0;
	uint32_t length=0;
	if(argslen>=2)
		offset=args[1]->toInt();
	if(argslen==3)
		length=args[2]->toInt();
	//TODO: Support offset (offset is in the destination!)
	if(offset!=0)
	{
		throw UnsupportedException("offset in ByteArray::readBytes");
	}

	if(length == 0)
		length = th->len;

	//Error checks
	if(length > th->len)
	{
		throw Class<EOFError>::getInstanceS("Error #2030: End of file was encountered.");
	}
	uint8_t* buf=out->getBuffer(length,true);
	memcpy(buf,th->bytes+th->position,length);
	th->position+=length;

	return NULL;
}

bool ByteArray::readUTF(tiny_string& ret)
{
	uint16_t stringLen;
	if(!readShort(stringLen))
		return false;
	if(len < (position+stringLen))
		return false;
	//Very inefficient copy
	//TODO: optmize
	ret=string((char*)bytes+position, (size_t)stringLen);
	position+=stringLen;
	return true;
}

ASFUNCTIONBODY(ByteArray,readUTF)
{
	ByteArray* th=static_cast<ByteArray*>(obj);

	if(th->len < th->position+2)
	{
		throw Class<EOFError>::getInstanceS("Error #2030: End of file was encountered.");
	}

	uint16_t length;
	memcpy(&length,th->bytes+th->position,2);
	th->position+=2;

	if(th->position+length > th->len)
	{
		throw Class<EOFError>::getInstanceS("Error #2030: End of file was encountered.");
	}

	uint8_t *bufStart=th->bytes+th->position;
	th->position+=length;
	return Class<ASString>::getInstanceS((char *)bufStart,length);
}

ASFUNCTIONBODY(ByteArray,readUTFBytes)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	uint32_t length;

	ARG_UNPACK (length);
	if(th->position+length > th->len)
	{
		throw Class<EOFError>::getInstanceS("Error #2030: End of file was encountered.");
	}

	uint8_t *bufStart=th->bytes+th->position;
	th->position+=length;
	return Class<ASString>::getInstanceS((char *)bufStart,length);
}

void ByteArray::writeUTF(const tiny_string& str)
{
	getBuffer(position+str.numBytes()+2,true);
	uint16_t numBytes=endianIn((uint16_t)str.numBytes());
	memcpy(bytes+position,&numBytes,2);
	memcpy(bytes+position+2,str.raw_buf(),str.numBytes());
	position+=str.numBytes()+2;
}

ASFUNCTIONBODY(ByteArray,writeUTF)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	//Validate parameters
	assert_and_throw(argslen==1);
	assert_and_throw(args[0]->getObjectType()==T_STRING);
	ASString* str=Class<ASString>::cast(args[0]);
	th->writeUTF(str->data);
	return NULL;
}

ASFUNCTIONBODY(ByteArray,writeUTFBytes)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	//Validate parameters
	assert_and_throw(argslen==1);
	assert_and_throw(args[0]->getObjectType()==T_STRING);
	ASString* str=Class<ASString>::cast(args[0]);
	th->getBuffer(th->position+str->data.numBytes(),true);
	memcpy(th->bytes+th->position,str->data.raw_buf(),str->data.numBytes());
	th->position+=str->data.numBytes();

	return NULL;
}

ASFUNCTIONBODY(ByteArray,writeMultiByte)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	tiny_string value;
	tiny_string charset;
	ARG_UNPACK(value)(charset);

	// TODO: should convert from UTF-8 to charset
	LOG(LOG_NOT_IMPLEMENTED, "ByteArray.writeMultiByte doesn't convert charset");

	th->getBuffer(th->position+value.numBytes(),true);
	memcpy(th->bytes+th->position,value.raw_buf(),value.numBytes());
	th->position+=value.numBytes();

	return NULL;
}

uint32_t ByteArray::writeObject(ASObject* obj)
{
	//Return the length of the serialized object

	//TODO: support AMF0
	assert_and_throw(objectEncoding==ObjectEncoding::AMF3);
	//TODO: support custom serialization
	map<tiny_string, uint32_t> stringMap;
	map<const ASObject*, uint32_t> objMap;
	uint32_t oldPosition=position;
	obj->serialize(this, stringMap, objMap);
	return position-oldPosition;
}

ASFUNCTIONBODY(ByteArray,writeObject)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	//Validate parameters
	assert_and_throw(argslen==1);
	th->writeObject(args[0]);

	return NULL;
}

void ByteArray::writeShort(uint16_t val)
{
	int16_t value2 = endianIn(val);
	getBuffer(position+2,true);
	memcpy(bytes+position,&value2,2);
	position+=2;
}

ASFUNCTIONBODY(ByteArray,writeShort)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	int32_t value;
	ARG_UNPACK(value);

	th->writeShort((static_cast<uint16_t>(value & 0xffff)));
	return NULL;
}

ASFUNCTIONBODY(ByteArray,writeBytes)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	//Validate parameters
	assert_and_throw(argslen>=1 && argslen<=3);
	assert_and_throw(args[0]->getClass()->isSubClass(Class<ByteArray>::getClass()));
	ByteArray* out=Class<ByteArray>::cast(args[0]);
	uint32_t offset=0;
	uint32_t length=0;
	if(argslen>=2)
		offset=args[1]->toUInt();
	if(argslen==3)
		length=args[2]->toUInt();

	// We need to clamp offset to the beginning of the bytes array
	if(offset > out->getLength()-1)
		offset = 0;
	// We need to clamp length to the end of the bytes array
	if(length > out->getLength()-offset)
		length = 0;

	//If the length is 0 the whole buffer must be copied
	if(length == 0)
		length=(out->getLength()-offset);
	uint8_t* buf=out->getBuffer(offset+length,false);
	th->getBuffer(th->position+length,true);
	memcpy(th->bytes+th->position,buf+offset,length);
	th->position+=length;

	return NULL;
}

void ByteArray::writeByte(uint8_t b)
{
	getBuffer(position+1,true);
	bytes[position++] = b;
}

ASFUNCTIONBODY(ByteArray,writeByte)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==1);

	int32_t value=args[0]->toInt();

	th->writeByte(value&0xff);

	return NULL;
}

ASFUNCTIONBODY(ByteArray,writeBoolean)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	bool b;
	ARG_UNPACK (b);

	if (b)
		th->writeByte(1);
	else
		th->writeByte(0);

	return NULL;
}

ASFUNCTIONBODY(ByteArray,writeDouble)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==1);

	double value = args[0]->toNumber();
	uint64_t value2=th->endianIn(*reinterpret_cast<uint64_t*>(&value));

	th->getBuffer(th->position+8,true);
	memcpy(th->bytes+th->position,&value2,8);
	th->position+=8;

	return NULL;
}

ASFUNCTIONBODY(ByteArray,writeFloat)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==1);

	float value = args[0]->toNumber();
	uint32_t value2=th->endianIn(*reinterpret_cast<uint32_t*>(&value));

	th->getBuffer(th->position+4,true);
	memcpy(th->bytes+th->position,&value2,4);
	th->position+=4;

	return NULL;
}

ASFUNCTIONBODY(ByteArray,writeInt)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==1);

	uint32_t value=th->endianIn(static_cast<uint32_t>(args[0]->toInt()));

	th->getBuffer(th->position+4,true);
	memcpy(th->bytes+th->position,&value,4);
	th->position+=4;

	return NULL;
}

void ByteArray::writeUnsignedInt(uint32_t val)
{
	getBuffer(position+4,true);
	memcpy(bytes+position,&val,4);
	position+=4;
}

ASFUNCTIONBODY(ByteArray,writeUnsignedInt)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==1);

	uint32_t value=th->endianIn(args[0]->toUInt());
	th->writeUnsignedInt(value);
	return NULL;
}

bool ByteArray::readByte(uint8_t& b)
{
	if (len <= position)
		return false;

	b=bytes[position++];
	return true;
}

bool ByteArray::readU29(uint32_t& ret)
{
	ret=0;
	for(uint32_t i=0;i<4;i++)
	{
		if (len <= position)
			return false;

		uint8_t tmp=bytes[position++];
		if(i<3)
		{
			ret|=((tmp&0x7f) << i*7);
			if((tmp&0x80)==0)
				break;
		}
		else
		{
			ret|=(tmp << 21);
			//Sign extend
			if(tmp&0x80)
				ret|=0xe0000000;
		}
	}
	return true;
}

ASFUNCTIONBODY(ByteArray, readByte)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==0);

	uint8_t ret;
	if(!th->readByte(ret))
	{
		throw Class<EOFError>::getInstanceS("Error #2030: End of file was encountered.");
	}
	return abstract_i((int8_t)ret);
}

ASFUNCTIONBODY(ByteArray,readDouble)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==0);

	if(th->len < th->position+8)
	{
		throw Class<EOFError>::getInstanceS("Error #2030: End of file was encountered.");
	}

	uint64_t ret;
	memcpy(&ret,th->bytes+th->position,8);
	th->position+=8;
	ret = th->endianOut(ret);

	return abstract_d(*reinterpret_cast<double*>(&ret));
}

ASFUNCTIONBODY(ByteArray,readFloat)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==0);

	if(th->len < th->position+4)
	{
		throw Class<EOFError>::getInstanceS("Error #2030: End of file was encountered.");
	}

	uint32_t ret;
	memcpy(&ret,th->bytes+th->position,4);
	th->position+=4;
	ret = th->endianOut(ret);

	return abstract_d(*reinterpret_cast<float*>(&ret));
}

ASFUNCTIONBODY(ByteArray,readInt)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==0);

	if(th->len < th->position+4)
	{
		throw Class<EOFError>::getInstanceS("Error #2030: End of file was encountered.");
	}

	uint32_t ret;
	memcpy(&ret,th->bytes+th->position,4);
	th->position+=4;

	return abstract_i((int32_t)th->endianOut(ret));
}

bool ByteArray::readShort(uint16_t& ret)
{
	if (len < position+2)
		return false;

	uint16_t tmp;
	memcpy(&tmp,bytes+position,2);
	ret=endianOut(tmp);
	position+=2;
	return true;
}

ASFUNCTIONBODY(ByteArray,readShort)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==0);

	uint16_t ret;
	if(!th->readShort(ret))
	{
		throw Class<EOFError>::getInstanceS("Error #2030: End of file was encountered.");
	}

	return abstract_i((int16_t)ret);
}

ASFUNCTIONBODY(ByteArray,readUnsignedByte)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==0);

	uint8_t ret;
	if (!th->readByte(ret))
	{
		throw Class<EOFError>::getInstanceS("Error #2030: End of file was encountered.");
	}
	return abstract_ui(ret);
}

bool ByteArray::readUnsignedInt(uint32_t& ret)
{
	if(len < position+4)
		return false;

	uint32_t tmp;
	memcpy(&tmp,bytes+position,4);
	ret=endianOut(tmp);
	position+=4;
	return true;
}

ASFUNCTIONBODY(ByteArray,readUnsignedInt)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==0);

	uint32_t ret;
	if(!th->readUnsignedInt(ret))
		throw Class<EOFError>::getInstanceS("Error #2030: End of file was encountered.");

	return abstract_ui(ret);
}

ASFUNCTIONBODY(ByteArray,readUnsignedShort)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==0);

	uint16_t ret;
	if(!th->readShort(ret))
	{
		throw Class<EOFError>::getInstanceS("Error #2030: End of file was encountered.");
	}

	return abstract_ui(ret);
}

ASFUNCTIONBODY(ByteArray,readMultiByte)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	uint32_t strlen;
	tiny_string charset;
	ARG_UNPACK(strlen)(charset);

	if(th->len < th->position+strlen)
	{
		throw Class<EOFError>::getInstanceS("Error #2030: End of file was encountered.");
	}

	// TODO: should convert from charset to UTF-8
	LOG(LOG_NOT_IMPLEMENTED, "ByteArray.readMultiByte doesn't convert charset");
	return Class<ASString>::getInstanceS((char*)th->bytes+th->position,strlen);
}

ASFUNCTIONBODY(ByteArray,readObject)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	assert_and_throw(argslen==0);
	if(th->bytes==NULL)
	{
		throw Class<EOFError>::getInstanceS("Error #2030: End of file was encountered.");
	}
	assert_and_throw(th->objectEncoding==ObjectEncoding::AMF3);
	std::vector<ASObject*> ret;
	Amf3Deserializer d(th);
	try
	{
		d.generateObjects(ret);
	}
	catch(LightsparkException& e)
	{
		LOG(LOG_ERROR,"Exception caught while parsing AMF3: " << e.cause);
		//TODO: throw AS exception
	}

	if(ret.size()==0)
	{
		LOG(LOG_ERROR,"No objects in the AMF3 data. Returning Undefined");
		return new Undefined;
	}
	if(ret.size()>1)
	{
		LOG(LOG_ERROR,"More than one object in the AMF3 data. Returning the first");
		for(uint32_t i=1;i<ret.size();i++)
			ret[i]->decRef();
	}
	return ret[0];
}

ASFUNCTIONBODY(ByteArray,_toString)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	//TODO: check for Byte Order Mark
	return Class<ASString>::getInstanceS((char*)th->bytes,th->len);
}

bool ByteArray::hasPropertyByMultiname(const multiname& name, bool considerDynamic)
{
	if(considerDynamic==false)
		return ASObject::hasPropertyByMultiname(name, considerDynamic);

	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
		return ASObject::hasPropertyByMultiname(name, considerDynamic);

	return index<len;
}

_NR<ASObject> ByteArray::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if((opt & ASObject::SKIP_IMPL)!=0 || !Array::isValidMultiname(name,index))
		return ASObject::getVariableByMultiname(name,opt);

	if(index<len)
	{
		uint8_t value = bytes[index];
		return _MNR(abstract_ui(static_cast<uint32_t>(value)));
	}
	else
		return _MNR(new Undefined);
}

int32_t ByteArray::getVariableByMultiname_i(const multiname& name)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
		return ASObject::getVariableByMultiname_i(name);

	if(index<len)
	{
		uint8_t value = bytes[index];
		return static_cast<uint32_t>(value);
	}
	else
		return _MNR(new Undefined);
}

void ByteArray::setVariableByMultiname(const multiname& name, ASObject* o)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!Array::isValidMultiname(name,index))
		return ASObject::setVariableByMultiname(name,o);

	if(index>=len)
	{
		uint32_t prevLen = len;
		getBuffer(index+1, true);
		// Fill the gap between the end of the current data and the index with zeros
		memset(bytes+prevLen, 0, index-prevLen);
	}

	// Fill the byte pointed to by index with the truncated uint value of the object.
	uint8_t value = static_cast<uint8_t>(o->toUInt() & 0xff);
	bytes[index] = value;

	o->decRef();
}

void ByteArray::setVariableByMultiname_i(const multiname& name, int32_t value)
{
	setVariableByMultiname(name, abstract_i(value));
}

void ByteArray::acquireBuffer(uint8_t* buf, int bufLen)
{
	if(bytes)
		free(bytes);
	bytes=buf;
	real_len=bufLen;
	len=bufLen;
	position=0;
}

void ByteArray::writeU29(uint32_t val)
{
	for(uint32_t i=0;i<4;i++)
	{
		uint8_t b;
		if(i<3)
		{
			b=val&0x7f;
			val>>=7;
			if(val)
				b|=0x80;
		}
		else
			b=val&0xff;

		writeByte(b);
		if(val==0)
			break;
	}
}

void ByteArray::writeStringVR(map<tiny_string, uint32_t>& stringMap, const tiny_string& s)
{
	const uint32_t len=s.numBytes();

	//Check if the string is already in the map
	auto it=stringMap.find(s);
	if(it!=stringMap.end())
	{
		//The first bit must be 0, the next 29 bits
		//store the index of the string in the map
		writeU29(it->second << 1);
	}
	else
	{
		//The AMF3 spec says that the empty string is never sent by reference
		//So add the string to the map only if it's not the empty string
		if(len)
			stringMap.insert(make_pair(s, stringMap.size()));

		//The first bit must be 1, the next 29 bits
		//store the number of bytes of the string
		writeU29((len<<1) | 1);

		getBuffer(position+len,true);
		memcpy(bytes+position,s.raw_buf(),len);
		position+=len;
	}
}

void ByteArray::compress_zlib()
{
	if(len==0)
		return;

	unsigned long buflen=compressBound(len);
	uint8_t *compressed=(uint8_t*) malloc(buflen);
	assert_and_throw(compressed);

	if(compress(compressed, &buflen, bytes, len)!=Z_OK)
	{
		free(compressed);
		throw RunTimeException("zlib compress failed");
	}

	acquireBuffer(compressed, buflen);
	position=buflen;
}

void ByteArray::uncompress_zlib()
{
	z_stream strm;
	int status;

	if(len==0)
		return;

	strm.zalloc=Z_NULL;
	strm.zfree=Z_NULL;
	strm.opaque=Z_NULL;
	strm.avail_in=len;
	strm.next_in=bytes;
	strm.total_out=0;
	status=inflateInit(&strm);
	if(status==Z_VERSION_ERROR)
		throw Class<IOError>::getInstanceS("not valid compressed data");
	else if(status!=Z_OK)
		throw RunTimeException("zlib uncompress failed");

	vector<uint8_t> buf(3*len);
	do
	{
		strm.next_out=&buf[strm.total_out];
		strm.avail_out=buf.size()-strm.total_out;
		status=inflate(&strm, Z_NO_FLUSH);

		if(status!=Z_OK && status!=Z_STREAM_END)
		{
			inflateEnd(&strm);
			throw Class<IOError>::getInstanceS("not valid compressed data");
		}

		if(strm.avail_out==0)
			buf.resize(buf.size()+len);
	} while(status!=Z_STREAM_END);

	inflateEnd(&strm);

	len=strm.total_out;
	real_len = len;
	uint8_t* bytes2=(uint8_t*) realloc(bytes, len);
	assert_and_throw(bytes2);
	bytes = bytes2;
	memcpy(bytes, &buf[0], len);
	position=0;
}

ASFUNCTIONBODY(ByteArray,_compress)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	tiny_string algorithm;
	ARG_UNPACK(algorithm, "zlib");
	if(algorithm=="zlib")
		th->compress_zlib();
	else
		throw Class<ASError>::getInstanceS("Unsupported algorithm");

	return NULL;
}

ASFUNCTIONBODY(ByteArray,_uncompress)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	tiny_string algorithm;
	ARG_UNPACK(algorithm, "zlib");
	if(algorithm=="zlib")
		th->uncompress_zlib();
	else
		throw Class<ASError>::getInstanceS("Unsupported algorithm");

	return NULL;
}

ASFUNCTIONBODY(ByteArray,_deflate)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	th->compress_zlib();
	return NULL;
}

ASFUNCTIONBODY(ByteArray,_inflate)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	th->uncompress_zlib();
	return NULL;
}

ASFUNCTIONBODY(ByteArray,clear)
{
	ByteArray* th=static_cast<ByteArray*>(obj);
	if(th->bytes)
		free(th->bytes);
	th->bytes = NULL;
	th->len=0;
	th->real_len=0;
	th->position=0;
	return NULL;
}

void Timer::tick()
{
	//This will be executed once if repeatCount was originally 1
	//Otherwise it's executed until stopMe is set to true
	this->incRef();
	getVm()->addEvent(_MR(this),_MR(Class<TimerEvent>::getInstanceS("timer")));

	if(repeatCount!=0)
	{
		currentCount++;
		if(currentCount==repeatCount)
		{
			this->incRef();
			getVm()->addEvent(_MR(this),_MR(Class<TimerEvent>::getInstanceS("timerComplete")));
			stopMe=true;
			running=false;
		}
	}
}

void Timer::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<EventDispatcher>::getRef());
	c->setDeclaredMethodByQName("currentCount","",Class<IFunction>::getFunction(_getCurrentCount),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("repeatCount","",Class<IFunction>::getFunction(_getRepeatCount),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("repeatCount","",Class<IFunction>::getFunction(_setRepeatCount),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("running","",Class<IFunction>::getFunction(_getRunning),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("delay","",Class<IFunction>::getFunction(_getDelay),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("delay","",Class<IFunction>::getFunction(_setDelay),SETTER_METHOD,true);
	c->setDeclaredMethodByQName("start","",Class<IFunction>::getFunction(start),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("reset","",Class<IFunction>::getFunction(reset),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("stop","",Class<IFunction>::getFunction(stop),NORMAL_METHOD,true);
}

ASFUNCTIONBODY(Timer,_constructor)
{
	EventDispatcher::_constructor(obj,NULL,0);
	Timer* th=static_cast<Timer*>(obj);

	th->delay=args[0]->toInt();
	if(argslen>=2)
		th->repeatCount=args[1]->toInt();

	return NULL;
}

ASFUNCTIONBODY(Timer,_getCurrentCount)
{
	Timer* th=static_cast<Timer*>(obj);
	return abstract_i(th->currentCount);
}

ASFUNCTIONBODY(Timer,_getRepeatCount)
{
	Timer* th=static_cast<Timer*>(obj);
	return abstract_i(th->repeatCount);
}

ASFUNCTIONBODY(Timer,_setRepeatCount)
{
	assert_and_throw(argslen==1);
	int32_t count=args[0]->toInt();
	Timer* th=static_cast<Timer*>(obj);
	th->repeatCount=count;
	if(th->repeatCount>0 && th->repeatCount<=th->currentCount)
	{
		getSys()->removeJob(th);
		th->decRef();
		th->running=false;
	}
	return NULL;
}

ASFUNCTIONBODY(Timer,_getRunning)
{
	Timer* th=static_cast<Timer*>(obj);
	return abstract_b(th->running);
}

ASFUNCTIONBODY(Timer,_getDelay)
{
	Timer* th=static_cast<Timer*>(obj);
	return abstract_i(th->delay);
}

ASFUNCTIONBODY(Timer,_setDelay)
{
	assert_and_throw(argslen==1);
	int32_t newdelay = args[0]->toInt();
	if (newdelay<=0)
		throw Class<ASError>::getInstanceS("delay must be positive");

	Timer* th=static_cast<Timer*>(obj);
	th->delay=newdelay;

	return NULL;
}

ASFUNCTIONBODY(Timer,start)
{
	Timer* th=static_cast<Timer*>(obj);
	if(th->running)
		return NULL;
	th->running=true;
	th->stopMe=false;
	th->incRef();
	if(th->repeatCount==1)
		getSys()->addWait(th->delay,th);
	else
		getSys()->addTick(th->delay,th);
	return NULL;
}

ASFUNCTIONBODY(Timer,reset)
{
	Timer* th=static_cast<Timer*>(obj);
	if(th->running)
	{
		//This spin waits if the timer is running right now
		getSys()->removeJob(th);
		//NOTE: although no new events will be sent now there might be old events in the queue.
		//Is this behaviour right?
		th->currentCount=0;
		//This is not anymore used by the timer, so it can die
		th->decRef();
		th->running=false;
	}
	return NULL;
}

ASFUNCTIONBODY(Timer,stop)
{
	Timer* th=static_cast<Timer*>(obj);
	if(th->running)
	{
		//This spin waits if the timer is running right now
		getSys()->removeJob(th);
		//NOTE: although no new events will be sent now there might be old events in the queue.
		//Is this behaviour right?

		//This is not anymore used by the timer, so it can die
		th->decRef();
		th->running=false;
	}
	return NULL;
}

ASFUNCTIONBODY(lightspark,getQualifiedClassName)
{
	//CHECK: what to do if ns is empty
	ASObject* target=args[0];
	Class_base* c;
	SWFOBJECT_TYPE otype=target->getObjectType();
	if(otype==T_NULL)
		return Class<ASString>::getInstanceS("null");
	else if(otype==T_UNDEFINED)
		// Testing shows that this really returns "void"!
		return Class<ASString>::getInstanceS("void");
	else if(otype!=T_CLASS)
	{
		assert_and_throw(target->getClass());
		c=target->getClass();
	}
	else
		c=static_cast<Class_base*>(target);

	return Class<ASString>::getInstanceS(c->getQualifiedClassName());
}

ASFUNCTIONBODY(lightspark,getQualifiedSuperclassName)
{
	//CHECK: what to do is ns is empty
	ASObject* target=args[0];
	Class_base* c;
	if(target->getObjectType()!=T_CLASS)
	{
		assert_and_throw(target->getClass());
		c=target->getClass()->super.getPtr();
	}
	else
		c=static_cast<Class_base*>(target)->super.getPtr();

	assert_and_throw(c);

	return Class<ASString>::getInstanceS(c->getQualifiedClassName());
}

ASFUNCTIONBODY(lightspark,getDefinitionByName)
{
	assert_and_throw(args && argslen==1);
	const tiny_string& tmp=args[0]->toString();
	multiname name;
	name.name_type=multiname::NAME_STRING;
	name.ns.push_back(nsNameAndKind("",NAMESPACE)); //TODO: set type

	stringToQName(tmp,name.name_s,name.ns[0].name);

	LOG(LOG_CALLS,_("Looking for definition of ") << name);
	ASObject* target;
	ASObject* o=getGlobal()->getVariableAndTargetByMultiname(name,target);

	//TODO: should raise an exception, for now just return undefined	
	if(o==NULL)
	{
		LOG(LOG_ERROR,_("Definition for '") << name << _("' not found."));
		return new Undefined;
	}

	assert_and_throw(o->getObjectType()==T_CLASS);

	LOG(LOG_CALLS,_("Getting definition for ") << name);
	o->incRef();
	return o;
}

ASFUNCTIONBODY(lightspark,describeType)
{
	assert_and_throw(argslen==1);
	return args[0]->describeType();
}

ASFUNCTIONBODY(lightspark,getTimer)
{
	uint64_t ret=compat_msectiming() - getSys()->startTime;
	return abstract_i(ret);
}

void Dictionary::finalize()
{
	ASObject::finalize();
	data.clear();
}

void Dictionary::sinit(Class_base* c)
{
	c->setConstructor(Class<IFunction>::getFunction(_constructor));
	c->setSuper(Class<ASObject>::getRef());
}

void Dictionary::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Dictionary,_constructor)
{
	return NULL;
}

void Dictionary::setVariableByMultiname_i(const multiname& name, int32_t value)
{
	assert_and_throw(implEnable);
	Dictionary::setVariableByMultiname(name,abstract_i(value));
}

void Dictionary::setVariableByMultiname(const multiname& name, ASObject* o)
{
	assert_and_throw(implEnable);
	if(name.name_type==multiname::NAME_OBJECT)
	{
		_R<ASObject> name_o(name.name_o);
		//This is ugly, but at least we are sure that we own name_o
		multiname* tmp=const_cast<multiname*>(&name);
		tmp->name_o=NULL;

		map<_R<ASObject>, _R<ASObject> >::iterator it=data.find(name_o);
		if(it!=data.end())
			it->second=_MR(o);
		else
			data.insert(make_pair(name_o,_MR(o)));
	}
	else
	{
		//Primitive types _must_ be handled by the normal ASObject path
		//REFERENCE: Dictionary Object on AS3 reference
		assert(name.name_type==multiname::NAME_STRING ||
			name.name_type==multiname::NAME_INT ||
			name.name_type==multiname::NAME_NUMBER);
		ASObject::setVariableByMultiname(name, o);
	}
}

bool Dictionary::deleteVariableByMultiname(const multiname& name)
{
	assert_and_throw(implEnable);

	if(name.name_type==multiname::NAME_OBJECT)
	{
		_R<ASObject> name_o(name.name_o);
		//This is ugly, but at least we are sure that we own name_o
		multiname* tmp=const_cast<multiname*>(&name);
		tmp->name_o=NULL;

		map<_R<ASObject>, _R<ASObject> >::iterator it=data.find(name_o);
		if(it != data.end())
		{
			data.erase(it);
			return true;
		}
		return false;
	}
	else
	{
		//Primitive types _must_ be handled by the normal ASObject path
		//REFERENCE: Dictionary Object on AS3 reference
		assert(name.name_type==multiname::NAME_STRING ||
			name.name_type==multiname::NAME_INT ||
			name.name_type==multiname::NAME_NUMBER);
		return ASObject::deleteVariableByMultiname(name);
	}
}

_NR<ASObject> Dictionary::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	if((opt & ASObject::SKIP_IMPL)==0 && implEnable)
	{
		if(name.name_type==multiname::NAME_OBJECT)
		{
			_R<ASObject> name_o(name.name_o);

			map<_R<ASObject>, _R<ASObject> >::iterator it=data.find(name_o);
			if(it != data.end())
			{
				//This is ugly, but at least we are sure that we own name_o
				multiname* tmp=const_cast<multiname*>(&name);
				tmp->name_o=NULL;
				return it->second;
			}
			else
			{
				//Make sure name_o gets not destroyed, it's still owned by name
				name_o->incRef();
				return NullRef;
			}
		}
		else
		{
			//Primitive types _must_ be handled by the normal ASObject path
			//REFERENCE: Dictionary Object on AS3 reference
			assert(name.name_type==multiname::NAME_STRING ||
				name.name_type==multiname::NAME_INT ||
				name.name_type==multiname::NAME_NUMBER);
			return ASObject::getVariableByMultiname(name, opt);
		}
	}
	//Try with the base implementation
	return ASObject::getVariableByMultiname(name, opt);
}

bool Dictionary::hasPropertyByMultiname(const multiname& name, bool considerDynamic)
{
	if(considerDynamic==false)
		return ASObject::hasPropertyByMultiname(name, considerDynamic);

	if(name.name_type==multiname::NAME_OBJECT)
	{
		_R<ASObject> name_o(name.name_o);

		map<_R<ASObject>, _R<ASObject> >::iterator it=data.find(name_o);
		return it != data.end();
	}
	else
	{
		//Primitive types _must_ be handled by the normal ASObject path
		//REFERENCE: Dictionary Object on AS3 reference
		assert(name.name_type==multiname::NAME_STRING ||
			name.name_type==multiname::NAME_INT ||
			name.name_type==multiname::NAME_NUMBER);
		return ASObject::hasPropertyByMultiname(name, considerDynamic);
	}
}

uint32_t Dictionary::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	if(cur_index<data.size())
		return cur_index+1;
	else
	{
		//Fall back on object properties
		uint32_t ret=ASObject::nextNameIndex(cur_index-data.size());
		if(ret==0)
			return 0;
		else
			return ret+data.size();

	}
}

_R<ASObject> Dictionary::nextName(uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=data.size())
	{
		map<_R<ASObject>,_R<ASObject> >::iterator it=data.begin();
		for(unsigned int i=1;i<index;i++)
			++it;

		return it->first;
	}
	else
	{
		//Fall back on object properties
		return ASObject::nextName(index-data.size());
	}
}

_R<ASObject> Dictionary::nextValue(uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=data.size())
	{
		map<_R<ASObject>,_R<ASObject> >::iterator it=data.begin();
		for(unsigned int i=1;i<index;i++)
			++it;

		return it->second;
	}
	else
	{
		//Fall back on object properties
		return ASObject::nextValue(index-data.size());
	}
}

tiny_string Dictionary::toString()
{
	std::stringstream retstr;
	retstr << "{";
	map<_R<ASObject>,_R<ASObject> >::iterator it=data.begin();
	while(it != data.end())
	{
		if(it != data.begin())
			retstr << ", ";
		retstr << "{" << it->first->toString() << ", " << it->second->toString() << "}";
		++it;
	}
	retstr << "}";

	return retstr.str();
}

void Proxy::sinit(Class_base* c)
{
	//c->constructor=Class<IFunction>::getFunction(_constructor);
	c->setConstructor(NULL);
}

void Proxy::buildTraits(ASObject* o)
{
}

void Proxy::setVariableByMultiname(const multiname& name, ASObject* o)
{
	//If a variable named like this already exist, use that
	if(ASObject::hasPropertyByMultiname(name, true) || !implEnable)
	{
		ASObject::setVariableByMultiname(name,o);
		return;
	}

	//Check if there is a custom setter defined, skipping implementation to avoid recursive calls
	multiname setPropertyName;
	setPropertyName.name_type=multiname::NAME_STRING;
	setPropertyName.name_s="setProperty";
	setPropertyName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	_NR<ASObject> proxySetter=getVariableByMultiname(setPropertyName,ASObject::SKIP_IMPL);

	if(proxySetter.isNull())
	{
		ASObject::setVariableByMultiname(name,o);
		return;
	}

	assert_and_throw(proxySetter->getObjectType()==T_FUNCTION);

	IFunction* f=static_cast<IFunction*>(proxySetter.getPtr());

	//Well, I don't how to pass multiname to an as function. I'll just pass the name as a string
	ASObject* args[2];
	args[0]=Class<ASString>::getInstanceS(name.name_s);
	args[1]=o;
	//We now suppress special handling
	implEnable=false;
	LOG(LOG_CALLS,_("Proxy::setProperty"));
	incRef();
	_R<ASObject> ret=_MR( f->call(this,args,2) );
	assert_and_throw(ret->is<Undefined>());
	implEnable=true;
}

_NR<ASObject> Proxy::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	//It seems that various kind of implementation works only with the empty namespace
	assert_and_throw(name.ns.size()>0);
	if(name.ns[0].name!="" || ASObject::hasPropertyByMultiname(name, true) || !implEnable || (opt & ASObject::SKIP_IMPL)!=0)
		return ASObject::getVariableByMultiname(name,opt);

	//Check if there is a custom getter defined, skipping implementation to avoid recursive calls
	multiname getPropertyName;
	getPropertyName.name_type=multiname::NAME_STRING;
	getPropertyName.name_s="getProperty";
	getPropertyName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	_NR<ASObject> o=getVariableByMultiname(getPropertyName,ASObject::SKIP_IMPL);

	if(o.isNull())
		return ASObject::getVariableByMultiname(name,opt);

	assert_and_throw(o->getObjectType()==T_FUNCTION);

	IFunction* f=static_cast<IFunction*>(o.getPtr());

	//Well, I don't how to pass multiname to an as function. I'll just pass the name as a string
	ASObject* arg=Class<ASString>::getInstanceS(name.name_s);
	//We now suppress special handling
	implEnable=false;
	LOG(LOG_CALLS,"Proxy::getProperty");
	incRef();
	_NR<ASObject> ret=_MNR(f->call(this,&arg,1));
	implEnable=true;
	return ret;
}

bool Proxy::hasPropertyByMultiname(const multiname& name, bool considerDynamic)
{
	//If a variable named like this already exist, use that
	if(ASObject::hasPropertyByMultiname(name, considerDynamic) || !implEnable)
	{
		return ASObject::deleteVariableByMultiname(name);
	}

	//Check if there is a custom deleter defined, skipping implementation to avoid recursive calls
	multiname hasPropertyName;
	hasPropertyName.name_type=multiname::NAME_STRING;
	hasPropertyName.name_s="hasProperty";
	hasPropertyName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	_NR<ASObject> proxyHasProperty=getVariableByMultiname(hasPropertyName,ASObject::SKIP_IMPL);

	if(proxyHasProperty.isNull())
	{
		return false;
	}

	assert_and_throw(proxyHasProperty->getObjectType()==T_FUNCTION);

	IFunction* f=static_cast<IFunction*>(proxyHasProperty.getPtr());

	//Well, I don't how to pass multiname to an as function. I'll just pass the name as a string
	ASObject* arg=Class<ASString>::getInstanceS(name.name_s);
	//We now suppress special handling
	implEnable=false;
	LOG(LOG_CALLS,_("Proxy::hasProperty"));
	incRef();
	_NR<ASObject> ret=_MNR(f->call(this,&arg,1));
	implEnable=true;
	Boolean* b = static_cast<Boolean*>(ret.getPtr());
	return b->val;
}
bool Proxy::deleteVariableByMultiname(const multiname& name)
{
	//If a variable named like this already exist, use that
	if(ASObject::hasPropertyByMultiname(name, true) || !implEnable)
	{
		return ASObject::deleteVariableByMultiname(name);
	}

	//Check if there is a custom deleter defined, skipping implementation to avoid recursive calls
	multiname deletePropertyName;
	deletePropertyName.name_type=multiname::NAME_STRING;
	deletePropertyName.name_s="deleteProperty";
	deletePropertyName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	_NR<ASObject> proxyDeleter=getVariableByMultiname(deletePropertyName,ASObject::SKIP_IMPL);

	if(proxyDeleter.isNull())
	{
		return ASObject::deleteVariableByMultiname(name);
	}

	assert_and_throw(proxyDeleter->getObjectType()==T_FUNCTION);

	IFunction* f=static_cast<IFunction*>(proxyDeleter.getPtr());

	//Well, I don't how to pass multiname to an as function. I'll just pass the name as a string
	ASObject* arg=Class<ASString>::getInstanceS(name.name_s);
	//We now suppress special handling
	implEnable=false;
	LOG(LOG_CALLS,_("Proxy::deleteProperty"));
	incRef();
	_NR<ASObject> ret=_MNR(f->call(this,&arg,1));
	implEnable=true;
	Boolean* b = static_cast<Boolean*>(ret.getPtr());
	return b->val;
}

uint32_t Proxy::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	LOG(LOG_CALLS,"Proxy::nextNameIndex");
	//Check if there is a custom enumerator, skipping implementation to avoid recursive calls
	multiname nextNameIndexName;
	nextNameIndexName.name_type=multiname::NAME_STRING;
	nextNameIndexName.name_s="nextNameIndex";
	nextNameIndexName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	_NR<ASObject> o=getVariableByMultiname(nextNameIndexName,ASObject::SKIP_IMPL);
	assert_and_throw(!o.isNull() && o->getObjectType()==T_FUNCTION);
	IFunction* f=static_cast<IFunction*>(o.getPtr());
	ASObject* arg=abstract_i(cur_index);
	this->incRef();
	ASObject* ret=f->call(this,&arg,1);
	uint32_t newIndex=ret->toInt();
	ret->decRef();
	return newIndex;
}

_R<ASObject> Proxy::nextName(uint32_t index)
{
	assert_and_throw(implEnable);
	LOG(LOG_CALLS, _("Proxy::nextName"));
	//Check if there is a custom enumerator, skipping implementation to avoid recursive calls
	multiname nextNameName;
	nextNameName.name_type=multiname::NAME_STRING;
	nextNameName.name_s="nextName";
	nextNameName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	_NR<ASObject> o=getVariableByMultiname(nextNameName,ASObject::SKIP_IMPL);
	assert_and_throw(!o.isNull() && o->getObjectType()==T_FUNCTION);
	IFunction* f=static_cast<IFunction*>(o.getPtr());
	ASObject* arg=abstract_i(index);
	incRef();
	return _MR(f->call(this,&arg,1));
}

_R<ASObject> Proxy::nextValue(uint32_t index)
{
	assert_and_throw(implEnable);
	LOG(LOG_CALLS, _("Proxy::nextValue"));
	//Check if there is a custom enumerator, skipping implementation to avoid recursive calls
	multiname nextValueName;
	nextValueName.name_type=multiname::NAME_STRING;
	nextValueName.name_s="nextValue";
	nextValueName.ns.push_back(nsNameAndKind(flash_proxy,NAMESPACE));
	_NR<ASObject> o=getVariableByMultiname(nextValueName,ASObject::SKIP_IMPL);
	assert_and_throw(!o.isNull() && o->getObjectType()==T_FUNCTION);
	IFunction* f=static_cast<IFunction*>(o.getPtr());
	ASObject* arg=abstract_i(index);
	incRef();
	return _MR(f->call(this,&arg,1));
}

ASFUNCTIONBODY(lightspark,setInterval)
{
	assert_and_throw(argslen >= 2 && args[0]->getObjectType()==T_FUNCTION);

	//Build arguments array
	ASObject** callbackArgs = g_newa(ASObject*,argslen-2);
	uint32_t i;
	for(i=0; i<argslen-2; i++)
	{
		callbackArgs[i] = args[i+2];
		//incRef all passed arguments
		args[i+2]->incRef();
	}

	//incRef the function
	args[0]->incRef();
	IFunction* callback=static_cast<IFunction*>(args[0]);
	//Add interval through manager
	uint32_t id = getSys()->intervalManager->setInterval(_MR(callback), callbackArgs, argslen-2, _MR(new Null), args[1]->toInt());
	return abstract_i(id);
}

ASFUNCTIONBODY(lightspark,setTimeout)
{
	assert_and_throw(argslen >= 2);

	//Build arguments array
	ASObject** callbackArgs = g_newa(ASObject*,argslen-2);
	uint32_t i;
	for(i=0; i<argslen-2; i++)
	{
		callbackArgs[i] = args[i+2];
		//incRef all passed arguments
		args[i+2]->incRef();
	}

	//incRef the function
	args[0]->incRef();
	IFunction* callback=static_cast<IFunction*>(args[0]);
	//Add timeout through manager
	uint32_t id = getSys()->intervalManager->setTimeout(_MR(callback), callbackArgs, argslen-2, _MR(new Null), args[1]->toInt());
	return abstract_i(id);
}

ASFUNCTIONBODY(lightspark,clearInterval)
{
	assert_and_throw(argslen == 1);
	getSys()->intervalManager->clearInterval(args[0]->toInt(), IntervalRunner::INTERVAL, true);
	return NULL;
}

ASFUNCTIONBODY(lightspark,clearTimeout)
{
	assert_and_throw(argslen == 1);
	getSys()->intervalManager->clearInterval(args[0]->toInt(), IntervalRunner::TIMEOUT, true);
	return NULL;
}

IntervalRunner::IntervalRunner(IntervalRunner::INTERVALTYPE _type, uint32_t _id, _R<IFunction> _callback, ASObject** _args,
		const unsigned int _argslen, _R<ASObject> _obj, uint32_t _interval):
	type(_type), id(_id), callback(_callback),argslen(_argslen),obj(_obj),interval(_interval)
{
	args = new ASObject*[argslen];
	for(uint32_t i=0; i<argslen; i++)
		args[i] = _args[i];
}

IntervalRunner::~IntervalRunner()
{
	for(uint32_t i=0; i<argslen; i++)
		args[i]->decRef();
	delete[] args;
}

void IntervalRunner::tick() 
{
	//incRef all arguments
	uint32_t i;
	for(i=0; i < argslen; i++)
	{
		args[i]->incRef();
	}
	_R<FunctionEvent> event(new FunctionEvent(callback, obj, args, argslen));
	getVm()->addEvent(NullRef,event);
	if(type == TIMEOUT)
	{
		//TODO: IntervalRunner deletes itself. Is this allowed?
		//Delete ourselves from the active intervals list
		getSys()->intervalManager->clearInterval(id, TIMEOUT, false);
		//No actions may be performed after this point
	}
}

IntervalManager::IntervalManager() : currentID(1)
{
}

IntervalManager::~IntervalManager()
{
	//Run through all running intervals and remove their tickjob, delete their intervalRunner and erase their entry
	std::map<uint32_t,IntervalRunner*>::iterator it = runners.begin();
	while(it != runners.end())
	{
		getSys()->removeJob((*it).second);
		delete((*it).second);
		runners.erase(it++);
	}
}

uint32_t IntervalManager::setInterval(_R<IFunction> callback, ASObject** args, const unsigned int argslen, _R<ASObject> obj, uint32_t interval)
{
	Mutex::Lock l(mutex);

	uint32_t id = getFreeID();
	IntervalRunner* runner = new IntervalRunner(IntervalRunner::INTERVAL, id, callback, args, argslen, obj, interval);

	//Add runner as tickjob
	getSys()->addTick(interval, runner);
	//Add runner to map
	runners[id] = runner;
	//Increment currentID
	currentID++;

	return currentID-1;
}
uint32_t IntervalManager::setTimeout(_R<IFunction> callback, ASObject** args, const unsigned int argslen, _R<ASObject> obj, uint32_t interval)
{
	Mutex::Lock l(mutex);

	uint32_t id = getFreeID();
	IntervalRunner* runner = new IntervalRunner(IntervalRunner::TIMEOUT, id, callback, args, argslen, obj, interval);

	//Add runner as waitjob
	getSys()->addWait(interval, runner);
	//Add runner to map
	runners[id] = runner;
	//increment currentID
	currentID++;

	return currentID-1;
}

uint32_t IntervalManager::getFreeID()
{
	//At the first run every currentID will be available. But eventually the currentID will wrap around.
	//Thats why we need to check if the currentID isn't used yet
	while(currentID == 0 || runners.count(currentID) != 0)
		currentID++;
	return currentID;
}

void IntervalManager::clearInterval(uint32_t id, IntervalRunner::INTERVALTYPE type, bool removeJob)
{
	Mutex::Lock l(mutex);

	std::map<uint32_t,IntervalRunner*>::iterator it = runners.find(id);
	//If the entry exists and the types match, remove its tickjob, delete its intervalRunner and erase their entry
	if(it != runners.end() && (*it).second->getType() == type)
	{
		if(removeJob)
		{
			getSys()->removeJob((*it).second);
		}
		delete (*it).second;
		runners.erase(it);
	}
}
