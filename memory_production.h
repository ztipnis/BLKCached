#ifndef S_BLK
#error Production build requires that S_BLK be defined before Store.h is included
#endif

#include "configParser.h"
#include <libpmemblk.h>
#define	POOL_SIZE ((off_t)(config["poolSize"] << 30))

extern std::unordered_map<std::string,std::string> config;


namespace BLKCACHE{
	namespace MEMORY{
		PMEMblkpool *pbp;
		void set(long long bno, void* rawm){
			if(!*pbp){
				pbp = pmemblk_create(config["path"], S_BLK,POOL_SIZE , 0666);
				if (pbp == NULL)
	    			pbp = pmemblk_open(config["path"], S_BLK);
	    		if (pbp == NULL) {
					perror(path);
					exit(1);
				}
			}
			if (pmemblk_write(pbp, rawm, bno) < 0) {
				perror("pmemblk_write");
				exit(1);
			}
			pmemblk_close(pbp);
			//std::memcpy(mem, rawm, DEFAULT_BLOCK_SIZE);
		}
		void get(long long bno, void* rawm){
			if(!*pbp){
				pbp = pmemblk_create(config["path"], S_BLK, POOL_SIZE, 0666);
				if (pbp == NULL)
	    			pbp = pmemblk_open(config["path"], S_BLK);
	    		if (pbp == NULL) {
					perror(path);
					exit(1);
				}
			}
			if (pmemblk_read(pbp, rawm, bno) < 0) {
				perror("pmemblk_read");
				exit(1);
			}
			pmemblk_close(pbp);
			//std::memcpy(rawm, mem, DEFAULT_BLOCK_SIZE);
		}
	}
}