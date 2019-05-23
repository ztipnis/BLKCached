
namespace BLKCACHE{
	namespace MEMORY{
		static void* mem = malloc(DEFAULT_BLOCK_SIZE);

		void set(long long bno, void* rawm){
			std::memcpy(mem, rawm, DEFAULT_BLOCK_SIZE);
		}
		void get(long long bno, void* rawm){
			std::memcpy(rawm, mem, DEFAULT_BLOCK_SIZE);
		}
	}
}