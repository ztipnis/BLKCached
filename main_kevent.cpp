#define QUEUESIZE 10
#define INF -1
#include <algorithm>
#include <sys/event.h>
#include <sys/socket.h>
#include <iostream>
#include <cerrno>
#include <cstring>
#include <clocale>
#include <netinet/in.h>
#include <unistd.h>
#include <sstream>
#include "tbb/parallel_for.h"
#include "tbb/task_scheduler_init.h"
#include <boost/algorithm/string.hpp>
#include <csignal>
#define DEBUG
#define S_BLK DEFAULT_BLOCK_SIZE
#include "Block.h"
#include "configParser.h"
#include "Store.h"

#ifndef MSG_NOSIGNAL
#define NOSIG_UNDEF
#define MSG_NOSIGNAL 0
#endif

#ifndef SOCK_NONBLOCK
#define SOCK_NONBLOCK 0
#endif

typedef BLKCACHE::Store<4096> store;
static int kq;
struct kevent evList[QUEUESIZE];
struct kevent evSet;

//shim for c++ calls to syscall "send"
inline static void swrite(int fd, std::string output){
	int len = output.size() * sizeof(char);
	//len = (len < sizeof(output.c_str())) ? sizeof(output.c_str()) : len;
	send(fd, output.c_str(), len, MSG_NOSIGNAL);
}
//shim for c++ calls to "read"
inline static std::string sread(int fd){
	char buffer[2<<20] = {0};
	read(fd, buffer, 2<<20);
	std::string in(std::to_string('\0'));
	in = buffer;
	return in;
}

/**
 * @brief      Handle commands passed to the program
 *
 * @param[in]  cmd   The command
 * @param[in]  fd    the socket file descriptor requesting the command be executed
 */
inline void handle_command(std::string cmd, int fd){
	static auto &s = store::getInst();
	std::stringstream cmds(cmd);
	std::string b;
	if(cmds >> b){

		if(boost::iequals(b,"set") || boost::iequals(b,"replace")){
			std::string k,_,e;
			cmds >> k >> _ >> e >> _;

			if(k != "")
			{
				std::string v = sread(fd);
				if(v != ""){
					(*s).put(k, v);
					swrite(fd, "STORED\r\n");
				}
				

			}
		}else if(boost::iequals(b,"get")){
			std::string k;
			cmds >> k;
			if(k != "" && (*s).containsKey(k))
				swrite(fd, (*s).get(k) + "\r\n");
		}else if(boost::iequals(b,"delete")){
			std::string k;
			cmds >> k;
			if(k != "" && (*s).containsKey(k))
				swrite(fd, (*s).del(k) + "\r\n");
		}else{
			std::cout << std::to_string(int(char(cmd[0]))) << std::endl;
			std::cout << "UNRECOGNIZED COMMAND" << std::endl;
		}
	}
}

/**
 * @brief      Functor executed on separate threads in thread pool: handles commands, and allows for some level of concurrency
 */
struct handleFDTask{
	struct kevent* evts;
	int i;
	int fd;
	/**
	 * @brief      Init a struct that contains data required for functor to run
	 *
	 * @param      _evts   The triggered epoll events
	 * @param[in]  _react  The number of epoll events triggered
	 * @param[in]  sfd     The server file descriptor
	 * @param[in]  pfd     The polling file descriptor
	 */
	handleFDTask(struct kevent* _evts, int _react,int sfd): evts(_evts), i(_react), fd(sfd)
	{}
	/**
	 * Actual function call called for each file descriptor for which epoll was triggered
	 */
	void operator()(){
		if(evts[i].ident == fd){

			sockaddr_in remote;
			socklen_t len = sizeof(sockaddr_in);
			int client_fd = accept(evList[i].ident, reinterpret_cast<sockaddr*> (&remote), &len);
			if(!(client_fd == -1)){
				std::cout << "Client connected with file descriptor #" << client_fd << std::endl;
				EV_SET(&evSet, client_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                if (kevent(kq, &evSet, 1, NULL, 0, NULL) == -1)
                    return;
			}else{
				if(errno == EAGAIN || errno == EWOULDBLOCK){
					//socket is closed;
					std::cout << "Would Block" << std::endl;
				}else{
					std::cerr << std::strerror(errno) << std::endl;
				}
			}
		}else{
			std::string buffer;
			buffer = sread(evList[i].ident);
			if(evList[i].flags & EV_EOF){
				std::cout << "Client with fd #" << evList[i].ident << " has disconnected..." << std::endl;
				EV_SET(&evSet, evList[i].ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
				close(evList[i].ident);
			}else{
				handle_command(buffer, evList[i].ident);
			}
			buffer = std::to_string(char(EOF));
			//client event
		}
	}
};

/**
 * @brief      Main Function: sets up listen socket and kqueue, 
 *             then repeatedly waits for incoming event on any of the
 *             polled socket file descriptors. When a new connection
 *             triggers kqueue, the new client file descriptor is
 *             added to polling; when any other file descriptor is
 *             triggered, the incoming data is parsed and the apropriate
 *             action is taken.
 *
 * @return     0 if no error occurred, 1 otherwise. (Ideally should 
 *             never return, except if it recieves SIGABRT, SIGTERM,
 *             SIGKILL, SIGINT, or SIGHUP)
 */
static volatile sig_atomic_t quit = 0;
void terminate(int actype){
	std::cout << "BLKCached is exiting..." << std::endl;
	quit = 1;
}

int main(){
	signal(SIGTERM, terminate);
	signal(SIGPIPE, SIG_IGN);
	parseConfig("bds.conf");
	std::cout << "BLKCached running on port: " << config["port"] << "..." << std::endl;

	kq = kqueue();

	//setup socket file descriptor
	int fd = socket (AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        std::cerr << std::strerror(errno) << "\n";
        return 1;
    }

    //listen
    int tr = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(tr)) == -1){
    	//ERROR set socket reuse address
    	std::cerr << std::strerror(errno) << std::endl;
    	return 1;
    }
    #ifdef NOSIG_UNDEF
    int set = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int)) == -1){
    	//ERROR set nosigpipe address
    	std::cerr << std::strerror(errno) << std::endl;
    	return 1;
    }
    #endif

    const unsigned short port = stoi(config["port"]);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    //bind socket
    if(bind(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(sockaddr_in)) == -1){
    	//ERROR with bind
    	std::cerr << std::strerror(errno) << std::endl;
    	return 1;
    }

    //listen on socket for connections requests
    if(listen(fd, 5) == -1){
    	//ERROR listen
    	std::cerr << std::strerror(errno) << std::endl;
    	return 1;
    }
    EV_SET(&evSet, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
	if (kevent(kq, &evSet, 1, NULL, 0, NULL) == -1){
		//epoll error
		std::cerr << std::strerror(errno) << std::endl;
		return 1;
	}
	//main thread loop
	while(!quit){
		int nev, i;
	    struct sockaddr_storage addr;
	    socklen_t socklen = sizeof(addr);
	    nev = kevent(kq, NULL, 0, evList, 32, NULL);
		if(nev < 1){
			//ERROR polling for accept calls
			close(fd);
			return 1;
		}
		//iterate evts
		std::vector<handleFDTask> v;
		for(int i = 0; i < nev; ++i){
			v.push_back(handleFDTask(evList, i, fd));
			tbb::parallel_for(
				tbb::blocked_range<size_t>(0,v.size()),
				[&v](const tbb::blocked_range<size_t>& r){
					for (size_t i=r.begin();i<r.end();++i) v[i]();
				});
		}
	}
	return 0;
}