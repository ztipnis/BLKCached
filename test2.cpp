#include "BlockMgr.hpp"

uint64_t hash(const std::string& key){
	return key.size();
}

int main(){
	BlockMgr<50, hash>  bm(200, 0.5f);
	bm.put("test", "value");
	std::cout << bm.get("test") << std::endl;
	bm.put("test2", "volumen");
	std::cout << "put" << std::endl;
	std::cout << bm.get("test2") << std::endl;
	//std::cout << bm.get("test2") << std::endl;
}