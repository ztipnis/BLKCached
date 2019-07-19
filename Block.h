#include <cmath>
#include <atomic>
#include <cstring>

template<uint BLOCK_SIZE, uint NBLOCK, float mem_ratio>
class BlockMgr{
public:
	//No Copy Constructor - Singleton only
	BlockMgr(BlockMgr const&) = delete;
	//No Assignment Operator - Singleton Only
    void operator=(BlockMgr const&) = delete;
    //Atomic singleton
	static std::atomic<BlockMgr> &instance = []{
		static std::atomic<BlockMgr> inst();
		return inst;
	}()
	void put(std::string key, std::string value){


	}
	std::string get(std::string key){

	}
private:
	void* start;
	typedef struct cachEntry{
		uint64_t hash,
		std::string val,
		unsigned short hits;
	} cachEntry;

	uint64_t cache(std::string key, std::string value){
		uint64_t min_hit_off = 0;
		uint16_t min_hit_ct = 0xFFFF;
		for(uint i = floor(NBLOCK * mem_ratio); i < NBLOCK; i++){
			void* blk = calloc(1, BLOCK_SIZE);
			uint64_t off = reinterpret_cast<uint64_t>(start) + (i*BLOCK_SIZE);
			std::memcpy(blk, reinterpret_cast<void*>(off), BLOCK_SIZE);
			for(uint j = 0; j < floor(BLOCK_SIZE / sizeof(cachEntry)); j++){
				uint64_t off2 = reinterpret_cast<uint64_t>(blk) + (j * sizeof(cachEntry));
				cachEntry* ce = *((cachEntry*) off2);
				if(ce->hash == 0){
					//empty
					ce->hash = hash(key);
					ce->val = value;
					ce->hits = 0;
					return off+off2;
				}else{
					//update min hits
					if(ce->hits < min_hit_ct){
						min_hit_off = off + off2;
						min_hit_ct = ce->hits;
					}
				}
			}
			free(blk);
		}
		//cache unsuccessful
		if(min_hit_off == 0){
			//this is probably an error
			throw std::runtime_error("Cache is corrupted...");
		}else{
			//
			/*uint bno = floor(min_hit_off / BLOCK_SIZE);
			uint coff = floor((min_hit_off - (bno * BLOCK_SIZE)));
			void* block = reinterpret_cast<void*>(reinterpret_cast<uint64_t*>(start)+(bno * BLOCK_SIZE));
			void* blk = calloc(1, BLOCK_SIZE);
			std::memcpy(blk, block, BLOCK_SIZE);*/
			throw std::runtime_error("Disk storage not implemented");

		}
	}
	BlockMgr(){
		start = calloc(NBLOCK, BLOCK_SIZE);
	}
	static uint64_t hash(std::string itm){
		return 0;
	}
	static uint64_t pack(uint64_t hash, uint64_t address, bool ondisk){
		if(address > uint64_t(0x7FFFFFFFFFFF)) throw std::runtime_error("Address Out Of Bounds");
		uint64_t addr = static_cast<uint64_t>(address);
		uint64_t od = 0x8000000000000000 * uint64_t(ondisk ? 1 : 0);
		return (od | (hash << 47) | address);
	}
	static void unpack(uint64_t packed, uint16_t& keyHash, off64_t& address, bool& ondisk){
		keyHash = static_cast<uint16_t>((packed << 1) >> 48); // overflow last bit, then bitshift
		ondisk = static_cast<bool>(packed >> 63);
		uint64_t bitmask = uint64_t(0x7FFFFFFFFFFF);
		address = static_cast<off64_t>(packed & bitmask);
	}
};

#if defined(DEFAULT_BLOCK_SIZE) && defined(DEFAULD_NBLOCK) && defined(DEFAULT_MEM_RATIO)
typedef BlockMgr<DEFAULT_BLOCK_SIZE, DEFAULD_MEM_SIZE, DEFAULT_MEM_RATIO> Block_Default_Manager;
#endif