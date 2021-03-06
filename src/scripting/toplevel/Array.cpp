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

#include "scripting/toplevel/Array.h"
#include "scripting/abc.h"
#include "scripting/argconv.h"
#include "parsing/amf3_generator.h"
#include "scripting/toplevel/Vector.h"
#include "scripting/toplevel/RegExp.h"
#include "scripting/flash/utils/flashutils.h"

using namespace std;
using namespace lightspark;

Array::Array(Class_base* c):ASObject(c),
	data(std::less<arrayType::key_type>(), reporter_allocator<arrayType::value_type>(c->memoryAccount))
{
	currentsize=0;
	type=T_ARRAY;
}

void Array::sinit(Class_base* c)
{
	CLASS_SETUP(c, ASObject, _constructor, CLASS_DYNAMIC_NOT_FINAL);
	c->isReusable = true;
	c->setVariableByQName("CASEINSENSITIVE","",abstract_di(c->getSystemState(),CASEINSENSITIVE),CONSTANT_TRAIT);
	c->setVariableByQName("DESCENDING","",abstract_di(c->getSystemState(),DESCENDING),CONSTANT_TRAIT);
	c->setVariableByQName("NUMERIC","",abstract_di(c->getSystemState(),NUMERIC),CONSTANT_TRAIT);
	c->setVariableByQName("RETURNINDEXEDARRAY","",abstract_di(c->getSystemState(),RETURNINDEXEDARRAY),CONSTANT_TRAIT);
	c->setVariableByQName("UNIQUESORT","",abstract_di(c->getSystemState(),UNIQUESORT),CONSTANT_TRAIT);

	// properties
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),_getLength),GETTER_METHOD,true);
	c->setDeclaredMethodByQName("length","",Class<IFunction>::getFunction(c->getSystemState(),_setLength),SETTER_METHOD,true);

	// public functions
	c->setDeclaredMethodByQName("concat",AS3,Class<IFunction>::getFunction(c->getSystemState(),_concat,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("every",AS3,Class<IFunction>::getFunction(c->getSystemState(),every,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("filter",AS3,Class<IFunction>::getFunction(c->getSystemState(),filter,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("forEach",AS3,Class<IFunction>::getFunction(c->getSystemState(),forEach,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("indexOf",AS3,Class<IFunction>::getFunction(c->getSystemState(),indexOf,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("lastIndexOf",AS3,Class<IFunction>::getFunction(c->getSystemState(),lastIndexOf,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("join",AS3,Class<IFunction>::getFunction(c->getSystemState(),join,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("map",AS3,Class<IFunction>::getFunction(c->getSystemState(),_map,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("pop",AS3,Class<IFunction>::getFunction(c->getSystemState(),_pop),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("push",AS3,Class<IFunction>::getFunction(c->getSystemState(),_push_as3,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("reverse",AS3,Class<IFunction>::getFunction(c->getSystemState(),_reverse),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("shift",AS3,Class<IFunction>::getFunction(c->getSystemState(),shift),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("slice",AS3,Class<IFunction>::getFunction(c->getSystemState(),slice,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("some",AS3,Class<IFunction>::getFunction(c->getSystemState(),some,1),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("sort",AS3,Class<IFunction>::getFunction(c->getSystemState(),_sort),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("sortOn",AS3,Class<IFunction>::getFunction(c->getSystemState(),sortOn),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("splice",AS3,Class<IFunction>::getFunction(c->getSystemState(),splice,2),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("toLocaleString",AS3,Class<IFunction>::getFunction(c->getSystemState(),_toLocaleString),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("unshift",AS3,Class<IFunction>::getFunction(c->getSystemState(),unshift),NORMAL_METHOD,true);

	c->prototype->setVariableByQName("concat","",Class<IFunction>::getFunction(c->getSystemState(),_concat,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("every","",Class<IFunction>::getFunction(c->getSystemState(),every,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("filter","",Class<IFunction>::getFunction(c->getSystemState(),filter,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("forEach","",Class<IFunction>::getFunction(c->getSystemState(),forEach,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("indexOf","",Class<IFunction>::getFunction(c->getSystemState(),indexOf,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("lastIndexOf","",Class<IFunction>::getFunction(c->getSystemState(),lastIndexOf,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("join","",Class<IFunction>::getFunction(c->getSystemState(),join,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("map","",Class<IFunction>::getFunction(c->getSystemState(),_map,1),DYNAMIC_TRAIT);
	// workaround, pop was encountered not in the AS3 namespace before, need to investigate it further
	c->setDeclaredMethodByQName("pop","",Class<IFunction>::getFunction(c->getSystemState(),_pop),NORMAL_METHOD,true);
	c->prototype->setVariableByQName("pop","",Class<IFunction>::getFunction(c->getSystemState(),_pop),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("push","",Class<IFunction>::getFunction(c->getSystemState(),_push,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("reverse","",Class<IFunction>::getFunction(c->getSystemState(),_reverse),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("shift","",Class<IFunction>::getFunction(c->getSystemState(),shift),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("slice","",Class<IFunction>::getFunction(c->getSystemState(),slice,2),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("some","",Class<IFunction>::getFunction(c->getSystemState(),some,1),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("sort","",Class<IFunction>::getFunction(c->getSystemState(),_sort),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("sortOn","",Class<IFunction>::getFunction(c->getSystemState(),sortOn),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("splice","",Class<IFunction>::getFunction(c->getSystemState(),splice,2),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toLocaleString","",Class<IFunction>::getFunction(c->getSystemState(),_toLocaleString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("toString","",Class<IFunction>::getFunction(c->getSystemState(),_toString),DYNAMIC_TRAIT);
	c->prototype->setVariableByQName("unshift","",Class<IFunction>::getFunction(c->getSystemState(),unshift),DYNAMIC_TRAIT);
}

void Array::buildTraits(ASObject* o)
{
}

ASFUNCTIONBODY(Array,_constructor)
{
	Array* th=static_cast<Array*>(obj);
	th->constructorImpl(args, argslen);
	return NULL;
}

ASFUNCTIONBODY(Array,generator)
{
	Array* th=Class<Array>::getInstanceS(getSys());
	th->constructorImpl(args, argslen);
	return th;
}

void Array::constructorImpl(ASObject* const* args, const unsigned int argslen)
{
	if(argslen==1 && (args[0]->is<Integer>() || args[0]->is<UInteger>() || args[0]->is<Number>()))
	{
		uint32_t size = args[0]->toUInt();
		if ((number_t)size != args[0]->toNumber())
			throwError<RangeError>(kArrayIndexNotIntegerError, Number::toString(args[0]->toNumber()));
		LOG(LOG_CALLS,_("Creating array of length ") << size);
		resize(size);
	}
	else
	{
		LOG(LOG_CALLS,_("Called Array constructor"));
		resize(argslen);
		for(unsigned int i=0;i<argslen;i++)
		{
			args[i]->incRef();
			set(i,_MR(args[i]));
		}
	}
}

ASFUNCTIONBODY(Array,_concat)
{
	Array* th=static_cast<Array*>(obj);
	Array* ret=Class<Array>::getInstanceS(obj->getSystemState());
	
	// copy values into new array
	ret->resize(th->size());
	auto it=th->data.begin();
	for(;it != th->data.end();++it)
	{
		ret->data[it->first]=it->second;
		if(ret->data[it->first].type==DATA_OBJECT && ret->data[it->first].data)
			ret->data[it->first].data->incRef();
	}

	for(unsigned int i=0;i<argslen;i++)
	{
		if (args[i]->is<Array>())
		{
			// Insert the contents of the array argument
			uint64_t oldSize=ret->size();
			Array* otherArray=args[i]->as<Array>();
			auto itother=otherArray->data.begin();
			for(;itother!=otherArray->data.end(); ++itother)
			{
				uint32_t newIndex=ret->size()+itother->first;
				ret->data[newIndex]=itother->second;
				if(ret->data[newIndex].type==DATA_OBJECT && ret->data[newIndex].data)
					ret->data[newIndex].data->incRef();
			}
			ret->resize(oldSize+otherArray->size());
		}
		else
		{
			//Insert the argument
			args[i]->incRef();
			ret->push(_MR(args[i]));
		}
	}

	return ret;
}

ASFUNCTIONBODY(Array,filter)
{
	Array* th=static_cast<Array*>(obj);
	Array* ret=Class<Array>::getInstanceS(obj->getSystemState());
	_NR<IFunction> f;
	ARG_UNPACK(f);
	if (f.isNull())
		return ret;

	ASObject* params[3];
	ASObject *funcRet;

	for(auto it=th->data.begin();it != th->data.end();++it)
	{
		if (it->second.type==DATA_OBJECT)
		{
			params[0] = it->second.data;
			it->second.data->incRef();
		}
		else
			params[0] =abstract_di(obj->getSystemState(),it->second.data_i);
		params[1] = abstract_i(obj->getSystemState(),it->first);
		params[2] = th;
		th->incRef();

		// ensure that return values are the original values
		ASObject *origval = params[0];
		origval->incRef();
		if(argslen==1)
		{
			funcRet=f->call(obj->getSystemState()->getNullRef(), params, 3);
		}
		else
		{
			args[1]->incRef();
			funcRet=f->call(args[1], params, 3);
		}
		if(funcRet)
		{
			if(Boolean_concrete(funcRet))
				ret->push(_MR(origval));
			else
				origval->decRef();
			funcRet->decRef();
		}
	}
	return ret;
}

ASFUNCTIONBODY(Array, some)
{
	Array* th=static_cast<Array*>(obj);
	_NR<IFunction> f;
	ARG_UNPACK(f);
	if (f.isNull())
		return abstract_b(obj->getSystemState(),false);

	ASObject* params[3];
	ASObject *funcRet;

	auto it=th->data.begin();
	for(;it != th->data.end();++it)
	{
		if (it->second.type==DATA_OBJECT)
		{
			params[0] = it->second.data;
			it->second.data->incRef();
		}
		else
			params[0] =abstract_di(obj->getSystemState(),it->second.data_i);
		params[1] = abstract_i(obj->getSystemState(),it->first);
		params[2] = th;
		th->incRef();

		if(argslen==1)
		{
			funcRet=f->call(obj->getSystemState()->getNullRef(), params, 3);
		}
		else
		{
			args[1]->incRef();
			funcRet=f->call(args[1], params, 3);
		}
		if(funcRet)
		{
			if(Boolean_concrete(funcRet))
			{
				return funcRet;
			}
			funcRet->decRef();
		}
	}
	return abstract_b(obj->getSystemState(),false);
}

ASFUNCTIONBODY(Array, every)
{
	Array* th=static_cast<Array*>(obj);
	_NR<IFunction> f;
	ARG_UNPACK(f);
	if (f.isNull())
		return abstract_b(obj->getSystemState(),true);

	ASObject* params[3];
	ASObject *funcRet;

	auto it=th->data.begin();
	for(;it != th->data.end();++it)
	{
		if (it->second.type==DATA_OBJECT)
		{
			params[0] = it->second.data;
			it->second.data->incRef();
		}
		else
			params[0] =abstract_di(obj->getSystemState(),it->second.data_i);
		params[1] = abstract_i(obj->getSystemState(),it->first);
		params[2] = th;
		th->incRef();

		if(argslen==1)
		{
			funcRet=f->call(obj->getSystemState()->getNullRef(), params, 3);
		}
		else
		{
			args[1]->incRef();
			funcRet=f->call(args[1], params, 3);
		}
		if(funcRet)
		{
			if(!Boolean_concrete(funcRet))
			{
				return funcRet;
			}
			funcRet->decRef();
		}
	}
	return abstract_b(obj->getSystemState(),true);
}

ASFUNCTIONBODY(Array,_getLength)
{
	Array* th=static_cast<Array*>(obj);
	return abstract_ui(obj->getSystemState(),th->size());
}

ASFUNCTIONBODY(Array,_setLength)
{
	uint32_t newLen;
	ARG_UNPACK(newLen);
	Array* th=static_cast<Array*>(obj);
	if (th->getClass() && th->getClass()->isSealed)
		return NULL;
	//If newLen is equal to size do nothing
	if(newLen==th->size())
		return NULL;
	th->resize(newLen);
	return NULL;
}

ASFUNCTIONBODY(Array,forEach)
{
	Array* th=static_cast<Array*>(obj);
	_NR<IFunction> f;
	ARG_UNPACK(f);
	if (f.isNull())
		return NULL;
	ASObject* params[3];

	uint32_t s = th->size();
	for (uint32_t i=0; i < s; i++ )
	{
		auto it = th->data.find(i);
		if (it != th->data.end())
		{
			if(it->second.type==DATA_INT)
				params[0]=abstract_i(obj->getSystemState(),it->second.data_i);
			else if(it->second.type==DATA_OBJECT && it->second.data)
			{
				params[0]=it->second.data;
				params[0]->incRef();
			}
			else
				params[0]=obj->getSystemState()->getUndefinedRef();
		}
		else
			continue;
		params[1] = abstract_i(obj->getSystemState(),i);
		params[2] = th;
		th->incRef();

		ASObject *funcret;
		if( argslen == 1 )
		{
			funcret=f->call(obj->getSystemState()->getNullRef(), params, 3);
		}
		else
		{
			args[1]->incRef();
			funcret=f->call(args[1], params, 3);
		}
		if(funcret)
			funcret->decRef();
	}

	return NULL;
}

ASFUNCTIONBODY(Array, _reverse)
{
	Array* th = static_cast<Array*>(obj);

	std::map<uint32_t, data_slot> tmp = std::map<uint32_t, data_slot>(th->data.begin(),th->data.end());
	uint32_t size = th->size();
	th->data.clear();
	auto it=tmp.begin();
	for(;it != tmp.end();++it)
 	{
		th->data[size-(it->first+1)]=it->second;
	}
	th->incRef();
	return th;
}

ASFUNCTIONBODY(Array,lastIndexOf)
{
	Array* th=static_cast<Array*>(obj);
	number_t index;
	_NR<ASObject> arg0;
	ARG_UNPACK(arg0) (index, 0x7fffffff);
	int ret=-1;

	if(argslen == 1 && th->data.empty())
		return abstract_di(obj->getSystemState(),-1);

	size_t i = th->size()-1;

	if(std::isnan(index))
		return abstract_i(obj->getSystemState(),0);

	int j = index; //Preserve sign
	if(j < 0) //Negative offset, use it as offset from the end of the array
	{
		if((size_t)-j > th->size())
			return abstract_i(obj->getSystemState(),-1);
		else
			i = th->size()+j;
	}
	else //Positive offset, use it directly
	{
		if((size_t)j > th->size()) //If the passed offset is bigger than the array, cap the offset
			i = th->size()-1;
		else
			i = j;
	}
	do
	{
		auto it = th->data.find(i);
		if (it == th->data.end())
		    continue;
		DATA_TYPE dtype = it->second.type;
		assert_and_throw(dtype==DATA_OBJECT || dtype==DATA_INT);
		if((dtype == DATA_OBJECT && it->second.data->isEqualStrict(arg0.getPtr())) ||
			(dtype == DATA_INT && arg0->toInt() == it->second.data_i))
		{
			ret=i;
			break;
		}
	}
	while(i--);

	return abstract_i(obj->getSystemState(),ret);
}

ASFUNCTIONBODY(Array,shift)
{
	if (!obj->is<Array>())
	{
		// this seems to be how Flash handles the generic shift calls
		if (obj->is<Vector>())
			return Vector::shift(obj,args,argslen);
		if (obj->is<ByteArray>())
			return ByteArray::shift(obj,args,argslen);
		// for other objects we just decrease the length property
		multiname lengthName(NULL);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=obj->getSystemState()->getUniqueStringId("length");
		lengthName.ns.push_back(nsNameAndKind(obj->getSystemState(),"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(obj->getSystemState(),AS3,NAMESPACE));
		lengthName.isAttribute = true;
		_NR<ASObject> o=obj->getVariableByMultiname(lengthName,SKIP_IMPL);
		uint32_t res = o->toUInt();
		if (res > 0)
			obj->setVariableByMultiname(lengthName,abstract_ui(obj->getSystemState(),res-1),CONST_NOT_ALLOWED);
		return obj->getSystemState()->getUndefinedRef();
	}
	Array* th=static_cast<Array*>(obj);
	if(!th->size())
		return obj->getSystemState()->getUndefinedRef();
	ASObject* ret;
	auto it = th->data.find(0);
	if(it == th->data.end())
		ret = obj->getSystemState()->getUndefinedRef();
	else
	{
		if(it->second.type==DATA_OBJECT)
			ret=it->second.data;
		else
			ret = abstract_i(obj->getSystemState(),it->second.data_i);
	}
	std::map<uint32_t,data_slot> tmp;
	it=th->data.begin();
	for (; it != th->data.end(); ++it )
	{
		if(it->first)
		{
			tmp[it->first-1]=it->second;
		}
	}
	th->data.clear();
	th->data.insert(tmp.begin(),tmp.end());
	th->resize(th->size()-1);
	return ret;
}

int Array::capIndex(int i) const
{
	int totalSize=size();

	if(totalSize <= 0)
		return 0;
	else if(i < -totalSize)
		return 0;
	else if(i > totalSize)
		return totalSize;
	else if(i>=0)     // 0 <= i < totalSize
		return i;
	else              // -totalSize <= i < 0
	{
		//A negative i is relative to the end
		return i+totalSize;
	}
}

ASFUNCTIONBODY(Array,slice)
{
	Array* th=static_cast<Array*>(obj);
	int startIndex;
	int endIndex;

	ARG_UNPACK(startIndex, 0) (endIndex, 16777215);
	startIndex=th->capIndex(startIndex);
	endIndex=th->capIndex(endIndex);

	Array* ret=Class<Array>::getInstanceS(obj->getSystemState());
	int j = 0;
	for(int i=startIndex; i<endIndex; i++) 
	{
		auto it = th->data.find(i);
		if (it != th->data.end())
		{
			if(it->second.type == DATA_OBJECT)
				it->second.data->incRef();
			ret->data[j]=it->second;
		}
		j++;
	}
	ret->resize(j);
	return ret;
}

ASFUNCTIONBODY(Array,splice)
{
	Array* th=static_cast<Array*>(obj);
	int startIndex;
	int deleteCount;
	//By default, delete all the element up to the end
	//DeleteCount defaults to the array len, it will be capped below
	ARG_UNPACK_MORE_ALLOWED(startIndex) (deleteCount, th->size());

	int totalSize=th->size();
	Array* ret=Class<Array>::getInstanceS(obj->getSystemState());

	startIndex=th->capIndex(startIndex);

	if((startIndex+deleteCount)>totalSize)
		deleteCount=totalSize-startIndex;

	ret->resize(deleteCount);
	if(deleteCount)
	{
		// write deleted items to return array
		for(int i=0;i<deleteCount;i++)
		{
			auto it = th->data.find(startIndex+i);
			if (it != th->data.end())
				ret->data[i] = it->second;
		}
		// delete items from current array
		for (int i = 0; i < deleteCount; i++)
		{
			auto it = th->data.find(startIndex+i);
			if (it != th->data.end())
			{
				th->data.erase(it);
			}
		}
	}
	// remember items in current array that have to be moved to new position
	vector<data_slot> tmp = vector<data_slot>(totalSize- (startIndex+deleteCount));
	for (int i = startIndex+deleteCount; i < totalSize ; i++)
	{
		auto it = th->data.find(i);
		if (it != th->data.end())
		{
			tmp[i-(startIndex+deleteCount)] = it->second;
			th->data.erase(it);
		}
	}
	th->resize(startIndex);

	
	//Insert requested values starting at startIndex
	for(unsigned int i=2;i<argslen;i++)
	{
		args[i]->incRef();
		th->push(_MR(args[i]));
	}
	// move remembered items to new position
	for(int i=0;i<totalSize- (startIndex+deleteCount);i++)
	{
		if (tmp[i].type != DATA_OBJECT || tmp[i].data != NULL)
			th->data[startIndex+i+(argslen > 2 ? argslen-2 : 0)] = tmp[i];
	}
	th->resize((totalSize-deleteCount)+(argslen > 2 ? argslen-2 : 0));
	return ret;
}

ASFUNCTIONBODY(Array,join)
{
	Array* th=static_cast<Array*>(obj);
	string ret;
	tiny_string del;
	ARG_UNPACK(del, ",");

	for(uint32_t i=0;i<th->size();i++)
	{
		_R<ASObject> o = th->at(i);
		if (!o->is<Undefined>() && !o->is<Null>())
			ret+= o->toString().raw_buf();
		if(i!=th->size()-1)
			ret+=del.raw_buf();
	}
	return abstract_s(obj->getSystemState(),ret);
}

ASFUNCTIONBODY(Array,indexOf)
{
	Array* th=static_cast<Array*>(obj);
	int ret=-1;
	int32_t index;
	_NR<ASObject> arg0;
	ARG_UNPACK(arg0) (index, 0);
	if (index < 0) index = th->size()+ index;
	if (index < 0) index = 0;

	DATA_TYPE dtype;
	for (auto it=th->data.begin() ; it != th->data.end(); ++it )
	{
		if (it->first < (uint32_t)index)
			continue;
		data_slot sl = it->second;
		dtype = sl.type;
		assert_and_throw(dtype==DATA_OBJECT || dtype==DATA_INT);
		if((dtype == DATA_OBJECT && sl.data->isEqualStrict(arg0.getPtr())) ||
			(dtype == DATA_INT && abstract_di(obj->getSystemState(),sl.data_i)->isEqualStrict(arg0.getPtr())))
		{
			ret=it->first;
			break;
		}
	}
	return abstract_i(obj->getSystemState(),ret);
}


ASFUNCTIONBODY(Array,_pop)
{
	if (!obj->is<Array>())
	{
		// this seems to be how Flash handles the generic pop calls
		if (obj->is<Vector>())
			return Vector::_pop(obj,args,argslen);
		if (obj->is<ByteArray>())
			return ByteArray::pop(obj,args,argslen);
		// for other objects we just decrease the length property
		multiname lengthName(NULL);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=obj->getSystemState()->getUniqueStringId("length");
		lengthName.ns.push_back(nsNameAndKind(obj->getSystemState(),"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(obj->getSystemState(),AS3,NAMESPACE));
		lengthName.isAttribute = true;
		_NR<ASObject> o=obj->getVariableByMultiname(lengthName,SKIP_IMPL);
		uint32_t res = o->toUInt();
		if (res > 0)
			obj->setVariableByMultiname(lengthName,abstract_ui(obj->getSystemState(),res-1),CONST_NOT_ALLOWED);
		return obj->getSystemState()->getUndefinedRef();
	}
	Array* th=static_cast<Array*>(obj);
	uint32_t size =th->size();
	if (size == 0)
		return obj->getSystemState()->getUndefinedRef();
	ASObject* ret;
	
	auto it = th->data.find(size-1);
	if (it != th->data.end())
	{
		if(it->second.type==DATA_OBJECT)
			ret=it->second.data;
		else
			ret = abstract_i(obj->getSystemState(),it->second.data_i);
		th->data.erase(it);
	}
	else
		ret = obj->getSystemState()->getUndefinedRef();

	th->currentsize--;
	return ret;
}

bool Array::sortComparatorDefault::operator()(const data_slot& d1, const data_slot& d2)
{
	if(isNumeric)
	{
		number_t a=numeric_limits<double>::quiet_NaN();
		number_t b=numeric_limits<double>::quiet_NaN();
		if(d1.type==DATA_INT)
			a=d1.data_i;
		else if(d1.type==DATA_OBJECT && d1.data)
			a=d1.data->toNumber();

		if(d2.type==DATA_INT)
			b=d2.data_i;
		else if(d2.type==DATA_OBJECT && d2.data)
			b=d2.data->toNumber();

		if(std::isnan(a) || std::isnan(b))
			throw RunTimeException("Cannot sort non number with Array.NUMERIC option");
		if(isDescending)
			return b>a;
		else
			return a<b;
	}
	else
	{
		//Comparison is always in lexicographic order
		tiny_string s1;
		tiny_string s2;
		if(d1.type==DATA_INT)
			s1=Integer::toString(d1.data_i);
		else if(d1.type==DATA_OBJECT && d1.data)
			s1=d1.data->toString();
		else
			s1="undefined";
		if(d2.type==DATA_INT)
			s2=Integer::toString(d2.data_i);
		else if(d2.type==DATA_OBJECT && d2.data)
			s2=d2.data->toString();
		else
			s2="undefined";

		if(isDescending)
		{
			//TODO: unicode support
			if(isCaseInsensitive)
				return s1.strcasecmp(s2)>0;
			else
				return s1>s2;
		}
		else
		{
			//TODO: unicode support
			if(isCaseInsensitive)
				return s1.strcasecmp(s2)<0;
			else
				return s1<s2;
		}
	}
}

bool Array::sortComparatorWrapper::operator()(const data_slot& d1, const data_slot& d2)
{
	ASObject* objs[2];
	if(d1.type==DATA_INT)
		objs[0]=abstract_i(comparator->getSystemState(),d1.data_i);
	else if(d1.type==DATA_OBJECT && d1.data)
	{
		objs[0]=d1.data;
		objs[0]->incRef();
	}
	else
		objs[0]=comparator->getSystemState()->getUndefinedRef();

	if(d2.type==DATA_INT)
		objs[1]=abstract_i(comparator->getSystemState(),d2.data_i);
	else if(d2.type==DATA_OBJECT && d2.data)
	{
		objs[1]=d2.data;
		objs[1]->incRef();
	}
	else
		objs[1]=comparator->getSystemState()->getUndefinedRef();

	assert(comparator);
	_NR<ASObject> ret=_MNR(comparator->call(comparator->getSystemState()->getNullRef(), objs, 2));
	assert_and_throw(ret);
	return (ret->toNumber()<0); //Less
}

ASFUNCTIONBODY(Array,_sort)
{
	Array* th=static_cast<Array*>(obj);
	IFunction* comp=NULL;
	bool isNumeric=false;
	bool isCaseInsensitive=false;
	bool isDescending=false;
	for(uint32_t i=0;i<argslen;i++)
	{
		if(args[i]->getObjectType()==T_FUNCTION) //Comparison func
		{
			assert_and_throw(comp==NULL);
			comp=static_cast<IFunction*>(args[i]);
		}
		else
		{
			uint32_t options=args[i]->toInt();
			if(options&NUMERIC)
				isNumeric=true;
			if(options&CASEINSENSITIVE)
				isCaseInsensitive=true;
			if(options&DESCENDING)
				isDescending=true;
			if(options&(~(NUMERIC|CASEINSENSITIVE|DESCENDING)))
				throw UnsupportedException("Array::sort not completely implemented");
		}
	}
	std::vector<data_slot> tmp = vector<data_slot>(th->data.size());
	auto it=th->data.begin();
	int i = 0;
	for(;it != th->data.end();++it)
	{
		tmp[i++]= it->second;
	}
	
	if(comp)
		sort(tmp.begin(),tmp.end(),sortComparatorWrapper(comp));
	else
		sort(tmp.begin(),tmp.end(),sortComparatorDefault(isNumeric,isCaseInsensitive,isDescending));

	th->data.clear();
	std::vector<data_slot>::iterator ittmp=tmp.begin();
	i = 0;
	for(;ittmp != tmp.end();++ittmp)
	{
		th->data[i++]= *ittmp;
	}
	obj->incRef();
	return obj;
}

bool Array::sortOnComparator::operator()(const data_slot& d1, const data_slot& d2)
{
	std::vector<sorton_field>::iterator it=fields.begin();
	for(;it != fields.end();++it)
	{
		assert_and_throw(d1.type == DATA_OBJECT && d1.type == DATA_OBJECT);

		_NR<ASObject> obj1 = d1.data->getVariableByMultiname(it->fieldname);
		_NR<ASObject> obj2 = d2.data->getVariableByMultiname(it->fieldname);
		if(it->isNumeric)
		{
			number_t a=numeric_limits<double>::quiet_NaN();
			number_t b=numeric_limits<double>::quiet_NaN();
			a=obj1->toNumber();
			
			b=obj2->toNumber();
			
			if(std::isnan(a) || std::isnan(b))
				throw RunTimeException("Cannot sort non number with Array.NUMERIC option");
			if (b != a)
			{
				if(it->isDescending)
					return b>a;
				else
					return a<b;
			}
		}
		else
		{
			//Comparison is always in lexicographic order
			tiny_string s1;
			tiny_string s2;
			s1=obj1->toString();
			s2=obj2->toString();
			if (s1 != s2)
			{
				if(it->isDescending)
				{
					if(it->isCaseInsensitive)
						return s1.strcasecmp(s2)>0;
					else
						return s1>s2;
				}
				else
				{
					if(it->isCaseInsensitive)
						return s1.strcasecmp(s2)<0;
					else
						return s1<s2;
				}
			}
		}
	}
	return false;
}

ASFUNCTIONBODY(Array,sortOn)
{
	if (argslen != 1 && argslen != 2)
		throwError<ArgumentError>(kWrongArgumentCountError, "1",
					  Integer::toString(argslen));
	Array* th=static_cast<Array*>(obj);
	std::vector<sorton_field> sortfields;
	if(args[0]->is<Array>())
	{
		Array* obj=static_cast<Array*>(args[0]);
		int n = 0;
		auto it=obj->data.begin();
		for(;it != obj->data.end();++it)
		{
			multiname sortfieldname(NULL);
			sortfieldname.ns.push_back(nsNameAndKind(obj->getSystemState(),"",NAMESPACE));
			if (it->second.type == DATA_OBJECT)
			{
				sortfieldname.setName(it->second.data);
			}
			sorton_field sf(sortfieldname);
			sortfields.push_back(sf);
		}
		if (argslen == 2 && args[1]->is<Array>())
		{
			Array* opts=static_cast<Array*>(args[1]);
			auto itopt=opts->data.begin();
			int nopt = 0;
			for(;itopt != opts->data.end() && nopt < n;++itopt)
			{
				uint32_t options=0;
				if (itopt->second.type == DATA_OBJECT)
					options = itopt->second.data->toInt();
				else
					options = itopt->second.data_i;
				if(options&NUMERIC)
					sortfields[nopt].isNumeric=true;
				if(options&CASEINSENSITIVE)
					sortfields[nopt].isCaseInsensitive=true;
				if(options&DESCENDING)
					sortfields[nopt].isDescending=true;
				if(options&(~(NUMERIC|CASEINSENSITIVE|DESCENDING)))
					throw UnsupportedException("Array::sort not completely implemented");
				nopt++;
			}
		}
	}
	else
	{
		multiname sortfieldname(NULL);
		sortfieldname.setName(args[0]);
		sortfieldname.ns.push_back(nsNameAndKind(obj->getSystemState(),"",NAMESPACE));
		sorton_field sf(sortfieldname);
		if (argslen == 2)
		{
			uint32_t options = args[1]->toInt();
			if(options&NUMERIC)
				sf.isNumeric=true;
			if(options&CASEINSENSITIVE)
				sf.isCaseInsensitive=true;
			if(options&DESCENDING)
				sf.isDescending=true;
			if(options&(~(NUMERIC|CASEINSENSITIVE|DESCENDING)))
				throw UnsupportedException("Array::sort not completely implemented");
		}
		sortfields.push_back(sf);
	}
	
	std::vector<data_slot> tmp = vector<data_slot>(th->data.size());
	auto it=th->data.begin();
	int i = 0;
	for(;it != th->data.end();++it)
	{
		tmp[i++]= it->second;
	}
	
	sort(tmp.begin(),tmp.end(),sortOnComparator(sortfields));

	th->data.clear();
	std::vector<data_slot>::iterator ittmp=tmp.begin();
	i = 0;
	for(;ittmp != tmp.end();++ittmp)
	{
		th->data[i++]= *ittmp;
	}
	obj->incRef();
	return obj;
}

ASFUNCTIONBODY(Array,unshift)
{
	if (!obj->is<Array>())
	{
		// this seems to be how Flash handles the generic unshift calls
		if (obj->is<Vector>())
			return Vector::unshift(obj,args,argslen);
		if (obj->is<ByteArray>())
			return ByteArray::unshift(obj,args,argslen);
		// for other objects we just increase the length property
		multiname lengthName(NULL);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=obj->getSystemState()->getUniqueStringId("length");
		lengthName.ns.push_back(nsNameAndKind(obj->getSystemState(),"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(obj->getSystemState(),AS3,NAMESPACE));
		lengthName.isAttribute = true;
		_NR<ASObject> o=obj->getVariableByMultiname(lengthName,SKIP_IMPL);
		uint32_t res = o->toUInt();
		obj->setVariableByMultiname(lengthName,abstract_ui(obj->getSystemState(),res+argslen),CONST_NOT_ALLOWED);
		return obj->getSystemState()->getUndefinedRef();
	}
	Array* th=static_cast<Array*>(obj);
	// Derived classes may be sealed!
	if (th->getClass() && th->getClass()->isSealed)
		throwError<ReferenceError>(kWriteSealedError,"unshift",th->getClass()->getQualifiedClassName());
	if (argslen > 0)
	{
		th->resize(th->size()+argslen);
		std::map<uint32_t,data_slot> tmp;
		for (auto it=th->data.rbegin(); it != th->data.rend(); ++it )
		{
			tmp[it->first+argslen]=it->second;
		}
		
		for(uint32_t i=0;i<argslen;i++)
		{
			tmp[i] = data_slot(args[i]);
			args[i]->incRef();
		}
		th->data.clear();
		th->data.insert(tmp.begin(),tmp.end());
	}
	return abstract_i(obj->getSystemState(),th->size());
}

ASFUNCTIONBODY(Array,_push)
{
	if (!obj->is<Array>())
	{
		// this seems to be how Flash handles the generic push calls
		if (obj->is<Vector>())
			return Vector::push(obj,args,argslen);
		if (obj->is<ByteArray>())
			return ByteArray::push(obj,args,argslen);
		// for other objects we just increase the length property
		multiname lengthName(NULL);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=obj->getSystemState()->getUniqueStringId("length");
		lengthName.ns.push_back(nsNameAndKind(obj->getSystemState(),"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(obj->getSystemState(),AS3,NAMESPACE));
		lengthName.isAttribute = true;
		_NR<ASObject> o=obj->getVariableByMultiname(lengthName,SKIP_IMPL);
		uint32_t res = o->toUInt();
		obj->setVariableByMultiname(lengthName,abstract_ui(obj->getSystemState(),res+argslen),CONST_NOT_ALLOWED);
		return obj->getSystemState()->getUndefinedRef();
	}
	Array* th=static_cast<Array*>(obj);
	for(unsigned int i=0;i<argslen;i++)
	{
		args[i]->incRef();
		th->push(_MR(args[i]));
	}
	return abstract_i(obj->getSystemState(),th->size());
}
// AS3 handles push on uint.MAX_VALUE differently than ECMA, so we need to push methods
ASFUNCTIONBODY(Array,_push_as3)
{
	if (!obj->is<Array>())
	{
		// this seems to be how Flash handles the generic push calls
		if (obj->is<Vector>())
			return Vector::push(obj,args,argslen);
		if (obj->is<ByteArray>())
			return ByteArray::push(obj,args,argslen);
		// for other objects we just increase the length property
		multiname lengthName(NULL);
		lengthName.name_type=multiname::NAME_STRING;
		lengthName.name_s_id=obj->getSystemState()->getUniqueStringId("length");
		lengthName.ns.push_back(nsNameAndKind(obj->getSystemState(),"",NAMESPACE));
		lengthName.ns.push_back(nsNameAndKind(obj->getSystemState(),AS3,NAMESPACE));
		lengthName.isAttribute = true;
		_NR<ASObject> o=obj->getVariableByMultiname(lengthName,SKIP_IMPL);
		uint32_t res = o->toUInt();
		obj->setVariableByMultiname(lengthName,abstract_ui(obj->getSystemState(),res+argslen),CONST_NOT_ALLOWED);
		return obj->getSystemState()->getUndefinedRef();
	}
	Array* th=static_cast<Array*>(obj);
	for(unsigned int i=0;i<argslen;i++)
	{
		if (th->size() >= UINT32_MAX)
			break;
		args[i]->incRef();
		th->push(_MR(args[i]));
	}
	return abstract_i(obj->getSystemState(),th->size());
}

ASFUNCTIONBODY(Array,_map)
{
	Array* th=static_cast<Array*>(obj);

	if(argslen < 1)
		throwError<ArgumentError>(kWrongArgumentCountError, "Array.map", "1", Integer::toString(argslen));
	IFunction* func= NULL;
	if (!args[0]->is<RegExp>())
	{
		assert_and_throw(args[0]->getObjectType()==T_FUNCTION);
		func=static_cast<IFunction*>(args[0]);
	}
	Array* arrayRet=Class<Array>::getInstanceS(obj->getSystemState());

	uint32_t s = th->size();
	for (uint32_t i=0; i < s; i++ )
	{
		ASObject* funcArgs[3];
		auto it = th->data.find(i);
		if (it != th->data.end())
		{
			if(it->second.type==DATA_INT)
				funcArgs[0]=abstract_i(obj->getSystemState(),it->second.data_i);
			else if(it->second.type==DATA_OBJECT && it->second.data)
			{
				funcArgs[0]=it->second.data;
				funcArgs[0]->incRef();
			}
			else
				funcArgs[0]=obj->getSystemState()->getUndefinedRef();
		}
		else
			funcArgs[0]=obj->getSystemState()->getUndefinedRef();
		funcArgs[1]=abstract_i(obj->getSystemState(),i);
		funcArgs[2]=th;
		funcArgs[2]->incRef();
		ASObject* funcRet= NULL;
		if (func)
		{
			if (argslen > 1)
				args[1]->incRef();
			funcRet = func->call(argslen > 1? args[1] : obj->getSystemState()->getNullRef(), funcArgs, 3);
		}
		else
			funcRet = RegExp::exec(args[0],funcArgs,1);
		assert_and_throw(funcRet);
		arrayRet->push(_MR(funcRet));
	}

	return arrayRet;
}

ASFUNCTIONBODY(Array,_toString)
{
	if(Class<Number>::getClass(obj->getSystemState())->prototype->getObj() == obj)
		return abstract_s(obj->getSystemState(),"");
	if(!obj->is<Array>())
	{
		LOG(LOG_NOT_IMPLEMENTED, "generic Array::toString");
		return abstract_s(obj->getSystemState(),"");
	}
	
	Array* th=obj->as<Array>();
	return abstract_s(obj->getSystemState(),th->toString_priv());
}

ASFUNCTIONBODY(Array,_toLocaleString)
{
	if(Class<Number>::getClass(obj->getSystemState())->prototype->getObj() == obj)
		return abstract_s(obj->getSystemState(),"");
	if(!obj->is<Array>())
	{
		LOG(LOG_NOT_IMPLEMENTED, "generic Array::toLocaleString");
		return abstract_s(obj->getSystemState(),"");
	}
	
	Array* th=obj->as<Array>();
	return abstract_s(obj->getSystemState(),th->toString_priv(true));
}

int32_t Array::getVariableByMultiname_i(const multiname& name)
{
	assert_and_throw(implEnable);
	uint32_t index=0;
	if(!isValidMultiname(getSystemState(),name,index))
		return ASObject::getVariableByMultiname_i(name);

	if(index<size())
	{
		auto it = data.find(index);
		if (it == data.end())
			return 0;
		const data_slot& sl = it->second;
		switch(sl.type)
		{
			case DATA_OBJECT:
			{
				assert(sl.data!=NULL);
				if(sl.data->getObjectType()==T_INTEGER)
				{
					Integer* i=static_cast<Integer*>(sl.data);
					return i->toInt();
				}
				else if(sl.data->getObjectType()==T_NUMBER)
				{
					Number* i=static_cast<Number*>(sl.data);
					return i->toInt();
				}
				else
					throw UnsupportedException("Array::getVariableByMultiname_i not completely implemented");
			}
			case DATA_INT:
				return sl.data_i;
		}
	}

	return ASObject::getVariableByMultiname_i(name);
}

_NR<ASObject> Array::getVariableByMultiname(const multiname& name, GET_VARIABLE_OPTION opt)
{
	if((opt & SKIP_IMPL)!=0 || !implEnable)
		return ASObject::getVariableByMultiname(name,opt);

	assert_and_throw(name.ns.size()>0);
	if(!name.ns[0].hasEmptyName())
		return ASObject::getVariableByMultiname(name,opt);

	uint32_t index=0;
	if(!isValidMultiname(getSystemState(),name,index))
		return ASObject::getVariableByMultiname(name,opt);
	auto it = data.find(index);
	if(it != data.end())
	{
		ASObject* ret=NULL;
		const data_slot& sl = it->second;
		switch(sl.type)
		{
			case DATA_OBJECT:
				ret=sl.data;
				if(ret==NULL)
					ret=getSystemState()->getUndefinedRef();
				ret->incRef();
				break;
			case DATA_INT:
				ret=abstract_di(this->getSystemState(),sl.data_i);
				break;
		}
		return _MNR(ret);
	}
	_NR<ASObject> ret;
	//Check prototype chain
	Prototype* proto = this->getClass()->prototype.getPtr();
	while(proto)
	{
		ret = proto->getObj()->getVariableByMultiname(name, opt);
		if(!ret.isNull())
			return ret;
		proto = proto->prevPrototype.getPtr();
	}
	if(index<size())
		return _MNR(getSystemState()->getUndefinedRef());
	
	return NullRef;
}

void Array::setVariableByMultiname_i(const multiname& name, int32_t value)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!isValidMultiname(getSystemState(),name,index))
	{
		ASObject::setVariableByMultiname_i(name,value);
		return;
	}
	if (index==0xFFFFFFFF)
		return;
	if(index>=size())
		resize(index+1);
	auto it = data.find(index);
	if(it != data.end())
		it->second.clear();
	data_slot ds;
	ds.data_i=value;
	ds.type=DATA_INT;
	data[index] = ds;
}


bool Array::hasPropertyByMultiname(const multiname& name, bool considerDynamic, bool considerPrototype)
{
	if(considerDynamic==false)
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);
	if (!isConstructed())
		return false;

	uint32_t index=0;
	if(!isValidMultiname(getSystemState(),name,index))
		return ASObject::hasPropertyByMultiname(name, considerDynamic, considerPrototype);

	return (data.find(index) != data.end());
}

bool Array::isValidMultiname(SystemState* sys, const multiname& name, uint32_t& index)
{
	if (name.name_type == multiname::NAME_INT && name.name_i >= 0)
	{
		index = name.name_i;
		return true;
	}
	if(name.isEmpty())
		return false;
	//First of all the multiname has to contain the null namespace
	//As the namespace vector is sorted, we check only the first one
	assert_and_throw(name.ns.size()!=0);
	if(!name.ns[0].hasEmptyName())
		return false;
	if (name.name_type == multiname::NAME_STRING && 
	    !isIntegerWithoutLeadingZeros(name.normalizedName(sys)))
		return false;

	return name.toUInt(sys,index);
}

bool Array::isIntegerWithoutLeadingZeros(const tiny_string& value)
{
	if (value.empty())
		return false;
	else if (value == "0")
		return true;

	bool first = true;
	for (CharIterator it=value.begin(); it!=value.end(); ++it)
	{
		if (!it.isdigit() || (first && *it == '0'))
			return false;

		first = false;
	}
	
	return true;
}

void Array::setVariableByMultiname(const multiname& name, ASObject* o, CONST_ALLOWED_FLAG allowConst)
{
	assert_and_throw(implEnable);
	uint32_t index=0;
	if(!isValidMultiname(getSystemState(),name,index))
	{
		ASObject::setVariableByMultiname(name,o,allowConst);
//		setIsEnumerable(name,false);
		return;
	}
	// Derived classes may be sealed!
	if (getClass() && getClass()->isSealed)
		throwError<ReferenceError>(kWriteSealedError,
					   name.normalizedNameUnresolved(getSystemState()),
					   getClass()->getQualifiedClassName());
	if (index==0xFFFFFFFF)
		return;
	if(index>=size())
		resize((uint64_t)index+1);

	data_slot ds;
	auto it = data.find(index);
	if(it != data.end())
		it->second.clear();

	if(o->getObjectType()==T_INTEGER)
	{
		Integer* i=static_cast<Integer*>(o);
		ds.data_i=i->val;
		ds.type=DATA_INT;
		o->decRef();
	}
	else
	{
		ds.data=o;
		ds.type=DATA_OBJECT;
	}
	data[index] = ds;
}

bool Array::deleteVariableByMultiname(const multiname& name)
{
	assert_and_throw(implEnable);
	unsigned int index=0;
	if(!isValidMultiname(getSystemState(),name,index))
		return ASObject::deleteVariableByMultiname(name);
	if (getClass() && getClass()->isSealed)
		return false;

	if(index>=size())
		return true;
	auto it = data.find(index);
	if(it == data.end())
		return true;
	it->second.clear();
	data.erase(it);
	return true;
}

bool Array::isValidQName(const tiny_string& name, const tiny_string& ns, unsigned int& index)
{
	if(ns!="")
		return false;
	assert_and_throw(!name.empty());
	index=0;
	//First we try to convert the string name to an index, at the first non-digit
	//we bail out
	for(auto i=name.begin(); i!=name.end(); ++i)
	{
		if(!i.isdigit())
			return false;

		index*=10;
		index+=i.digit_value();
	}
	return true;
}

tiny_string Array::toString()
{
	assert_and_throw(implEnable);
	return toString_priv();
}

tiny_string Array::toString_priv(bool localized) const
{
	string ret;
	for(uint32_t i=0;i<size();i++)
	{
		auto it = data.find(i);
		if(it != data.end())
		{
			const data_slot& sl = it->second;
			if(sl.type==DATA_OBJECT)
			{
				if(sl.data && !sl.data->is<Undefined>() && !sl.data->is<Null>())
				{
					if (localized)
						ret += sl.data->toLocaleString().raw_buf();
					else
						ret += sl.data->toString().raw_buf();
				}
			}
			else if(sl.type==DATA_INT)
			{
				char buf[20];
				snprintf(buf,20,"%i",sl.data_i);
				ret+=buf;
			}
			else
				throw UnsupportedException("Array::toString not completely implemented");
		}
		if(i!=size()-1)
			ret+=',';
	}
	return ret;
}

_R<ASObject> Array::nextValue(uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=size())
	{
		index--;
		auto it = data.find(index);
		if(it == data.end())
			return _MR(getSystemState()->getUndefinedRef());
		const data_slot& sl = it->second;
		if(sl.type==DATA_OBJECT)
		{
			if(sl.data==NULL)
				return _MR(getSystemState()->getUndefinedRef());
			else
			{
				sl.data->incRef();
				return _MR(sl.data);
			}
		}
		else if(sl.type==DATA_INT)
			return _MR(abstract_di(getSystemState(),sl.data_i));
		else
			throw UnsupportedException("Unexpected data type");
	}
	else
	{
		//Fall back on object properties
		return ASObject::nextValue(index-size());
	}
}

uint32_t Array::nextNameIndex(uint32_t cur_index)
{
	assert_and_throw(implEnable);
	if(cur_index<size())
	{
		while (!data.count(cur_index) && cur_index<size())
		{
			cur_index++;
		}
		if(cur_index<size())
			return cur_index+1;
	}
	//Fall back on object properties
	uint32_t ret=ASObject::nextNameIndex(cur_index-size());
	if(ret==0)
		return 0;
	else
		return ret+size();
	
}

_R<ASObject> Array::nextName(uint32_t index)
{
	assert_and_throw(implEnable);
	if(index<=size())
		return _MR(abstract_i(getSystemState(),index-1));
	else
	{
		//Fall back on object properties
		return ASObject::nextName(index-size());
	}
}

_R<ASObject> Array::at(unsigned int index) const
{
	if(size()<=index)
		outofbounds(index);

	auto it = data.find(index);
	if(it == data.end())
		return _MR(getSystemState()->getUndefinedRef());
	const data_slot& sl = it->second;
	switch(sl.type)
	{
		case DATA_OBJECT:
		{
			if(sl.data)
			{
				sl.data->incRef();
				return _MR(sl.data);
			}
		}
		case DATA_INT:
			return _MR(abstract_di(getSystemState(),sl.data_i));
	}

	//We should be here only if data is an object and is NULL
	return _MR(getSystemState()->getUndefinedRef());
}

void Array::outofbounds(unsigned int index) const
{
	throwError<RangeError>(kInvalidArrayLengthError, Number::toString(index));
}

void Array::resize(uint64_t n)
{
	// Bug-for-bug compatible wrapping. See Tamarin test
	// as3/Array/length_mods.swf and Tamarin bug #685323.
	if (n > 0xFFFFFFFF)
		n = (n % 0x100000000);

	std::map<uint32_t,data_slot>::reverse_iterator it;
	std::map<uint32_t,data_slot>::iterator itstart = n ? data.end() : data.begin();
	for ( it=data.rbegin() ; it != data.rend(); ++it )
	{
		if (it->first < n)
		{
			itstart = it.base();
			break;
		}
		it->second.clear();
	}
	if (itstart != data.end())
		data.erase(itstart,data.end());
	currentsize = n;
}

void Array::serialize(ByteArray* out, std::map<tiny_string, uint32_t>& stringMap,
				std::map<const ASObject*, uint32_t>& objMap,
				std::map<const Class_base*, uint32_t>& traitsMap)
{
	if (out->getObjectEncoding() == ObjectEncoding::AMF0)
	{
		LOG(LOG_NOT_IMPLEMENTED,"serializing Array in AMF0 not implemented");
		return;
	}
	assert_and_throw(objMap.find(this)==objMap.end());
	out->writeByte(array_marker);
	//Check if the array has been already serialized
	auto it=objMap.find(this);
	if(it!=objMap.end())
	{
		//The least significant bit is 0 to signal a reference
		out->writeU29(it->second << 1);
	}
	else
	{
		//Add the array to the map
		objMap.insert(make_pair(this, objMap.size()));

		uint32_t denseCount = size();
		assert_and_throw(denseCount<0x20000000);
		uint32_t value = (denseCount << 1) | 1;
		out->writeU29(value);
		serializeDynamicProperties(out, stringMap, objMap, traitsMap);
		for(uint32_t i=0;i<denseCount;i++)
		{
			if (data.find(i) == data.end())
			{
				out->writeByte(null_marker);
			}
			else
			{
				switch(data.at(i).type)
				{
					case DATA_INT:
						out->writeByte(double_marker);
						out->serializeDouble(data.at(i).data_i);
						break;
					case DATA_OBJECT:
						data.at(i).data->serialize(out, stringMap, objMap, traitsMap);
						break;
				}
			}
		}
	}
}

tiny_string Array::toJSON(std::vector<ASObject *> &path, IFunction *replacer, const tiny_string& spaces,const tiny_string& filter)
{
	bool ok;
	tiny_string res = call_toJSON(ok,path,replacer,spaces,filter);
	if (ok)
		return res;
	// check for cylic reference
	if (std::find(path.begin(),path.end(), this) != path.end())
		throwError<TypeError>(kJSONCyclicStructure);
	
	path.push_back(this);
	res += "[";
	bool bfirst = true;
	tiny_string newline = (spaces.empty() ? "" : "\n");
	for (auto it=data.begin() ; it != data.end(); ++it)
	{
		tiny_string subres;
		ASObject* o = it->second.type==DATA_OBJECT ? it->second.data : abstract_i(getSystemState(),it->second.data_i);
		if (replacer != NULL)
		{
			ASObject* params[2];
			
			params[0] = abstract_di(getSystemState(),it->first);
			params[0]->incRef();
			params[1] = o;
			params[1]->incRef();
			ASObject *funcret=replacer->call(getSystemState()->getNullRef(), params, 2);
			if (funcret)
				subres = funcret->toJSON(path,NULL,spaces,filter);
		}
		else
		{
			if(it->second.type==DATA_OBJECT)
			{
				if (it->second.data)
					subres = it->second.data->toJSON(path,replacer,spaces,filter);
				else
					continue;
			}
			else
				subres = o->toString();
		}
		if (!subres.empty())
		{
			if (!bfirst)
				res += ",";
			res += newline+spaces;
			
			bfirst = false;
			res += subres;
		}
	}
	if (!bfirst)
		res += newline+spaces.substr_bytes(0,spaces.numBytes()/2);
	res += "]";
	path.pop_back();
	return res;
	
}

Array::~Array()
{
}

void Array::set(unsigned int index, _R<ASObject> o)
{
	if(index<currentsize)
	{
		data_slot ds;
		if(data.find(index) != data.end())
			data[index].clear();
		if(o->getObjectType()==T_INTEGER)
		{
			Integer* i=o->as<Integer>();
			ds.data_i=i->val;
			ds.type=DATA_INT;
		}
		else
		{
			o->incRef();
			ds.data=o.getPtr();
			ds.type=DATA_OBJECT;
		}
		data[index]=ds;
	}
	else
		outofbounds(index);
}

void Array::push(Ref<ASObject> o)
{
	// Derived classes may be sealed!
	if (getClass() && getClass()->isSealed)
		throwError<ReferenceError>(kWriteSealedError,"push",getClass()->getQualifiedClassName());
	currentsize++;
	set(currentsize-1,o);
}

