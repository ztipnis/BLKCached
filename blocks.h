#define _LARGEFILE64_SOURCE
#define tpairs (nblocks * bsize/8);
#include <sys/types.h>
#include <iostream>
typedef unsigned int uint;
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
uint64_t pack(uint64_t hash, __off64_t address){
	if(address > ((1 << 49) - 1)) throw std::runtime_error("Address Out Of Bounds");
	uint64_t addr = static_cast<uint64_t> address;
	return ((hash << 48) | address);
}
void unpack(uint64_t packed, uint16_t& keyHash, __off64_t& address){
	keyHash = static_cast<uint16_t>(packed >> 48);
	uint64_t bitmask = (1 << 49) - 1;
	address = (packed & bitmask);
}


void allocate(uint nblocks, uint bsize){
	void* sector = malloc(bsize * nblocks);
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
}

uint32_t fast_range_probing(uint32_t hashed_key, uint32_t probe, uint32_t N)
{
  return ((uint64_t)hashed_key + ((uint64_t)probe << 32)/N) * N >> 32;
}

int main(){
	int key = 0xAB;
	int val = 0xCC;
	std::cout << std::hex << pack(key,val);
}
