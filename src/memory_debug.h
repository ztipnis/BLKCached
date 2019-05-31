#pragma once
namespace BLKCACHE{
	namespace MEMORY{
		/**
		 * Emulated block-addressed memory
		 */
		static void* mem = malloc(DEFAULT_BLOCK_SIZE);
		/**
		 * @brief      Store block in memory
		 *
		 * @param[in]  bno   The block number
		 * @param      rawm  The raw block
		 */
		void set(long long bno, void* rawm){
			std::memcpy(mem, rawm, DEFAULT_BLOCK_SIZE);
		}
		/**
		 * @brief      Fetch block from memory
		 *
		 * @param[in]  bno   The block number
		 * @param      rawm  The raw memory to store block to
		 */
		void get(long long bno, void* rawm){
			std::memcpy(rawm, mem, DEFAULT_BLOCK_SIZE);
		}
	}
}