#define DEBUG

#include <iostream>
#include "Block.h"
#include "configParser.h"
#define S_BLK DEFAULT_BLOCK_SIZE
#include "Store.h"
using namespace std;
typedef BLKCACHE::Store<S_BLK> store;


int main(){
	parseConfig("bds.conf");
	auto &s = store::getInst();
	(*s).put("asdf", "test");
	cout << (*s).get("asdf") << endl;

}