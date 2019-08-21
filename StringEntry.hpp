#include "Entry.hpp"
#include <string>
data_ptr SerializeString(std::string key, std::string val){
	data_ptr p = {0};
	std::string full = key + val;
	size_t size = full.length() + 1;
	p.data = malloc(size);
	std::memcpy(p.data, full.data(), size);
	p.length = size + 1;
	return p;
}
void DeSerializeToString(const void* data, uint key_size, uint value_size, std::string& key, std::string& value){
	//std::cout << std::endl << key_size << std::endl;
	//std::cout << value_size << std::endl;
	std::string pair((const char*) data);
	//std::cout << pair << std::endl;
	key = pair.substr(0, key_size);
	value = pair.substr(key_size, value_size);
}
inline size_t LenForString(std::string s){
	return s.length() * sizeof(char);
}
class StringEntry: public Entry<std::string, std::string, SerializeString, DeSerializeToString, LenForString, LenForString>{
	using Entry::Entry;
	public:
	StringEntry(std::string s, std::string v) : Entry(s, v){}
	size_t size(){
		return key.length() + value.length();
	}
	const char* Serialize(){
		std::string full = key + value;
		char* ret = (char*)malloc(full.length() + 1);
		memcpy(ret, full.c_str(), (full.length() + 1));
		return ret;
	}
	static StringEntry fromCache(cacheEntry *ce){
		if(ce->magic != CKMAGIC){
			throw std::runtime_error("Cache Entry Not Valid... Unable to Recover");
		}else{
			int offset = ce->offset;
			auto enty = StringEntry(((char*)ce)+offset, ce->keyLen, ce->valLen);
			return enty;
		}
	}


};