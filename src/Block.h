//Block.h
#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <time.h>
#include <mutex>

#ifndef BLKCACHE_BLOCK
#define BLKCACHE_BLOCK

#ifndef DEFAULT_BLOCK_SIZE
#define DEFAULT_BLOCK_SIZE 4096
#endif
namespace BLKCACHE{
	template <unsigned long> class Store;
	/**
	 * @brief      Block Class: Stores metatadata to access block, and contains
	 *             a manager that is in charge of allocating DRAM space to fetch
	 *             the block from disk, and for deallocting DRAM space after
	 *             writing changes to disk
	 *
	 * @tparam     BLOCK_SIZE  Length of blocks to allocate (bytes)
	 * 
	 */
	template <unsigned long BLOCK_SIZE>  class Block{
	/**
	 * @brief      Store Class
	 */
		private:
			struct RawMemory;
			long long blockno; //number of the block within the segments (used by PMDK)
			size_t size; //total size of the objects represented by the block
			std::weak_ptr<RawMemory> raw; //pointer to the raw memory object that is stored by the block. Auto handles constructors and desctructors
			void (&getter)(long long, void*); //pointer to function to fetch the data from the disk
			void (&setter)(long long, void*); //pointer to function to store the data to the disk
			ssize_t free_space; //total free space remaining in the block
			std::mutex blkMTX; //mutex - ensures concurrent write requests don't currupt the data
			~Block(){};
		public:
			friend class Store<BLOCK_SIZE>;
			typedef struct RawMemory RawMemory;
			/**
			 * @brief      Creates a block
			 *
			 * @param[in]  block    The block number
			 * @param[in]  _getter  The getter function to fetch data from disk
			 * @param[in]  _setter  The setter function to store data on disk
			 */
			Block(long long block, void (&_getter)(long long, void*) , void (&_setter)(long long, void*) ): setter(_setter), getter(_getter), blockno(block), size(BLOCK_SIZE), free_space(BLOCK_SIZE){}
			/**
			 * @brief      get string which starts at specified offset
			 *
			 * @param[in]  offset  The offset at which the string starts
			 *
			 * @return    string stored at offset
			 */
			std::string get(size_t offset);
			/**
			 * @brief      put a string in the block
			 *
			 * @param[in]  s     string to store
			 *
			 * @return     the offset at which the string is stored
			 */
			ssize_t put(std::string s);
			/**
			 * @brief      delete data at offset
			 *
			 * @param[in]  offset  The offset
			 *
			 * @return     the string that was stored at offset
			 */
			std::string del(size_t offset);
			/**
			 * @brief      Gets the free space.
			 *
			 * @return     The free space.
			 */
			size_t getFreeSpace() const{
				return free_space;
			}
			bool operator<(const Block<BLOCK_SIZE>& b) const{
				return (free_space < b.free_space);
			}
			bool operator<(const size_t b) const{
				return (free_space < b);
			}
			bool operator>(const Block<BLOCK_SIZE>& b) const{
				return (free_space > b.free_space);
			}
			bool operator>=(const Block<BLOCK_SIZE>& b) const{
				return !(free_space >= b.free_space);
			}
			bool operator<=(const Block<BLOCK_SIZE>& b) const{
				return !(free_space <= b.free_space);
			}
		private:
			/**
			 * @brief      debug func prints the block data to the screen
			 */
			void print_stack() const{
				auto start = lock();
				std::cout << ((char*) start->mem) << std::endl;
			}
			/**
			 * @brief      write the data to the disk
			 *
			 * @param[in]  lock_ptr  lock on the raw memory stored
			 */
			void write(std::shared_ptr<RawMemory> lock_ptr);
			/**
			 * @brief      keep the raw memory alive for operations
			 *
			 * @return     return strong pointer to the raw memory handled by the block.
			 */
			std::shared_ptr<RawMemory> lock();

	};

	/**
	 * @brief      Represents raw memory, in the form of a void pointer, as a C++ object
	 *
	 * @tparam     BLOCK_SIZE  The size of the block stored in memory
	 */
	template<unsigned long BLOCK_SIZE> 
	struct Block<BLOCK_SIZE>::RawMemory{
		void* mem;
		RawMemory();
		virtual ~RawMemory();
	};
	/**
	 * @brief      Initialise Raw Memory;
	 *
	 * @tparam     BLOCK_SIZE  Length of memory block to allocate
	 */
	template<unsigned long BLOCK_SIZE> Block<BLOCK_SIZE>::RawMemory::RawMemory(){
		mem = malloc(BLOCK_SIZE);
	}
	/**
	 * @brief      Deallocates the raw memory
	 *
	 * @tparam     BLOCK_SIZE  length of memory block allocated
	 */
	template<unsigned long BLOCK_SIZE> Block<BLOCK_SIZE>::RawMemory::~RawMemory(){
		free(mem);
	}

	template<unsigned long BLOCK_SIZE>
	std::string Block<BLOCK_SIZE>::get(size_t offset){
		std::lock_guard<std::mutex> lg(blkMTX);
		auto start = lock();
		size_t sndOffset = offset;
		while(sndOffset < (size) && *(((char*)start->mem) + sndOffset) != '\0' && *(((char*)start->mem) + sndOffset) != char(30))
			sndOffset += sizeof(char);
		size_t chrln = (sndOffset - offset) / sizeof(char);
		char* c = new char[chrln + 1];
		std::memcpy(c, (((char*)start->mem) + offset), sndOffset - offset);
		c[chrln] = '\0';
		std::string s(c);
		delete[] c;
		return s;
	}
	template<unsigned long BLOCK_SIZE>
	ssize_t Block<BLOCK_SIZE>::put(std::string s){
		std::lock_guard<std::mutex> lgb(blkMTX);
		auto start = lock();
		size_t i = 0;
		if((s.length() + 1)*sizeof(char) > free_space) return -1; //wont fit
		while(i < size && *(((char*)start->mem) + i) != '\0') i+= sizeof(char);
		size_t st_offset = i;
		for(int j = 0; j < s.length(); j++){
			*(((char*)start->mem) + i) = s.c_str()[j];
			i += sizeof(char);
		}
		if(i < size - 1){
			*(((char*)start->mem) + i) = char(30);
		}
		free_space -= (s.length()+1) * sizeof(char);
		write(start);
		raw = start;
		return st_offset;
	}
	template<unsigned long BLOCK_SIZE>
	std::string Block<BLOCK_SIZE>::del(size_t offset){
		std::lock_guard<std::mutex> lgb(blkMTX);
		auto start = lock();
		auto start2 = std::make_shared<RawMemory>();
		std::memcpy(start2->mem, start->mem, offset);
		size_t sndOffset = offset;
		while(sndOffset < (size) && *(((char*)start->mem) + sndOffset) != '\0' && *(((char*)start->mem) + sndOffset) != char(30))
			sndOffset += sizeof(char);
		std::memcpy(((char*) start2->mem) + offset, ((char*) start->mem) + sndOffset, BLOCK_SIZE - sndOffset);
		//free_space += sndOffset - offset + 1;
		write(start2);
		raw = start2;
		size_t chrln = (sndOffset - offset) / sizeof(char);
		char* c = new char[chrln + 1];
		std::memcpy(c, (((char*)start->mem) + offset), sndOffset - offset);
		c[chrln] = '\0';
		std::string s(c);
		delete[] c;
		return s;
	}
	template<unsigned long BLOCK_SIZE>
	void Block<BLOCK_SIZE>::write(std::shared_ptr<RawMemory> lock_ptr){
		if(!lock_ptr) return;
		static std::mutex fileMTX;
		std::lock_guard<std::mutex> lgf(fileMTX);
		setter(blockno, lock_ptr->mem);
	}


	template<unsigned long BLOCK_SIZE>
	std::shared_ptr<typename Block<BLOCK_SIZE>::RawMemory > Block<BLOCK_SIZE>::lock(){
		static std::mutex fileMTX;
		std::lock_guard<std::mutex> lgf(fileMTX);
		if(auto ptr = raw.lock()){return ptr;}
		auto ptr = std::make_shared<RawMemory>();
		getter(blockno, ptr->mem);
		raw = ptr;
		return ptr;
	}
}
#endif