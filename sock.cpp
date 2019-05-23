#define QUEUESIZE = 10
#define INF = -1
#include "configParser.h"
#include <sys/epoll.h>
#include <sys/socket.h>

int main(){
	static int efd = epoll_create(QUEUESIZE);
	if(epollfd == -1){
		//epoll error
		std::cerr << strerr(errno) << std::endl;
		return 1;
	}
	//setup socket file descriptor
	int fd = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd == -1) {
        std::cerr << strerror (errno) << "\n";
        return 1;
    }

    //listen
    int tr = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1){
    	//ERROR set socket reuse address
    	std::cerr << strerr(errno) << std::endl;
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
    	std::cerr << strerror(errno) << std::endl;
    	return 1;
    }

    //listen on socket for connections requests
    if(listen(fd, 5) == -1){
    	//ERROR listen
    	std::cerr << strerror(errno) << std::endl;
    	return 1;
    }
	{
	    epoll_event event;
	    event.events = EPOLLIN;
	    event.data.fd = fd;
	    int react = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, $event);
	    if(react == -1){
	    	//ERROR cannot poll socket
	    	std::cerr << strerr(errno) << std::endl;
	    	close(epollfd);
	    	close(fd);
	    	return 1;
	    }
	}
	//main thread loop
	while(true){
		epoll_event evts[QUEUESIZE];
		int react = epoll_wait(epollfd, events, QUEUESIZE, INF);
		if(react == -1){
			//ERROR polling for accept calls
			close(epollfd);
			close(fd);
			return 1;
		}

		//iterate events
		for(int i = 0; i < react; ++i){
			if(events[i].data.fd == fd){
				sockaddr_in remote;
				socklen_t len = sizeof(sockaddr_in);
				int client_fd = accept(events[i].data.fd, reinterpret_cast<sockaddr*> (&remote), &len);
				if(client_fd == -1){
					if(errno == EAGAIN || error == EWOULDBLOCK){
						//socket is closed;
						continue;
					}else{
						std::cerr << strerror(errno) << std::endl;
					}
				}else{
					//new connection
					
					std::cout << "Connection initiated";
				}
			}
		}
	}
}