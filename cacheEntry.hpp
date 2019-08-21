#ifndef __cacheEntry__
#define __cacheEntry__
#define CKMAGIC 0xCAC8EBE9

	#ifndef uint
		typedef unsigned int uint;
	#endif

	typedef struct {
		uint magic;
		int offset; //should be negative
		uint keyLen; //n bytes
		uint valLen; //n bytes
		unsigned short hitct;

	} cacheEntry;

	std::ostream& operator<<(std::ostream& os, cacheEntry entry){
		return os << "{" << std::endl
		<< '\t' << entry.magic << std::endl
		<< '\t' << entry.offset << std::endl
		<< '\t' << entry.keyLen << std::endl
		<< '\t' << entry.valLen << std::endl
		<< '\t' << entry.hitct << std::endl
		<< "}" << std::endl;
	}
	bool operator==(const cacheEntry& c1, const cacheEntry& c2){
		return
			c1.magic == c2.magic &&
			c1.offset == c2.offset &&
			c1.keyLen == c2.keyLen &&
			c1.valLen == c2.valLen &&
			c1.hitct == c2.hitct;
	}
	inline bool operator!=(const cacheEntry& c1, const cacheEntry& c2){
		return !(c1==c2);
	}
	bool operator < (const cacheEntry& c1, const cacheEntry& c2)
    {
        return (c1.offset < c2.offset);
    }
#endif