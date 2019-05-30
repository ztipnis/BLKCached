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
	template <unsigned long BLOCK_SIZE>
	class Store{
	private:
		Store(){}
		static long long lastBlockno = 0;
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
			Block<BLOCK_SIZE> operator*(){
				return *b;
			}
			bref(Block<BLOCK_SIZE> *_b): b(_b){}
			bref(long long i): s(i){}
		};
		typedef struct bref bref;
		std::unordered_map<std::string, ref> _store;
		std::set<bref> blocks;
	public:
		Store(Store const&)               = delete;
        void operator=(Store const&)  	  = delete;
		static auto &getInst(){
			static Store* inst = new Store();
			static std::atomic<Store*> inst_atomic(inst);
			return inst_atomic;
		}
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
		bool containsKey(std::string key){
			return !(_store.find(key) == _store.end());
		}
		std::string get(std::string key){
			ref& r = _store[key];
			return *r;
		}
		std::string del(std::string key){
			auto ret = _store[key]->del(_store[key].offset);
			_store.erase(key);
			return ret;
		}
		void forKeys(auto cb){
			for(auto kv : _store){
				cb(kv.first);
			}
		}
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