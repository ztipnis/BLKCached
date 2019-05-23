#define DEBUG

#include <iostream>
#include "Block.h"
#include "configParser.h"
#define S_BLK DEFAULT_BLOCK_SIZE
#include "Store.h"
using namespace std;
typedef BLKCACHE::Store<S_BLK> store;
#include <uv.h>

inline bool isInteger(const std::string & s)
{
   if(s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false;

   char * p;
   strtol(s.c_str(), &p, 10);

   return (*p == 0);
}

int main(){
	bool isFile = !isInteger(config["socket"]);
	io_service srv;
	
	
}