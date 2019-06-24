template <int BLOCK_SIZE, int MSize>
class BLKCache{
private:
	static uint64_t pack(uint64_t hash, off64_t address, bool ondisk){
		if(address > uint64_t(0x7FFFFFFFFFFF)) throw std::runtime_error("Address Out Of Bounds");
		uint64_t addr = static_cast<uint64_t>(address);
		uint64_t od = 0x8000000000000000 * uint64_t(ondisk ? 1 : 0);
		return (od | (hash << 47) | address);
	}
	static void unpack(uint64_t packed, uint16_t& keyHash, off64_t& address, bool& ondisk){
		keyHash = static_cast<uint16_t>((packed << 1) >> 48); // overflow last bit, then bitshift
		ondisk = static_cast<bool>(packed >> 63);
		uint64_t bitmask = uint64_t(0x7FFFFFFFFFFF);
		address = (packed & bitmask);
	}
	uint64_t (*hashfunc)(std::string);
	off64_t fs_offset
public:
	BLKCache(uint64_t (*hf)(std::string), off64_t start_offset): hashfunc(hf), fs_offset(start_offset){
		
	}
	~BLKCache();
};