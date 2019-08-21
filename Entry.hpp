//
// Item
// K
// V
// Serialize
// Cache
//
#include <cstdio>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include "cacheEntry.hpp"



typedef struct {
	void* data;
	std::size_t length;
} data_ptr;

template <class T>
bool cprArrToVal(const T* arr, T val, int len){
	for(int i = 0 ; i < len ; i++){
		if(arr[i] != val){
			return false;
		}
	}
	return true;
}

template <class KeyType,
			class ValueType,
			data_ptr (srlze) (KeyType, ValueType),
			void (deserialize) (const void*, uint, uint, KeyType&, ValueType&),
			size_t (keyLen) (KeyType),
			size_t (valLen) (ValueType)>
class Entry{
protected:
	KeyType key;
	ValueType value;
public:
	KeyType getKey(){
		return key;
	}
	ValueType getVal(){
		return value;
	}
	explicit Entry(KeyType k, ValueType v){
		key = k;
		value = v;
	}
	explicit Entry(const void* data, uint klen, uint vlen){
		KeyType k;
		ValueType v;
		deserialize(data, klen, vlen, k, v);
		key = k;
		value = v;
	}
	const char* Serialize(){
		return (char*)(srlze(key, value).data);
	}
	virtual std::size_t size(){
		return srlze(key,value).length;
	}
	void cache(void* data, void* CEloc){
		data_ptr srl = srlze(key, value);
		if(!cprArrToVal(reinterpret_cast<char*>(data), '\0', srl.length/sizeof(char))){
			throw std::runtime_error("Data not null");
		}else if(*(reinterpret_cast<cacheEntry*>(CEloc)) != (cacheEntry){0}){
			std::cout << "Entry: "<<*(reinterpret_cast<cacheEntry*>(CEloc))<<std::endl;
			throw std::runtime_error("Cache not null");
		}else{
			std::memcpy(data, srl.data, srl.length/sizeof(char));
			cacheEntry* ce = new cacheEntry;
			*ce = (cacheEntry){0};
			ce->magic = CKMAGIC;
			ce->offset = reinterpret_cast<int64_t>(data) - reinterpret_cast<int64_t>(CEloc);
			ce->keyLen = keyLen(key);
			ce->valLen = valLen(value);
			ce->hitct = 1;
			std::memcpy(CEloc, ce, sizeof(cacheEntry));
			delete ce;
		}
	}
	static Entry<KeyType, ValueType, srlze, deserialize, keyLen, valLen> fromCache(cacheEntry* ce){
		if(ce->magic != CKMAGIC){
			throw std::runtime_error("Cache Entry Not Valid... Unable to Recover");
		}else{
			int offset = ce->offset;
			auto enty = Entry(ce+offset, ce->keyLen, ce->valLen);
			return enty;
		}
	}
	void diskCache(int fd, off_t offset){
		//TODO: Implement disk caching;
		throw std::runtime_error("Unimplemented...");
	}
};
template <class KeyType,
			class ValueType,
			data_ptr (srlze) (KeyType, ValueType),
			void (deserialize) (const void*, uint, uint, KeyType&, ValueType&),
			size_t (keyLen) (KeyType),
			size_t (valLen) (ValueType)>
std::ostream &operator<<(std::ostream &os, Entry<KeyType,ValueType,srlze,deserialize,keyLen,valLen> entry) { 
    return os << "Key: " << entry.getKey() << std::endl << "Value: " << entry.getVal() << std::endl;
}