#define _LARGEFILE64_SOURCE
#define tpairs (nblocks * bsize/8);
#include <sys/types.h>
#include <iostream>
#include <cstdint>
typedef unsigned int uint;

#ifndef off64_t
#define off64_t off_t
#endif

static uint64_t nblocks = 0;
static uint64_t bsize = 0;
static uint64_t start = 0;


uint64_t hash(const char* key){
	uint h;
	/**
	 * Hash the key
	 */
	return h;
}


static uint64_t pack(uint64_t hash, uint64_t address, bool ondisk){
	if(address > uint64_t(0x7FFFFFFFFFFF)) throw std::runtime_error("Address Out Of Bounds");
	uint64_t addr = static_cast<uint64_t>(address) & 0x00007FFFFFFFFFFF;
	uint64_t od = 0x8000000000000000 * uint64_t(ondisk ? 1 : 0);
	uint64_t hnum = (hash << 47) & (0x7FFF800000000000);
	return (od | hnum | address);
}
static void unpack(uint64_t packed, uint16_t& keyHash, off64_t& address, bool& ondisk){
	keyHash = static_cast<uint16_t>((packed << 1) >> 48); // overflow last bit, then bitshift
	ondisk = static_cast<bool>(packed >> 63);
	uint64_t bitmask = uint64_t(0x7FFFFFFFFFFF);
	address = static_cast<off64_t>(packed & bitmask);
}

int main(){
	int key = 0xABAB;
	void* val = malloc(sizeof(std::string));
	*((std::string*)val) = "test";
	bool ondisk = false;
	uint64_t packed = pack(key,reinterpret_cast<uint64_t>(val), ondisk);
	std::cout << std::hex << packed << ":" << std::bitset<64>(packed) << std::endl;
	uint16_t nkey;
	off64_t nval;
	bool od;
	unpack(packed, nkey, nval, od);
	std::cout << sizeof(packed);
	std::cout << (od ? "true" : "false") << " " << std::hex << nkey << " " << std::hex << nval << " " << *(reinterpret_cast<std::string*>(nval))  << std::endl;
	std::cout << sizeof(std::string) << std::endl;
	free((std::string*)val);
}
