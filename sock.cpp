#define QUEUESIZE 10
#define INF -1
#include <algorithm>
#include <sys/epoll.h>
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

typedef BLKCACHE::Store<4096> store;

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

struct handleFDTask{
	epoll_event* evts;
	int i;
	int fd;
	int efd;
	handleFDTask(epoll_event* _evts, int _react,int sfd, int pfd): evts(_evts), i(_react), fd(sfd), efd(pfd)
	{}
	void operator()(){
		if(evts[i].data.fd == fd){
			epoll_event event;
			sockaddr_in remote;
			socklen_t len = sizeof(sockaddr_in);
			int client_fd = accept(evts[i].data.fd, reinterpret_cast<sockaddr*> (&remote), &len);
			if(!(client_fd == -1)){
				std::cout << "Client connected with file descriptor #" << client_fd << std::endl;
				event.events = EPOLLIN | EPOLLET;
				event.data.fd = client_fd;
				if(epoll_ctl(efd, EPOLL_CTL_ADD, client_fd, &event) < 0){
					std::cerr << "epoll set insertion error" << std::endl;
					return;
				}
			}else{
				if(errno == EAGAIN || errno == EWOULDBLOCK){
					//socket is closed;
					std::cout << "Would Block" << std::endl;
				}else{
					std::cerr << std::strerror(errno) << std::endl;
				}
			}
		}else{
			if((evts[i].events & EPOLLIN)){
				std::string buffer;
				buffer = sread(evts[i].data.fd);
				if(buffer[0] == EOF || buffer[0] == '\0' || buffer == ""){
					epoll_event event;
					event.events = EPOLLIN | EPOLLET;
					event.data.fd = evts[i].data.fd;
					std::cout << "Client with fd #" << evts[i].data.fd << " has disconnected..." << std::endl;
					epoll_ctl(efd, EPOLL_CTL_DEL, evts[i].data.fd, &event);
					close(evts[i].data.fd);
				}else{
					handle_command(buffer, evts[i].data.fd);
				}
				buffer = std::to_string(char(EOF));
			}
			//client event
		}
	}
};

int main(){
	signal(SIGPIPE, SIG_IGN);
	parseConfig("bds.conf");
	std::cout << "BLKCached running on port: " << config["port"] << "..." << std::endl;
	static int efd = epoll_create(QUEUESIZE);
	if(efd == -1){
		//epoll error
		std::cerr << std::strerror(errno) << std::endl;
		return 1;
	}
	//setup socket file descriptor
	int fd = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
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
	{
	    epoll_event event;
	    event.events = EPOLLIN;
	    event.data.fd = fd;
	    int react = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event);
	    if(react == -1){
	    	//ERROR cannot poll socket
	    	std::cerr << std::strerror(errno) << std::endl;
	    	close(efd);
	    	close(fd);
	    	return 1;
	    }
	}
	//main thread loop
	while(true){
		epoll_event evts[QUEUESIZE];
		int react = epoll_wait(efd, evts, QUEUESIZE, INF);
		if(react == -1){
			//ERROR polling for accept calls
			close(efd);
			close(fd);
			return 1;
		}

		//iterate evts
		std::vector<handleFDTask> v;
		for(int i = 0; i < react; ++i){
			v.push_back(handleFDTask(evts, i, fd, efd));
			tbb::parallel_for(
				tbb::blocked_range<size_t>(0,v.size()),
				[&v](const tbb::blocked_range<size_t>& r){
					for (size_t i=r.begin();i<r.end();++i) v[i]();
				});
		}
	}
}