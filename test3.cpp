#include <iostream>
#include "block.hpp"


uint64_t hash(const std::string& key){
	return key.size();
}


int main(){
	BlockMgr<250, hash>  bm(1000, 0.5f);
	bm.put("1", "a");
	bm.put("2", "b");
	bm.put("3", "c");
	bm.put("4", "d");
	bm.del("3");
	bm.collect();
	bm.put("5", "e");
	std::cout << bm.get("1") << std::endl;
	std::cout << bm.get("2") << std::endl;
	std::cout << bm.get("3") << std::endl;
	std::cout << bm.get("4") << std::endl;
	std::cout << bm.get("5") << std::endl;

}