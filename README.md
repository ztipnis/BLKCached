# BLKCached
PMEM Block Storage based in-memory database

## Table of Contents

   * [BLKCached](#blkcached)
      * [Files](#files)
         * [Block.h](#blockh)
            * [Class Block](#class-block)
            * [Struct RawMemory](#struct-rawmemory)
         * [Store.h](#storeh)
            * [Class Store](#class-store)
            * [Struct ref](#struct-ref)
            * [Struct bref](#struct-bref)
         * [configParser.h](#configparserh)
         * [memory_*.h](#memory_h)
         * [bds.conf](#bdsconf)
      * [Appendix](#appendix)
         * [Code for Headers](#code-for-headers)
            * [Block.h](#blockh-1)
         * [Store.h](#storeh-1)
            * [configParser.h](#configparserh-1)
            * [memory_debug.h](#memory_debugh)

## Files

### Block.h

Class for managing in-memory blocks of data (as well as for fetching those blocks from some block addressable storage medium)

#### Class Block

| Function    | Args                                 | Effect                                                       | Return                     |
| ----------- | ------------------------------------ | ------------------------------------------------------------ | -------------------------- |
| getInst     | None                                 | Calls default contructor on first call, after that returns atomic pointer to shared instance | ```std::atomic<Store*>&``` |
| get         | `std::string key`                    | Fetches reference to block/offset stored for given key, then dereferences to find value | `std::string`              |
| put         | `std::string key, std::string value` | Inserts value into the block with the lowest free space that is able to accomodate the string, then associates the key with the ref to that value | N/A                        |
| del         | `std::string key`                    | Fetches reference to block/offset for key, deletes te value from the block, and returns the old value | `std::string`              |
| containsKey | std::string key                      | Checks if store contains a certain key                       | `bool`                     |
| forKeys     | `<typename F> cb`                    | Calls the function pointer (or lambda function, etc.) cb, for each key. | N/A                        |

#### Struct RawMemory

Bridges malloc (allocate raw memory of certain size) to C++'s internal 

### Store.h

Singleton class which serves as a coordinator for blocks, and a storage for key/value ref pairs.

#### Class Store

| Function     | Args                                                         | Effect                                                       | Return        |
| ------------ | ------------------------------------------------------------ | ------------------------------------------------------------ | ------------- |
| Constructor  | `long long block, void (&_getter)(long long, void*), void (&_setter)(long long, void*)` | Creates a new Block Object at Block offset # (block), with function (\_getter) to fetch data from the disk, and (\_setter) to save to disk | N/A           |
| get          | `size_t offset`                                              | fetches null-terminated or RS- terminated char array starting at (offset) | `std::string` |
| put          | `std::string s`                                              | Inserts string (s) into block (if fits), returns offset of new char array | `ssize_t`     |
| del          | `size_t offset`                                              | Deletes string at (offset) and returns its old value         | `std::string` |
| getFreeSpace | None                                                         | Returns remaining space in block                             | `size_t`      |

#### Struct ref

Reference to block/offset. Contains a few helpful functions for dereferencing etc.

#### Struct bref

Shim for pointer to block, with operators for comparing blocks based on free space.

### configParser.h

Defines static unordered map with k/v pairs for config options and values.

### memory_*.h

Debug (memory-only), and Production (Persistent Memory [aka. on disk]) getters and setters used to emulate block-addressable mediums.

### bds.conf

Options used to configure k/v store and server daemon

| Option   | Values         | Desc.                                                        |
| -------- | -------------- | ------------------------------------------------------------ |
| poolSize | (int) 1+       | Size of pool (pre-allocated) in GB                           |
| path     | (string) [any] | Path to persistent memory store. Can be relative or absolute. |
| threads  | (int) 0+       | 0 => let system choose # of threads. 1+ => specific max number of threads (not yet implemented) |
| ip       | (ipaddr) [any] | The ip on which to listen for connections (not yet implemented) |
| port     | (int)[any]     | The port on which to listen for connections                  |



## Appendix

### Code for Headers



#### Block.h

```cpp
template <unsigned long BLOCK_SIZE>  class Block{
		friend class Store<BLOCK_SIZE>;
		private:
			/**
			 * @brief      Represents raw memory (void pointer) which is described by this block object
			 */
			struct RawMemory{
				void* mem;
				RawMemory();
				virtual ~RawMemory();
			};
			long long blockno; //number of the block within the segments (used by PMDK)
			size_t size; //total size of the objects represented by the block
			std::weak_ptr<RawMemory> raw; //pointer to the raw memory object that is stored by the block. Auto handles constructors and desctructors
			void (&getter)(long long, void*); //pointer to function to fetch the data from the disk
			void (&setter)(long long, void*); //pointer to function to store the data to the disk
			ssize_t free_space; //total free space remaining in the block
			std::mutex blkMTX; //mutex - ensures concurrent write requests don't currupt the data
			~Block(){};
		public:
			typedef struct RawMemory RawMemory;
			/**
			 * @brief      Creates a block
			 *
			 * @param[in]  block    The block number
			 * @param[in]  _getter  The getter function to fetch data from disk
			 * @param[in]  _setter  The setter function to store data on disk
			 */
			Block(long long block, void (&_getter)(long long, void*) , void (&_setter)(long long, void*) ): setter(_setter), getter(_getter), blockno(block), size(BLOCK_SIZE), free_space(BLOCK_SIZE){}
			/**
			 * @brief      get string which starts at specified offset
			 *
			 * @param[in]  offset  The offset at which the string starts
			 *
			 * @return    string stored at offset
			 */
			std::string get(size_t offset);
			/**
			 * @brief      put a string in the block
			 *
			 * @param[in]  s     string to store
			 *
			 * @return     the offset at which the string is stored
			 */
			ssize_t put(std::string s);
			/**
			 * @brief      delete data at offset
			 *
			 * @param[in]  offset  The offset
			 *
			 * @return     the string that was stored at offset
			 */
			std::string del(size_t offset);
			/**
			 * @brief      Gets the free space.
			 *
			 * @return     The free space.
			 */
			size_t getFreeSpace() const{
				return free_space;
			}
			bool operator<(const Block<BLOCK_SIZE>& b) const{
				return (free_space < b.free_space);
			}
			bool operator<(const size_t b) const{
				return (free_space < b);
			}
			bool operator>(const Block<BLOCK_SIZE>& b) const{
				return (free_space > b.free_space);
			}
			bool operator>=(const Block<BLOCK_SIZE>& b) const{
				return !(free_space >= b.free_space);
			}
			bool operator<=(const Block<BLOCK_SIZE>& b) const{
				return !(free_space <= b.free_space);
			}
		private:
			/**
			 * @brief      debug func prints the block data to the screen
			 */
			void print_stack() const;
			/**
			 * @brief      write the data to the disk
			 *
			 * @param[in]  lock_ptr  lock on the raw memory stored
			 */
			void write(std::shared_ptr<RawMemory> lock_ptr);
			/**
			 * @brief      keep the raw memory alive for operations
			 *
			 * @return     return strong pointer to the raw memory handled by the block.
			 */
			std::shared_ptr<RawMemory> lock();

	};
```

### Store.h

~~~~cpp
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
			bool operator<(const bref& b2) const;
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
		static auto &getInst();
		/**
		 * @brief      Put K/V pair into map, storing data in a block
		 *
		 * @param[in]  key    The key
		 * @param[in]  value  The value
		 */
		void put(std::string key, std::string value);
		/**
		 * @brief      Determines if KV Store contains key.
		 *
		 * @param[in]  key   The key
		 *
		 * @return     True if contains key, False otherwise.
		 */
		bool containsKey(std::string key);
		/**
		 * @brief      Get data from KV Store
		 *
		 * @param[in]  key   The key
		 *
		 * @return     value associated with key
		 */
		std::string get(std::string key);
		/**
		 * @brief      Delete K/V pair
		 *
		 * @param[in]  key   The key
		 *
		 * @return     the value that was assocaited with key
		 */
		std::string del(std::string key);
		/**
		 * @brief      For each key, exec callback
		 *
		 * @param[in]  cb    Callback Function
		 *
		 * @tparam     F     Function Type [std::function, lambda function, void
		 *                   (fnptr&)(std::string)]
		 */
		template <typename F>
		void forKeys(F cb);
		/**
		 * @brief      Destroys the store, leaving on-disk data intact (recovery
		 *             purposes, will be overwritten next start if unrecovered.
		 */
		~Store();
		
	};
~~~~

#### configParser.h

~~~~cpp
/**
 * map that stores the config info from the config file
 */
static std::unordered_map<std::string,std::string> config;
/**
 * @brief      parse the config file
 *
 * @param[in]  config_path  The path to the config file
 */
void parseConfig(std::string config_path);
~~~~

#### memory_debug.h

~~~~cpp
namespace MEMORY{
		/**
		 * Emulated block-addressed memory
		 */
		static void* mem;
		/**
		 * @brief      Store block in memory
		 *
		 * @param[in]  bno   The block number
		 * @param      rawm  The raw block
		 */
		void set(long long bno, void* rawm);
		/**
		 * @brief      Fetch block from memory
		 *
		 * @param[in]  bno   The block number
		 * @param      rawm  The raw memory to store block to
		 */
		void get(long long bno, void* rawm);
}
~~~~

####memory_production.h

~~~~cpp
namespace MEMORY{
		/**
		 * @brief      C++ Wrapper for Intel libpmemblk
		 */
		class PMEMBlock{
		private:
			PMEMblkpool* pbp;
		public:
			/**
			 * @brief      Creates or loads pmem file from disk
			 *
			 * @param[in]  path       The path to the pmem file
			 * @param[in]  blocksize  The block size
			 * @param[in]  poolSize   The pool size
			 */
			PMEMBlock(const char* path, size_t blocksize, size_t poolSize): pbp(pmemblk_create(path, blocksize,poolSize,0666));
			/**
			 * @brief      Closes the pmem file
			 */
			~PMEMBlock();
			/**
			 * @brief      Reads from pmem file to memory
			 *
			 * @param[in]  bno   The block number
			 * @param      mem   The memory
			 */
			void read(int bno, void* mem);
			/**
			 * @brief      Writes from memory to pmem file
			 *
			 * @param      mem   The memory
			 * @param[in]  bno   The block numer
			 */
			void write(void* mem, int bno);

		};

		/**
		 * C++ smart pointer which allocates the block on first access and deallocates on program end
		 */
		static std::shared_ptr<PMEMBlock> blk(nullptr);

		/**
		 * @brief      Store block in pmem
		 *
		 * @param[in]  bno   The block number
		 * @param      rawm  The raw block
		 */
		void set(long long bno, void* rawm);
		/**
		 * @brief      Fetch block from pmem
		 *
		 * @param[in]  bno   The block number
		 * @param      rawm  The raw memory to store block to
		 */
		void get(long long bno, void* rawm);
	}
~~~~

