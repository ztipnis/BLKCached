#include <sys/types.h>
#include <iostream>
#include <cstdint>

#ifndef _LARGEFILE64_SOURCE
	#define _LARGEFILE64_SOURCE
#endif


#ifndef __BLOCKMANAGER_H
	#define __BLOCKMANAGER_H

typedef unsigned int uint;

#ifndef uint64_t
#define uint64_t off_t
#endif
#define CKMAGIC 0xCAC8EBE9 //0xCACHEKEY
#define buckets int(nblocks * (1.25*(BLOCK_SIZE)/sizeof(uint64_t)));

#ifndef mem_alloc
#define mem_alloc(number, size) calloc(number, size)
#endif

extern void* mem_alloc(size_t number, size_t size);

template <uint64_t BLOCK_SIZE, uint64_t (*hash_func)(const std::string&)>
class BlockMgr {
	//private variables
private:
	void* start;
	const float table_cache_ratio;
	uint64_t nblocks;
	typedef struct cachekey {
		static const unsigned int magic = CKMAGIC;
		unsigned short hitct;
		signed short offset;
		unsigned short length;
		unsigned short klen;
	} cachekey;
	
	typedef uint64_t bucket[BLOCK_SIZE/sizeof(uint64_t)];
	bucket& toBucket(void* block){
		uint64_t* block_rdata = reinterpret_cast<uint64_t*>(block);
		bucket& blkarray = *reinterpret_cast<bucket*>(block_rdata);
		return blkarray;
	}
	//public functions
public:
	BlockMgr(uint64_t max_mem, float ratio) : table_cache_ratio(ratio), nblocks(max_mem/BLOCK_SIZE), start( mem_alloc(max_mem/BLOCK_SIZE, BLOCK_SIZE) ) {}
	~BlockMgr(){
		free(start);
	}
	std::string get(const char* key){
		std::string str(key);
		return get(str, 0);
	}
	void put(const std::string &key, const std::string &value){
		uint block, itm;
		hash_to_bucket(key, block, itm, 0);
		bucket& blk = toBucket(get_block(block));
		if(blk[itm] > 0){
			//robinhood
			rh(key, value, 1);
		}else{
			uint64_t adr = cache(key, value);
			uint64_t hashed = hash_func(key);
			uint64_t data = pack(hashed, adr, false);
			blk[itm] = data;
			//std::cout << std::hex << data << std::endl;
			std::cout << std::flush << std::endl;
			std::cout << '\0';
		}
		set_block(block, blk);
		//free(blk);
	}
	//private member functions/helpers
private:
	std::string get(std::string &key, int offset){
		uint block, itm;
		hash_to_bucket(key, block, itm, offset);
		if(block >= table_cache_ratio * nblocks) throw std::runtime_error("Value does not exist");
		bucket& blk = toBucket(get_block(block));
		uint64_t item = blk[itm];
		uint16_t keyHash;
		uint64_t address;
		bool ondisk;
		std::cout << '\0';
		unpack(item, keyHash, address, ondisk);
		std::cout << "GET: trying " << block << " - " << itm << std::endl;
		std::cout << "Found: " << item << std::endl;
		if(!cpr_hash(hash_func(key), keyHash)){
			//robinhood
			std::cout << "Hashes don't match" << std::endl;
			return get(key, offset + 1);
		}
		const char* pair = retrieve(item);
		std::cout << address << " : " << pair << std::endl;
		std::cout << '\0';
		free(blk);
		unsigned int keysize = (key.size()) * sizeof(std::string::value_type);
		char* _key = (char*)calloc(sizeof(char), keysize + 1);
		strncpy(_key, pair, keysize);
		char* val = (char*)calloc(sizeof(char), sizeof(pair) - keysize + 1);
		strncpy(val, (pair + keysize), sizeof(pair) - keysize + 1);
		if(key == _key){
			std::string _val(val);
			free((void*)pair);
			free(_key);
			free(val);
			return _val;
		}else{
			std::cout << "Key no match" << std::endl;
			std::cout << key << " : " << _key << std::endl;
			free((void*)pair);
			free(_key);
			//robinhood
			return get(key, offset + 1);
			//return key;
		}
	}
	void rh(const std::string &key, const std::string &val,const int offset){
		uint block, itm;
		hash_to_bucket(key, block, itm, offset);
		std::cout << "PUT: trying " << block << " - " << itm << std::endl;
		bucket& blk = toBucket(get_block(block));
		if(blk[itm] != 0){
			std::cout << "Bucket is not empty" << std::endl;
			//which one is farther from home?
			//I'm 'offset' away from home!
			//
			//get the metadata for the other pair
			const char* pair = retrieve(blk[itm]);
			cachekey* ck = metadata(blk[itm]);
			//check for an error real quick
			if(!(ck == nullptr)){
				//figure out the key
				char* oKey = (char*)calloc(ck->klen + 1, sizeof(char));
				std::strncpy(oKey, pair, ck->klen);
				//got null terminated key, now hash it
				uint oBlock, oItm;
				hash_to_bucket(std::string(oKey), oBlock, oItm, 0);
				//now, how far are you from home?
				uint oOffset = (abs((int)block - (int)oBlock) * BLOCK_SIZE / sizeof(uint64_t)) + abs((int)itm - (int)oItm) + UINT_MAX + 1;
				if(oOffset >= offset){
					//If I'm at most equally as far from home, I'll keep on moving
					std::cout << "I'll move" << std::endl;
					free(blk);
					rh(key, val, offset + 1);
				}else{
					//I'm commendeering this bucket!
					std::cout << "Move over!" << std::endl;
					char* oVal = (char*)calloc((ck->length - ck->klen) + 1, sizeof(char));
					std::strncpy(oVal, pair + ck->klen, ck->length - ck->klen);
					uint64_t adr = cache(key, val);
					uint64_t hashed = hash_func(key);
					uint64_t data = pack(hashed, adr, false);
					//back up the old data
					uint16_t keyHash;
					uint64_t address;
					bool ondisk;
					unpack(blk[itm], keyHash, address, ondisk);
					blk[itm] = data;
					set_block(block, blk);
					free(blk);
					rh(std::string(oKey), std::string(oVal), 0, address);
					free(oVal);
				}
				free(oKey);
			}else{
				std::cout << "Couldn't find metadata! :(" << std::endl;
				std::cout << "I'll move" << std::endl;
				free(blk);
				rh(key, val, offset + 1);
			}
		}else{
			uint64_t adr = cache(key, val);
			uint64_t hashed = hash_func(key);
			uint64_t data = pack(hashed, adr, false);
			blk[itm] = data;
			std::cout << "Putting " << data << " in empty bucket" << std::endl;
			std::cout << '\0';
			set_block(block, blk);
			//free(blk);
		}
	}
	void rh(const std::string &key, const std::string &val,const int offset, uint64_t ckAddr){
		uint block, itm;
		hash_to_bucket(key, block, itm, offset);
		std::cout << "PUT: trying " << block << " - " << itm << std::endl;
		bucket& blk = toBucket(get_block(block));
		if(blk[itm] > 0){
			//which one is farther from home?
			//I'm 'offset' away from home!
			//
			//get the metadata for the other pair
			const char* pair = retrieve(blk[itm]);
			cachekey* ck = metadata(blk[itm]);
			//check for an error real quick
			if(!(ck == nullptr)){
				//figure out the key
				char* oKey = (char*)calloc(ck->klen + 1, sizeof(char));
				std::strncpy(oKey, pair, ck->klen);
				//got null terminated key, now hash it
				uint oBlock, oItm;
				hash_to_bucket(std::string(oKey), oBlock, oItm, 0);
				//now, how far are you from home?
				uint oOffset = (abs((int)block - (int)oBlock) * BLOCK_SIZE / sizeof(uint64_t)) + abs((int)itm - (int)oItm) + UINT_MAX + 1;
				if(oOffset >= offset){
					//If I'm at most equally as far from home, I'll keep on moving
					free(blk);
					rh(key, val, offset + 1, ckAddr);
				}else{
					//I'm commendeering this bucket!
					char* oVal = (char*)calloc((ck->length - ck->klen) + 1, sizeof(char));
					std::strncpy(oVal, pair + ck->klen, ck->length - ck->klen);
					uint64_t adr = ckAddr;
					uint64_t hashed = hash_func(key);
					uint64_t data = pack(hashed, adr, false);
					//back up the old data
					uint16_t keyHash;
					uint64_t address;
					bool ondisk;
					unpack(blk[itm], keyHash, address, ondisk);
					blk[itm] = data;
					set_block(block, blk);
					free(blk);
					rh(std::string(oKey), std::string(oVal), 0, address);
					free(oVal);
				}
				free(oKey);
			}
		}else{
			uint64_t adr = ckAddr;
			uint64_t hashed = hash_func(key);
			uint64_t data = pack(hashed, adr, false);
			blk[itm] = data;
			std::cout << std::hex << data << std::endl;
			std::cout << std::flush << std::endl;
			std::cout << '\0';
			set_block(block, blk);
			//(blk);
		}
	}

	cachekey* metadata(const char* forPair, void* inBlock){
		cachekey* cBlk = static_cast<cachekey *>(inBlock);
		for(cachekey* i = cBlk + (BLOCK_SIZE - sizeof(cachekey)); reinterpret_cast<uint64_t>(i) >= reinterpret_cast<uint64_t>(cBlk); i -= sizeof(cachekey)){
			if(!(i->magic == CKMAGIC)){
				continue;
			}
			if(reinterpret_cast<const char*>(i) - (i->offset) == forPair){
				return i;
			}
		}
		return nullptr;
	}
	cachekey* metadata(uint64_t packed){
		uint16_t keyHash;
		uint64_t address;
		bool ondisk;
		unpack(packed, keyHash, address, ondisk);
		if(ondisk){
			throw std::runtime_error("Not Yet Implemented");
		}else{
			uint64_t block = address/BLOCK_SIZE;
			uint off = address % BLOCK_SIZE;
			void* blok = get_block(block);
			std::cout << '\0';
			cachekey* ck = static_cast<cachekey*>(blok) + off;
			unsigned int keysize = ck->klen;
			const char* pairaddr = static_cast<char*>((char*)(ck - (ck->offset)));
			return metadata(pairaddr, blok);
		}
	}

	void move(char* pair, char* dest, void* block){
		cachekey* k = metadata(pair, block);
		std::memmove(dest, pair, k->length);
		k->offset = reinterpret_cast<uint64_t>(k-dest);
	}
public:
	void collect(){
		for(unsigned int bno = 0; bno <= (nblocks * (1-table_cache_ratio)); bno++){
			void* block_data = get_block((nblocks * table_cache_ratio) + bno);
			cachekey* ck;
			for(ck = static_cast<cachekey*>(block_data) + BLOCK_SIZE - sizeof(cachekey);
				(reinterpret_cast<uint64_t>(ck) >= reinterpret_cast<uint64_t>(block_data)) && (ck->magic == CKMAGIC);
				ck -= sizeof(cachekey));
			for(char* i = static_cast<char*>(block_data); i < static_cast<char*>(ck); i += sizeof(char)){
				if(*i == '\0'){
					char* j;
					for(j = i; j < static_cast<char*>(ck); j += sizeof(char)){
						if(*j != '\0') break;
					}
					if(j != ck){
						move(j,i, block_data);
					}else{
						break;
					}
				}
			}
			set_block((nblocks * table_cache_ratio) + bno, block_data);
			//free(block_data);
		}
	}
private:

	const char* retrieve(uint64_t packed){
		uint16_t keyHash;
		uint64_t address;
		bool ondisk;
		unpack(packed, keyHash, address, ondisk);
		if(ondisk){
			throw std::runtime_error("Not Yet Implemented");
		}else{
			unsigned long adr = static_cast<uint64_t>(address + ULONG_MAX + 1);
			uint64_t block = adr/BLOCK_SIZE;
			uint64_t off = adr - ( block * BLOCK_SIZE );
			void* blok = get_block(block);
			std::cout << '\0';
			if(blok){
				cachekey* ck = reinterpret_cast<cachekey*>(blok) + off;
				if(ck && ck->magic == CKMAGIC){
					unsigned int keysize = ck->klen;
					const char* pairaddr = static_cast<char*>((char*)(ck - (ck->offset)));
					char* pair = (char*)calloc(sizeof(char), ck->length + 1);
					strncpy(pair, pairaddr, ck->length);
					free(blok);
					return pair;
				}
				free(blok);
				return nullptr;
			}
			free(blok);
			return nullptr;
		}
	}
	void* get_block(uint blockno){
		void* block = (char*)calloc(1, BLOCK_SIZE);
		char* bstart = static_cast<char*>(start);
		std::memcpy(block, bstart+(blockno * BLOCK_SIZE), BLOCK_SIZE);
		return block;
	}
	void set_block(uint blockno, void* data){
		char* bstart = static_cast<char*>(start);
		std::memcpy(bstart+(blockno * BLOCK_SIZE), data , BLOCK_SIZE);
		std::cout << "Set bucket to: ";
		/*for(uint* i = (uint*) data; i < ((uint*) data) + BLOCK_SIZE; i+= sizeof(uint64_t)){
			if(i){
				std::cout << std::hex << *i;
			}else{
				std::cout << std::hex << 0;
			}
		}
		std::cout << std::endl;*/
		free(data);
	}
uint64_t cache(const std::string &key, const std::string &val){
		int fcblock = table_cache_ratio * nblocks;
		int pairsize = (key.size() + val.size()) * sizeof(char);

		for(int block = fcblock; block < nblocks; block++){
			char* first_null = nullptr;
			char* last_null = nullptr;

			void* raw = get_block(block);
			char* cRaw = static_cast<char*>(raw);
			for(first_null = cRaw; *first_null != '\0' && first_null < cRaw + BLOCK_SIZE; first_null += sizeof(char));
			//first_null is now address of first null byte			
			for(last_null = cRaw + BLOCK_SIZE - sizeof(char); *last_null != '\0' && last_null > first_null; last_null -= sizeof(char));
			//last_null is now address of last null byte

			restart_first_half:
			for(char* i = first_null; i < first_null + reinterpret_cast<ptrdiff_t>((first_null - last_null)/2); i+= sizeof(char)){
				if(*i != '\0'){
					while(*i != '\0') i+=sizeof(char);
					first_null = i;
					goto restart_first_half;
				}
			}
			restart_second_half:
			for(char* i = last_null - sizeof(char); i >= first_null + reinterpret_cast<ptrdiff_t>((first_null - last_null)/2); i -=sizeof(char)){
				if(!i || *i != '\0'){
					while(!i || *i != '\0') i-=sizeof(char);
					last_null = i;
					goto restart_second_half;
				}
			}
			if(last_null - first_null > (pairsize + sizeof(cachekey))) {
				char* pair = first_null;
				std::memcpy(pair,key.c_str(),key.size() * sizeof(char));
				std::memcpy(pair+(key.size()*sizeof(char)), val.c_str(), val.size() * sizeof(char));
				uint64_t ckoff = reinterpret_cast<uint64_t>(last_null - sizeof(cachekey)) - reinterpret_cast<uint64_t>(cRaw);
				cachekey* ck = reinterpret_cast<cachekey*>(raw) + ckoff;
				*ck = (cachekey){0};
				ck->hitct = 0;
				ck->offset = reinterpret_cast<long>((last_null - sizeof(cachekey)) - pair);
				ck->length = pairsize;
				ck->klen = key.size();
				std::memmove(first_null, pair, pairsize);
				//std::memmove(raw, cRaw, BLOCK_SIZE);
				set_block(block,raw);
				//free(raw);
				return (block * BLOCK_SIZE) + ckoff;
			}
		}
		throw std::runtime_error("out of memory");
	}
	inline void hash_to_bucket(const std::string& key, uint& block, uint& itm, int offset){
		uint64_t hashed = hash_func(key) + offset;
		uint bucket = hashed % buckets;
		block = bucket / (BLOCK_SIZE/sizeof(uint64_t));
		itm = bucket % (BLOCK_SIZE/sizeof(uint64_t));
	}
	bool cpr_hash(uint64_t hash, uint64_t hash2){
		return ((hash & 0x7FFF8) == (hash2 & 0x7FFF8));
	}
	static uint64_t pack(uint64_t hash, uint64_t address, bool ondisk){
		if(address > uint64_t(0x7FFFFFFFFFFF)) throw std::runtime_error("Address Out Of Bounds");
		uint64_t addr = static_cast<uint64_t>(address) & 0x00007FFFFFFFFFFF;
		uint64_t od = 0x8000000000000000 * uint64_t(ondisk ? 1 : 0);
		uint64_t hnum = (hash << 47) & (0x7FFF800000000000);
		return (od | hnum | address);
	}
	static void unpack(uint64_t packed, uint16_t& keyHash, uint64_t& address, bool& ondisk){
		keyHash = static_cast<uint16_t>((packed << 1) >> 48); // overflow last bit, then bitshift
		ondisk = static_cast<bool>(packed >> 63);
		uint64_t bitmask = uint64_t(0x7FFFFFFFFFFF);
		address = static_cast<uint64_t>(packed & bitmask);
	}


};

void* mem_alloc(int number, int size){
	return (char*)calloc(number, size);
}
#endif