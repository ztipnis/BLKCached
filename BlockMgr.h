#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <iostream>
#include <cstdint>
typedef unsigned int uint;

#ifndef off64_t
#define off64_t off_t
#endif

static void* allocator(int number, int size);
template <uint64_t BLOCK_SIZE, uint64_t (*hash_func)(const std::string&)>
#define buckets (nblocks * (BLOCK_SIZE/sizeof(uint64_t)));
class BlockMgr {
	//private variables
private:
	void* start;
	const float table_cache_ratio;
	uint64_t nblocks;
	
	//public functions
public:
	BlockMgr(uint64_t max_mem, float ratio) : table_cache_ratio(ratio), nblocks(max_mem/BLOCK_SIZE), start( allocator(max_mem/BLOCK_SIZE, BLOCK_SIZE) ) {}
	~BlockMgr(){
		free(start);
	}
	std::string get(std::string &key){
		uint block, itm;
		hash_to_bucket(key, block, itm);

	}
	void put(const std::string &key, const std::string &value){
		uint block, itm;
		hash_to_bucket(key, block, itm);
		uint64_t* blk = static_cast<uint64_t*>(get_block(block));
		if(blk[itm] > 0){
			//robinhood
			
		}else{
			void* adr = cache(key, value);
			data = pack(hash_func(key), reinterpret_cast<uint64_t>(adr), false);
		}
		set_block(block, 0, blk, BLOCK_SIZE);
	}
	//private member functions/helpers
private:
	void 
	uint scanCache(){
		static uint next_cache_off = 0;
		#define max ((1-table_cache_ratio)*nblocks) * BLOCK_SIZE
		if(next_cache_off + sizeof(cacheobj) > max){
			uint minblock = 0;
			uint minoff = 0;
			uint minhit = ~0;
			for(int i = (1-table_cache_ratio) * nblocks; i < nblocks; i++){
				//iterate blocks
				void* block = get_block(i);
				for(int j = 0; j < BLOCK_SIZE; j += sizeof(cacheobj)){
					cacheobj *co = reinterpret_cast<cacheobj*>(block);
					if(co->hitct < minhit){
						minblock = i;
						minoff = j;
					}
				}
				free(block);
			}
			return (minblock * BLOCK_SIZE) + (minoff * sizeof(cacheobj));
		}else{
			return next_cache_off;
		}
		#undef max
	}
	void* cache(const std::string &key, const std::string &val){
		//cache the key
		uint offset = scanCache();
		uint block = offset / BLOCK_SIZE;
		uint boff = offset % BLOCK_SIZE;
		cacheobj* cacheblock = reinterpret_cast<cacheobj*>(get_block(block));
		cacheblock += boff;
		cacheobj co;
		co->key = key;
		co->hitct = 0;
		co->val = val;
		*cacheblock = co;
		cacheblock -= boff;
		set_block(block, boff, static_cast<void*>(cacheblock), sizeof(cacheobj));
		free(cacheblock);
		return cacheblock;
	}
	void* get_block(uint blockno){
		//TODO: implement get block
		void* block = calloc(1, BLOCK_SIZE);
		uint64_t* bstart = static_cast<uint64_t*>(start);
		std::memcpy(block, bstart+(blockno * BLOCK_SIZE), BLOCK_SIZE);
		return block;
	}
	void set_block(uint blockno, uint64_t offset, void* data, uint size){
		//TODO: implement set block
		uint64_t* bstart = static_cast<uint64_t*>(start);
		uint64_t* block = static_cast<uint64_t*>(calloc(1, BLOCK_SIZE));
		std::memcpy(block, bstart+(blockno * BLOCK_SIZE), BLOCK_SIZE);
		std::memcpy((block+offset), data, size);
		std::memcpy(bstart+(blockno * BLOCK_SIZE), block, BLOCK_SIZE);
	}
	typedef struct cacheobj {
		unsigned int hitct;
		std::string key;
		std::string val;
	} cacheobj;
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

static void* allocator(int number, int size){
	return calloc(number, size);
}