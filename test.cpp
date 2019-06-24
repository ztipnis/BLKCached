#define _LARGEFILE64_SOURCE
#define tpairs (nblocks * bsize/8);
#include <sys/types.h>
#include <iostream>
typedef unsigned int uint;

#ifndef off64_t
#define off64_t off_t
#endif

static uint nblocks = 0;
static uint bsize = 0;
static uint start = 0;


uint64_t hash(const char* key){
	uint h;
	/**
	 * Hash the key
	 */
	return h;
}
uint64_t pack(uint64_t hash, off64_t address, bool ondisk){
	if(address > uint64_t(0x7FFFFFFFFFFF)) throw std::runtime_error("Address Out Of Bounds");
	uint64_t addr = static_cast<uint64_t>(address);
	uint64_t od = 0x8000000000000000 * uint64_t(ondisk ? 1 : 0);
	return (od | (hash << 47) | address);
}
void unpack(uint64_t packed, uint16_t& keyHash, off64_t& address, bool& ondisk){
	keyHash = static_cast<uint16_t>((packed << 1) >> 48); // overflow last bit, then bitshift
	ondisk = static_cast<bool>(packed >> 63);
	uint64_t bitmask = uint64_t(0x7FFFFFFFFFFF);
	address = (packed & bitmask);
}


/*void allocate(uint nblocks, uint bsize){
	char* sector = static_cast<char*>(malloc(bsize * nblocks));
	for(int i = 0; i < bsize * nblocks; i += sizeof(unsigned char)){
		unsigned char* c = (unsigned char *) (sector + i);
		*c = '\0';
	}
	start = (uint) sector;
}

void put(const char* key, const char* value){
	uint pnum = hash(key) % tpairs;
	uint blockno = pnum / (bsize/8);
	uint pblkno = pnum % (bsize/8);
	uint startn = blockno * bsize;
	unsigned char currentKey[2] = (unsigned char*)((void*) start + startn + (pnum * 8));
	if(currentKey[0] == '\n' && currentKey[1] == '\n')
		//Set key to 2 MSB in hash
		//Set value to 6byte address in disk
}

void get(const char* key){
	uint pnum = hash(key) / tpairs;
	uint blockno = pnum / (bsize/8);
	uint pblkno = pnum % (bsize/8);
	uint startn = blockno * bsize;
	uint64_t 
}*/



int main(){
	int key = 0xABAB;
	off64_t val = 0x7CCCCCCCCCCC;
	bool ondisk = true;
	uint64_t packed = pack(key,val, true);
	std::cout << std::hex << packed << ":" << std::bitset<64>(packed) << std::endl;
	uint16_t nkey;
	off64_t nval;
	bool od;
	unpack(packed, nkey, nval, od);
	std::cout << od << " " << std::hex << nkey << " " << std::hex << nval << std::endl;
}
