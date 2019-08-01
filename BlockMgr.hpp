#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <iostream>
#include <cstdint>
typedef unsigned int uint;

#ifndef off64_t
#define off64_t off_t
#endif

extern void* allocator(int number, int size);
template <uint64_t BLOCK_SIZE, uint64_t (*hash_func)(const std::string&)>
#define buckets (nblocks * (BLOCK_SIZE/sizeof(uint64_t)));
class BlockMgr {
	//private variables
private:
	void* start;
	const float table_cache_ratio;
	uint64_t nblocks;
	typedef struct cachekey {
		unsigned int hitct;
		ssize_t offset;
		size_t length;
	} cachekey;
	
	//public functions
public:
	BlockMgr(uint64_t max_mem, float ratio) : table_cache_ratio(ratio), nblocks(max_mem/BLOCK_SIZE), start( allocator(max_mem/BLOCK_SIZE, BLOCK_SIZE) ) {}
	~BlockMgr(){
		free(start);
	}
	std::string get(std::string &key){
		uint block, itm;
		hash_to_bucket(key, block, itm);
		uint64_t* blk = static_cast<uint64_t*>(get_block(block));
		uint64_t item = blk[itm];
		uint16_t keyHash;
		off64_t address;
		bool ondisk;
		if(ondisk){
			throw std::runtime_error("Not Yet Implemented");
		}else{
			
		}
	}
	void put(const std::string &key, const std::string &value){
		uint block, itm;
		hash_to_bucket(key, block, itm);
		uint64_t* blk = static_cast<uint64_t*>(get_block(block));
		if(blk[itm] > 0){
			//robinhood

		}else{
			void* adr = cache(key, value);
			uint64_t data = pack(hash_func(key), reinterpret_cast<uint64_t>(adr), false);
		}
		set_block(block, blk);
	}
	//private member functions/helpers
private:
	void* get_block(uint blockno){
		void* block = calloc(1, BLOCK_SIZE);
		uint64_t* bstart = static_cast<uint64_t*>(start);
		std::memcpy(block, bstart+(blockno * BLOCK_SIZE), BLOCK_SIZE);
		return block;
	}
	void set_block(uint blockno, void* data){
		uint64_t* bstart = static_cast<uint64_t*>(start);
		std::memcpy(bstart+(blockno * BLOCK_SIZE), data , BLOCK_SIZE);
	}
	uint64_t cache(std::string &key, std::string &val){
		cachekey* offset;
		void* block_data;
		unsigned int csize = (key.length() + val.length() + 1) * sizeof(char);
		//size without null_terminator
		unsigned int keysize = (key.size()) * sizeof(std::string::value_type);
		unsigned int valsize = (val.size()) * sizeof(std::string::value_type);
		//for block in cache-dedicated blocks
		for(unsigned int bno = 1; bno <= (nblocks * (1-table_cache_ratio)); bno++){
			int ckoff = -1;
			int cdoff = -1;
			block_data = get_block((nblocks * table_cache_ratio) + bno);
			//for cache_key in block (from end)
			for(int i = BLOCK_SIZE - sizeof(cachekey); i >= 0; i -= sizeof(cachekey)){
				char *tmp = static_cast<char*>(block_data) + i;
				if(*tmp != char(0)){
					continue;
				}else if(*(tmp - (keysize+valsize)) != char(0)){
					goto nextBlock;
				}else{
					ckoff = i;
					break;
				}
			}
			if(ckoff != 0) for(int i = 0; i < ckoff - (keysize + valsize); i++){
				char *tmp = static_cast<char*>(block_data) + i;
				if(*tmp == '\0'){
					for(int j = 1; j < keysize+valsize; j++){
						if(*(tmp+j) != '\0'){
							goto nextOff;
						}
					}
					cdoff = i;
					break;
				}
				nextOff:
				((void)0);
			}
			if(ckoff >= 0 && cdoff >= 0){
				cachekey* k = static_cast<cachekey*>(block_data) + ckoff;
				char* v = static_cast<cachekey*>(block_data) + cdoff;
				struct cachekey rk;
				rk.hitct = 0;
				rk.offset = ckoff-cdoff;
				rk.length = keysize+valsize;
				std::memcpy(k, &rk, sizeof(cachekey));
				std::memcpy(v,key.c_str(), keysize);
				std::memcpy(v+keysize, val.c_str(), valsize);
				set_block((nblocks * table_cache_ratio) + bno, block_data);
				free(block_data);
				return (nblocks * table_cache_ratio) + bno * BLOCK_SIZE + ckoff;
			}
			nextBlock:
				free(block_data);
		}
		return -1;

	}
	inline void hash_to_bucket(const std::string& key, uint& block, uint& itm){
		uint64_t hashed = hash_func(key);
		uint bucket = hashed % buckets;
		block = bucket / (BLOCK_SIZE/sizeof(uint64_t));
		itm = bucket % (BLOCK_SIZE/sizeof(uint64_t));
	}
	static bool cpr_hash(uint64_t hash, uint64_t hash2){
		if( (hash & ~0xFFFF800000000000) && (hash2 & ~0xFFFF800000000000)){
			return hash == hash2;
		}else{
			if((hash & ~0xFFFF800000000000)){
				return ((hash << 47) & (0x7FFF800000000000)) == hash2;
			}else if((hash2 & ~0xFFFF800000000000)){
				return ((hash2 << 47) & (0x7FFF800000000000)) == hash;
			}
		}
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


};

void* allocator(int number, int size){
	return calloc(number, size);
}