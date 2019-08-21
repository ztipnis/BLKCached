#include "StringEntry.hpp"
#include "cacheEntry.hpp"
#include <sys/types.h>
#include <iostream>
#include <cstdint>
#include <vector>
#include <algorithm>

#ifndef _LARGEFILE64_SOURCE
	#define _LARGEFILE64_SOURCE
#endif

#ifndef __BLOCKMANAGER_H
	#define __BLOCKMANAGER_H

typedef unsigned int uint;

#define CKMAGIC 0xCAC8EBE9 //0xCACHEKEY
#define buckets int(nblocks*(BLOCK_SIZE)/sizeof(uint64_t))
#define bpblk int(BLOCK_SIZE/sizeof(uint64_t)) 
#define firstBlock 0
#define firtCacheBlock int(nblocks*table_cache_ratio)
#ifndef mem_alloc
#define mem_alloc(number, size) calloc(number, size)
#endif	

extern void* mem_alloc(size_t number, size_t size);
template <size_t BLOCK_SIZE, uint64_t (*hash_func)(const std::string&)>
class BlockMgr {
private:
	void* start;
	const float table_cache_ratio;
	uint64_t nblocks;
	typedef uint64_t bucket[BLOCK_SIZE/sizeof(uint64_t)];
public:
	BlockMgr(uint64_t max_mem, float ratio) : table_cache_ratio(ratio), nblocks(max_mem/BLOCK_SIZE), start( mem_alloc(max_mem/BLOCK_SIZE, BLOCK_SIZE) ) {}
	std::string get(std::string key){
		uint offset = 0;
		while(offset < buckets){
			uint bucket = hash_func(key);
			uint blkno = ((bucket+offset)/(bpblk))%nblocks;
			uint bkt = (bucket+offset % bpblk);
			uint64_t* blk = reinterpret_cast<uint64_t*>(get_block(blkno));
			uint64_t packed = blk[bkt];
			free(blk);
			if(packed != 0){
				StringEntry ret = get(packed);
				if(ret.getKey() != key){
					offset++;
					continue;
				}else{
					return ret.getVal();
				}
			}else{
				return "NOT FOUND";
			}
		}
		return "";
	}
	void put(std::string key, std::string value){
		StringEntry s(key,value);
		put(s);
	}
	void del(std::string key){
		uint offset = 0;
		while(offset < buckets){
			uint bucket = hash_func(key);
			uint blkno = ((bucket+offset)/(bpblk))%nblocks;
			uint bkt = (bucket+offset % bpblk);
			uint64_t* blk = reinterpret_cast<uint64_t*>(get_block(blkno));
			uint64_t packed = blk[bkt];
			if(packed != 0){
				StringEntry ret = get(packed);
				if(ret.getKey() != key){
					offset++;
					free(blk);
					continue;
				}else{
					del(packed);
					blk[bkt] = uint64_t{0};
					set_block(blkno, blk);
					free(blk);
					return;
				}
			}else{
				free(blk);
				return;
			}
		}
	}
private:
	void put(StringEntry &s){
		uint64_t cacheAddress = cacheStringEntry(s);
		uint64_t packed = pack(hash_func(s.getKey()), cacheAddress, false);
		robinhood(packed, hash_func(s.getKey()), 0);
	}
	StringEntry get(uint64_t packed){
		uint16_t keyHash;
		uint64_t address;
		bool ondisk;
		unpack(packed, keyHash, address, ondisk);
		uint block = address / BLOCK_SIZE;
		uint offset = address % BLOCK_SIZE;
		void* blk = get_block(block);
		cacheEntry* ce = reinterpret_cast<cacheEntry*>(((char*)blk) + offset);
		if(ce->magic == CKMAGIC){
			auto se = StringEntry::fromCache(ce);
			ce->hitct++;
			set_block(block, blk);
			free(blk);
			return se;
		}else{
			throw std::runtime_error("Entry Invalid");
		}
	}
	void del(uint64_t packed){
		uint16_t keyHash;
		uint64_t address;
		bool ondisk;
		unpack(packed, keyHash, address, ondisk);
		uint block = address / BLOCK_SIZE;
		uint offset = address % BLOCK_SIZE;
		void* blk = get_block(block);
		cacheEntry* ce = reinterpret_cast<cacheEntry*>(((char*)blk) + offset);
		if(ce->magic == CKMAGIC){
			memset (((char*)ce)+ce->offset, 0, ce->keyLen+ce->valLen);
			memset (ce, 0,sizeof(cacheEntry));
			set_block(block, blk);
			free(blk);
		}else{
			throw std::runtime_error("Entry Invalid");
		}
	}
	void robinhood(uint64_t packed, uint bucket, uint offset){
		uint blkno = ((bucket+offset)/(bpblk))%nblocks;
		uint bkt = (bucket+offset % bpblk);
		uint64_t* blk = reinterpret_cast<uint64_t*>(get_block(blkno));
		if(blk[bkt] == 0){
			blk[bkt] = packed;
			set_block(blkno,blk);
		}else{
			StringEntry me = get(packed);
			StringEntry you = get(blk[bkt]);
			if(me.getKey() == you.getKey()){
				blk[bkt] = packed;
			}else{
				//myOff = offset
				uint yourHash = hash_func(you.getKey());
				uint yourblkno = (yourHash / bpblk)%nblocks;
				uint yourbkt = (yourHash % bpblk);
				uint yourOff = ((abs((int)blkno-(int)yourblkno)*bpblk)+abs((int)bkt-(int)yourbkt));
				if(offset>yourOff){
					uint64_t packed2 = blk[bkt];
					blk[bkt] = packed;
					set_block(blkno,blk);
					robinhood(packed2, yourHash, yourOff+1);
				}else{
					robinhood(packed, bucket, offset + 1);
				}
			}
		}
		free(blk);
	}
	uint cacheStringEntry(StringEntry &s){
		//find the first contiguous free area
		long cacheEntry_off = -1;
		long dataBlock_off = -1;
		int blockno = -1;
		for(uint i = firtCacheBlock; i < nblocks; i++){
			char* blk = (char*)get_block(i);
			for(int cb = BLOCK_SIZE/sizeof(cacheEntry) - 1; cb >= 0; cb--){
				if(  ((cacheEntry*)(blk + (cb * sizeof(cacheEntry))))->magic != CKMAGIC ){
					cacheEntry_off = cb * sizeof(cacheEntry);
					break;
				}
			}
			if(cacheEntry_off < 0) continue;
			for(uint db = 0; db < cacheEntry_off; db+= sizeof(char)){
				if(*(blk + db) == (char){0}){
					uint oldb = db;
					for(uint j = db; j < db+s.size(); j+=sizeof(char)){
						if(*(blk+j) != (char){0}){
							db = j;
							break;
						}
					}
					if(db == oldb){
						dataBlock_off = db;
						blockno = i;
						break;
					}
				}
			}
			free(blk);
			if(blockno == i){
				break;
			}
		}

		if(cacheEntry_off <= 0 || dataBlock_off < 0 || blockno < 0){
			throw std::runtime_error("Sorry, moving cache to disk is not supported yet");
		}else{
			char* blk = (char*)get_block(blockno);
			s.cache((blk+dataBlock_off), (blk + cacheEntry_off));
			set_block(blockno, blk);
			free(blk);
			return blockno*BLOCK_SIZE + cacheEntry_off;
		}
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
	void* get_block(uint blockno){
		void* block = (char*)calloc(1, BLOCK_SIZE);
		char* bstart = static_cast<char*>(start);
		std::memcpy(block, bstart+(blockno * BLOCK_SIZE), BLOCK_SIZE);
		return block;
	}
	void set_block(uint blockno, void* data){
		char* bstart = static_cast<char*>(start);
		std::memcpy(bstart+(blockno * BLOCK_SIZE), data , BLOCK_SIZE);
	}
public:
	void collect(){
		for(int i = 0; i < nblocks; i++){
			void* _blk = get_block(i);
			std::vector<cacheEntry*> cEs;
			for(char* blk = (((char*)_blk) + BLOCK_SIZE - sizeof(char)); blk > _blk; blk -= sizeof(char)){
				if(((cacheEntry*)blk)->magic == CKMAGIC){
					cEs.push_back((cacheEntry*) blk);
				}
			}
			std::sort(cEs.begin(), cEs.end(), [](const cacheEntry* c1, const cacheEntry* c2){
				return ((*c1) < (*c2));
			});
			for(int j = 0; j < ((int)cEs.size()) - 1; j++){
				
					char* ths = (char*) cEs[j];
					int thsoff = cEs[j]->offset;
					uint64_t thslen = cEs[j]->keyLen + cEs[j]->valLen;
					char* that = (char*)cEs[j+1];
					int thatoff = cEs[j+1]->offset;
					uint64_t thatlen = cEs[j+1]->keyLen + cEs[j+1]->valLen;
				if(ths + thsoff + thslen < that + thatoff){
					//j [blank] j+1
					std::memmove(	(ths) + thsoff + thslen /*End of this KV Pair (dest) */,
									 (that) + thatoff  /*next data loc (src)*/,
									 thatlen/*next data length */);
					std::memset(	(ths + thsoff + thslen + thatlen),
									'\0',
									(that+thatoff) - (ths + thsoff + thslen));
					cEs[j+1]->offset = static_cast<int>((ths+thsoff+thslen) - that);
				}
			}
			set_block(i, _blk);
			/*format for debug*/
			std::string s((char*)_blk, (char*)_blk + BLOCK_SIZE);
			for(int j = 0; j < s.size(); j++){
				if(s[j] == '\0') s[j] = ' ';
			}
			free(_blk);
		}
	}
};

#endif


//
//Put:
//	Put in cache
//	Hash to Bucket
//	Pack Bucket
//	Find open Bucket
//
//
