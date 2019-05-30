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
		class PMEMBlock{
		private:
			PMEMblkpool* pbp;
		public:
			PMEMBlock(const char* path, size_t blocksize, size_t poolSize): pbp(pmemblk_create(path, blocksize,poolSize,0666)){
				if (pbp == NULL)
	    			pbp = pmemblk_open(path, blocksize);
	    		if (pbp == NULL) {
					perror(path);
					exit(1);
				}
			}
			~PMEMBlock(){
				pmemblk_close(pbp);
			}
			void read(int bno, void* mem){
				if (pmemblk_read(pbp, mem, bno) < 0) {
					perror("pmemblk_read");
					exit(1);
				}
			}
			void write(void* mem, int bno){
				if (pmemblk_write(pbp, mem, bno) < 0) {
					perror("pmemblk_write");
					exit(1);
				}
			}

		};


		static std::shared_ptr<PMEMBlock> blk(nullptr);
		void set(long long bno, void* rawm){
			if(!blk) blk = std::make_shared<PMEMBlock>(config["path"].c_str(), S_BLK, POOL_SIZE);
			blk->write(rawm, bno);
		}
		void get(long long bno, void* rawm){
			if(!blk) blk = std::make_shared<PMEMBlock>(config["path"].c_str(), S_BLK, POOL_SIZE);
			blk->read(bno, rawm);
		}
	}
}