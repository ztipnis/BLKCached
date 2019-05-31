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
		//default constructor
		Store() = default;
		//running count of last created block number
		long long lastBlockno = 0;
		//reference to item within block
		struct ref {
			size_t offset;
			Block<BLOCK_SIZE>* block;
			Block<BLOCK_SIZE>* operator->(){ return block; }
			const std::string operator*() const{ return block->get(offset); }
			ref(){}
			ref(ssize_t bo, Block<BLOCK_SIZE>* b): offset(bo), block(b){};
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
			static auto inst = std::make_shared<Store>(new Store());
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
				blocks.erase(blk);
				auto loc = (*blk).b->put(value);
				blocks.insert(blk, *blk);
				const ref& r = ref(loc, (*blk).b);
				_store[value] = r;
			}else{
				//create new block
				auto b = new Block<BLOCK_SIZE>(lastBlockno++, MEMORY::get, MEMORY::set);
				auto loc = b->put(value);
				blocks.insert(bref(b));
				_store[key] = ref(loc, b);
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
		std::string del(std::string key){
			auto ret = _store[key]->del(_store[key].offset);
			_store.erase(key);
			return ret;
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