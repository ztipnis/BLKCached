# BLKCached

[![Build Status](https://travis-ci.com/ztipnis/BLKCached.svg?token=4hGLqJR7tjfwfUvVUMxd&branch=master)](https://travis-ci.com/ztipnis/BLKCached)

## Intro
BLKCached was created in response to _IBM Research_'s Paper [_"Reaping the performance of fast NVM storage
with uDepot"_](https://www.usenix.org/system/files/fast19-kourtis.pdf). The goal of this project was to, like _uDepot_, create a drop-in replacement for _memcached_, but to simplify the API, and to tune for performance. Unlike _uDepot_ the goal of BLKCached is to provide a proof-of-concept for the use of block-addressable DRAM in the in-memory database sector. The hope is that, in time, block-addressable DRAM (accessible via either PCIe, DIMM or other northbridge ports) will provide more affordable, reliable and fast storage solutions. 

## Installation
~~~bash
git clone http://github.com/ztipnis/BLKCached
cd BLKCached
mkdir build
cd build
cmake ..
make
~~~
## Usage
> Usage: BLKCached [options]  
> 
> General:  
>    -v [ –version ] Print version string  
>    -h [ –help ] Print command line help  
> 
> Configuration:  
>    -s [ –poolSize ] Size of Block Pool Stored on Disk (Defines maximum data stored to disk)  
>    -f [ –path ] Path to pool file  
>    -j [ –threads ] Maximum number of concurrent threads handling socket i/o   
>    -i [ –ip ] IP Address on which to listen  
>    -p [ –port ] Port on which to listen  
> 
> Command Line:  
>    -c [ –cfg ] Path to config file  

## Table of Contents  
   * [BLKCached](#blkcached)
      * [Intro](#intro)
      * [Installation](#installation)
      * [Usage](#usage)
      * [Table of Contents](#table-of-contents)
      * [Files](#files)
         * [Block.h](#blockh)
            * [Class Block](#class-block)
            * [Struct RawMemory](#struct-rawmemory)
         * [Store.h](#storeh)
            * [Class Store](#class-store)
            * [Struct ref](#struct-ref)
            * [Struct bref](#struct-bref)
         * [configParser.h](#configparserh)
         * [memory_*.h](#memory_h)
         * [bds.conf](#bdsconf)

## Files
### Block.h

Class for managing in-memory blocks of data (as well as for fetching those blocks from some block addressable storage medium)

#### Class Block

| Function    | Args                                 | Effect                                                       | Return                     |
| ----------- | ------------------------------------ | ------------------------------------------------------------ | -------------------------- |
| getInst     | None                                 | Calls default contructor on first call, after that returns atomic pointer to shared instance | ```std::atomic<Store*>&``` |
| get         | `std::string key`                    | Fetches reference to block/offset stored for given key, then dereferences to find value | `std::string`              |
| put         | `std::string key, std::string value` | Inserts value into the block with the lowest free space that is able to accomodate the string, then associates the key with the ref to that value | N/A                        |
| del         | `std::string key`                    | Fetches reference to block/offset for key, deletes te value from the block, and returns the old value | `std::string`              |
| containsKey | std::string key                      | Checks if store contains a certain key                       | `bool`                     |
| forKeys     | `<typename F> cb`                    | Calls the function pointer (or lambda function, etc.) cb, for each key. | N/A                        |

#### Struct RawMemory

Bridges malloc (allocate raw memory of certain size) to C++'s internal 

### Store.h

Singleton class which serves as a coordinator for blocks, and a storage for key/value ref pairs.

#### Class Store

| Function     | Args                                                         | Effect                                                       | Return        |
| ------------ | ------------------------------------------------------------ | ------------------------------------------------------------ | ------------- |
| Constructor  | `long long block, void (&_getter)(long long, void*)`,  <br/> `void (&_setter)(long long, void*)` | Creates a new Block Object at Block offset # (block), with function (\_getter) to fetch data from the disk, and (\_setter) to save to disk | N/A           |
| get          | `size_t offset`                                              | fetches null-terminated or RS- terminated char array starting at (offset) | `std::string` |
| put          | `std::string s`                                              | Inserts string (s) into block (if fits), returns offset of new char array | `ssize_t`     |
| del          | `size_t offset`                                              | Deletes string at (offset) and returns its old value         | `std::string` |
| getFreeSpace | None                                                         | Returns remaining space in block                             | `size_t`      |

#### Struct ref

Reference to block/offset. Contains a few helpful functions for dereferencing etc.

#### Struct bref

Shim for pointer to block, with operators for comparing blocks based on free space.

### configParser.h

Defines static unordered map with k/v pairs for config options and values.

### memory_*.h

Debug (memory-only), and Production (Persistent Memory [aka. on disk]) getters and setters used to emulate block-addressable mediums.

### bds.conf

Options used to configure k/v store and server daemon

| Option   | Values         | Desc.                                                        |
| -------- | -------------- | ------------------------------------------------------------ |
| poolSize | (int) 1+       | Size of pool (pre-allocated) in GB                           |
| path     | (string) [any] | Path to persistent memory store. Can be relative or absolute. |
| threads  | (int) 0+       | 0 => let system choose # of threads. 1+ => specific max number of threads (not yet implemented) |
| ip       | (ipaddr) [any] | The ip on which to listen for connections (not yet implemented) |
| port     | (int)[any]     | The port on which to listen for connections                  |



## Documentation

Full documentation is available on [GitHub Pages](//ztipnis.github.io/BLKCached)
