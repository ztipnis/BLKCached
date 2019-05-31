#pragma once
#ifndef S_BLK
#error Production build requires that S_BLK be defined before Store.h is included
#endif

#include "configParser.h"
#include <libpmemblk.h>
#include <string>
#define	POOL_SIZE ((off_t)(stoi(config["poolSize"])) << 30)

extern std::unordered_map<std::string,std::string> config;


namespace BLKCACHE{
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
			PMEMBlock(const char* path, size_t blocksize, size_t poolSize): pbp(pmemblk_create(path, blocksize,poolSize,0666)){
				if (pbp == NULL)
	    			pbp = pmemblk_open(path, blocksize);
	    		if (pbp == NULL) {
					perror(path);
					exit(1);
				}
			}
			/**
			 * @brief      Closes the pmem file
			 */
			~PMEMBlock(){
				pmemblk_close(pbp);
			}
			/**
			 * @brief      Reads from pmem file to memory
			 *
			 * @param[in]  bno   The block number
			 * @param      mem   The memory
			 */
			void read(int bno, void* mem){
				if (pmemblk_read(pbp, mem, bno) < 0) {
					perror("pmemblk_read");
					exit(1);
				}
			}
			/**
			 * @brief      Writes from memory to pmem file
			 *
			 * @param      mem   The memory
			 * @param[in]  bno   The block numer
			 */
			void write(void* mem, int bno){
				if (pmemblk_write(pbp, mem, bno) < 0) {
					perror("pmemblk_write");
					exit(1);
				}
			}

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
		void set(long long bno, void* rawm){
			if(!blk) blk = std::make_shared<PMEMBlock>(config["path"].c_str(), S_BLK, POOL_SIZE);
			blk->write(rawm, bno);
		}
		/**
		 * @brief      Fetch block from pmem
		 *
		 * @param[in]  bno   The block number
		 * @param      rawm  The raw memory to store block to
		 */
		void get(long long bno, void* rawm){
			if(!blk) blk = std::make_shared<PMEMBlock>(config["path"].c_str(), S_BLK, POOL_SIZE);
			blk->read(bno, rawm);
		}
	}
}