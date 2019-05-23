#include <string>
#include <iostream>
#include <vector>
#include <time.h>

#ifndef BLKCACHE_BLOCK
#define BLKCACHE_BLOCK

#ifndef DEFAULT_BLOCK_SIZE
#define DEFAULT_BLOCK_SIZE 4096
#endif
namespace BLKCACHE{
	//Block.h
	template <unsigned long> class Store;
	template <unsigned long BLOCK_SIZE>  class Block{
		friend class Store<BLOCK_SIZE>;
		private:
			struct RawMemory{
				void* mem;
				RawMemory(){
					mem = malloc(BLOCK_SIZE);
				}
				~RawMemory(){
					free(mem);
				}
			};
			long long blockno;
			size_t size;
			std::weak_ptr<RawMemory> raw;
			void (&getter)(long long, void*);
			void (&setter)(long long, void*);
			ssize_t free_space;
			std::mutex blkMTX;
		public:
			typedef struct RawMemory RawMemory;
			Block(long long block, void (&_getter)(long long, void*) , void (&_setter)(long long, void*) ): setter(_setter), getter(_getter){
				blockno = block;
				size = BLOCK_SIZE;
				free_space = BLOCK_SIZE;
			}
			std::string get(size_t offset){
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
			ssize_t put(std::string s){
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
			std::string del(size_t offset){
				std::lock_guard<std::mutex> lgb(blkMTX);
				auto start = lock();
				auto start2 = std::make_shared<RawMemory>();
				std::memcpy(start2->mem, start->mem, offset);
				size_t sndOffset = offset;
				while(sndOffset < (size) && *(((char*)start->mem) + sndOffset) != '\0' && *(((char*)start->mem) + sndOffset) != char(30))
					sndOffset += sizeof(char);
				std::memcpy(((char*) start2->mem) + offset, ((char*) start->mem) + sndOffset, BLOCK_SIZE - sndOffset);
				free_space += sndOffset - offset + 1;
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
			void print_stack() const{
				auto start = lock();
				std::cout << ((char*) start->mem) << std::endl;
			}
			void write(std::shared_ptr<RawMemory> lock_ptr){
				if(!lock_ptr) return;
				static std::mutex fileMTX;
				std::lock_guard<std::mutex> lgf(fileMTX);
				setter(blockno, lock_ptr->mem);
			}
			std::shared_ptr<RawMemory> lock(){
				static std::mutex fileMTX;
				std::lock_guard<std::mutex> lgf(fileMTX);
				if(auto ptr = raw.lock()){return ptr;}
				auto ptr = std::make_shared<RawMemory>();
				getter(blockno, ptr->mem);
				raw = ptr;
				return ptr;
			}

	};
}

#endif