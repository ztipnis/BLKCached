#pragma once
#include <unordered_map>
#include <set>
#include <mutex>
#include <atomic>

#ifdef DEBUG
#include "memory_debug.h"
#else
#include "memory_production.h"
#endif

#ifndef BLKCACHE_STORE
#define BLKCACHE_STORE

#ifndef DEFAULT_BLOCK_SIZE
#define DEFAULT_BLOCK_SIZE 4096
#endif

#define NEEDS_BIT(N, B)     (((unsigned long)N >> B) > 0)

#define BITS_TO_REPRESENT(N)                            \
        (NEEDS_BIT(N,  0) + NEEDS_BIT(N,  1) + \
         NEEDS_BIT(N,  2) + NEEDS_BIT(N,  3) + \
         NEEDS_BIT(N,  4) + NEEDS_BIT(N,  5) + \
         NEEDS_BIT(N,  6) + NEEDS_BIT(N,  7) + \
         NEEDS_BIT(N,  8) + NEEDS_BIT(N,  9) + \
         NEEDS_BIT(N, 10) + NEEDS_BIT(N, 11) + \
         NEEDS_BIT(N, 12) + NEEDS_BIT(N, 13) + \
         NEEDS_BIT(N, 14) + NEEDS_BIT(N, 15) + \
         NEEDS_BIT(N, 16) + NEEDS_BIT(N, 17) + \
         NEEDS_BIT(N, 18) + NEEDS_BIT(N, 19) + \
         NEEDS_BIT(N, 20) + NEEDS_BIT(N, 21) + \
         NEEDS_BIT(N, 22) + NEEDS_BIT(N, 23) + \
         NEEDS_BIT(N, 24) + NEEDS_BIT(N, 25) + \
         NEEDS_BIT(N, 26) + NEEDS_BIT(N, 27) + \
         NEEDS_BIT(N, 28) + NEEDS_BIT(N, 29) + \
         NEEDS_BIT(N, 30) + NEEDS_BIT(N, 31)   \
        )

extern std::unordered_map<std::string,std::string> config;

namespace BLKCACHE{
	template <unsigned long>
	class Block;
	/**
	 * @brief      Class for store: Stores keys/value REFERENCES in a map. Handles storage of blocks.
	 *
	 * @tparam     BLOCK_SIZE  { description }
	 */
	template <unsigned long BLOCK_SIZE>
	class Store{
	private:
		std::vector<ref> deleted;
		//default constructor
		Store() = default;
		//running count of last created block number
		long long lastBlockno = 0;
		//reference to item within block
		struct ref {
			size_t offset : BITS_TO_REPRESENT(BLOCK_SIZE);
			size_t size : BITS_TO_REPRESENT(BLOCK_SIZE);

			Block<BLOCK_SIZE>* block;
			Block<BLOCK_SIZE>* operator->(){ return block; }
			const std::string operator*() const{ return block->get(offset); }
			ref(){}
			ref(ssize_t bo, ssize_t sz,  Block<BLOCK_SIZE>* b): offset(bo), size(sz), block(b){};
			ref(const ref& r): offset(r.offset), block(r.block){};
		};
		typedef struct ref ref;
		//reference to block
		struct bref {
			Block<BLOCK_SIZE>* b;
			int s;
			bool operator<(const bref& b2) const{
				if((b) && (b2.b))
					return *b < *(b2.b);
				else if(b)
					return b->free_space < b2.s;
				else if(b2.b){
					return s < b2.b->free_space;
				}else{
					throw;
				}
			}
			Block<BLOCK_SIZE> *operator->(){
				return b;
			}
			Block<BLOCK_SIZE>& operator*(){
				return *b;
			}
			bref(Block<BLOCK_SIZE> *_b): b(_b){}
			bref(long long i): s(i){}
		};
		typedef struct bref bref;
		//KV store
		std::unordered_map<std::string, ref> _store;
		//Set of blocks managed by this instance
		std::set<bref> blocks;
	public:
		//No Copy Constructor - Singleton only
		Store(Store const&)               = delete;
		//No Assignment Operator - Singleton Only
        void operator=(Store const&)  	  = delete;
        //Get instance of Store
		static auto &getInst(){
			//initialize initial store
			static std::shared_ptr<Store> inst(new Store());
			//make atomic
			static std::atomic<Store*> inst_atomic(inst.get());
			//return atomic instance
			return inst_atomic;
		}
		/**
		 * @brief      Put K/V pair into map, storing data in a block
		 *
		 * @param[in]  key    The key
		 * @param[in]  value  The value
		 */
		void put(std::string key, std::string value){
			auto blk = blocks.lower_bound(value.length() * sizeof(char));
			if(blk != blocks.end()){
				//Block found which can fit
				blocks.erase(blk)
				for (auto it = begin(deleted); it != end (deleted); ++it) {
				    if(deleted->block == blk.b){
				    	blk->del(deleted->offset);
				    }
				}
				auto loc = (*blk).b->put(value);
				blocks.insert(blk, *blk);
				const ref& r = ref(loc, sizeof(value.c_str()), (*blk).b);
				_store[value] = r;
			}else{
				//create new block
				auto b = new Block<BLOCK_SIZE>(lastBlockno++, MEMORY::get, MEMORY::set);
				auto loc = b->put(value);
				blocks.insert(bref(b));
				_store[key] = ref(loc,sizeof(value.c_str()), b);
			}
		}
		/**
		 * @brief      Determines if KV Store contains key.
		 *
		 * @param[in]  key   The key
		 *
		 * @return     True if contains key, False otherwise.
		 */
		bool containsKey(std::string key){
			return !(_store.find(key) == _store.end());
		}
		/**
		 * @brief      Get data from KV Store
		 *
		 * @param[in]  key   The key
		 *
		 * @return     value associated with key
		 */
		std::string get(std::string key){
			ref& r = _store[key];
			return *r;
		}
		/**
		 * @brief      Delete K/V pair
		 *
		 * @param[in]  key   The key
		 *
		 * @return     the value that was assocaited with key
		 */
		void del(std::string key){
			//auto ret = _store[key]->del(_store[key].offset);
			auto ref = _store[key];
			deleted.push_bak(ref);
			ref.block->free_space += ref.size;
			_store.erase(key);
			//return ret;
		}
		/**
		 * @brief      For each key, exec callback
		 *
		 * @param[in]  cb    Callback Function
		 *
		 * @tparam     F     Function Type [std::function, lambda function, void
		 *                   (fnptr&)(std::string)]
		 */
		template <typename F>
		void forKeys(F cb){
			for(auto kv : _store){
				cb(kv.first);
			}
		};
		/**
		 * @brief      Destroys the store, leaving on-disk data intact (recovery
		 *             purposes, will be overwritten next start if unrecovered.
		 */
		~Store(){
			for(auto b: blocks){
				std::lock_guard<std::mutex> lgb(b->blkMTX);
				delete b.b;
			}
			#ifdef DEBUG
			free(BLKCACHE::MEMORY::mem);
			#endif
		}
		
	};
}


#endif