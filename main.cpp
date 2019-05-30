#define INF -1
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <csignal>
#include <netinet/in.h>
#include "tbb/task_scheduler_init.h"
#include "tbb/task_group.h"
#include <sstream>
#include <boost/algorithm/string.hpp>
//#define DEBUG
#define S_BLK DEFAULT_BLOCK_SIZE
#include "Block.h"
#include "configParser.h"
#include "Store.h"

typedef BLKCACHE::Store<4096> store;

static volatile sig_atomic_t quit = 0;

class ServerError : public virtual std::runtime_error{
public:
	ServerError() : std::runtime_error::runtime_error("Unknown Server Error") {}
	ServerError(const int no, const std::string &linedesig): std::runtime_error::runtime_error( 
		#ifdef DEBUG
		linedesig + " | " +
		#endif
		"Server Error: " + std::string(std::strerror(no))){
	}
};
class SocketError : public virtual std::runtime_error{
public:
	SocketError() : std::runtime_error::runtime_error("Unknown Socket Error") {}
	SocketError(const int no, const std::string &linedesig): std::runtime_error::runtime_error( 
		#ifdef DEBUG
		linedesig + " | " +
		#endif
		"SocketError Error: " + std::string(std::strerror(no))){
	}
};


inline std::string srecv(int fd){
	char buffer[2048] = {0};
	if(read(fd,buffer,2048) < 0) throw SocketError(errno, std::string(__FILE__) + " - " + std::to_string(__LINE__));
	std::string in(std::to_string('\0'));
	in = buffer;
	return in;
}
static inline void ssend(int fd, std::string output){
	int len = output.size() * sizeof(char);
	if(send(fd, output.c_str(), len, MSG_NOSIGNAL) < 0) throw SocketError(errno, std::string(__FILE__) + " - " + std::to_string(__LINE__));
}
inline void handle_command(int fd){
	static auto &s = store::getInst();
	std::string cmd = std::to_string('\0');
	cmd = srecv(fd);
	std::stringstream cmds(cmd);
	std::string b;
	if(cmds >> b){
		if(boost::iequals(b,"set") || boost::iequals(b,"replace")){
			std::string k,_,e;
			cmds >> k >> _ >> e >> _;
			if(k != "")
			{
				std::string v = srecv(fd);
				if(v != ""){
					(*s).put(k, boost::trim_copy(v));
					ssend(fd, "STORED\r\n");
				}				
			}
		}else if(boost::iequals(b,"get")){
			std::string k;
			cmds >> k;
			if(k != "" && (*s).containsKey(k))
				ssend(fd, (*s).get(k) + "\r\n");
			else
				ssend(fd, "ERROR\r\n");
		}else if(boost::iequals(b,"delete")){
			std::string k;
			cmds >> k;
			if(k != "" && (*s).containsKey(k))
				ssend(fd, (*s).del(k) + "\r\n");
			else
				ssend(fd, "ERROR\r\n");
		}else if(boost::iequals(b,"list")){
			(*s).forKeys([&fd](std::string s){ssend(fd, s + "\r\n");});
		}else{
			ssend(fd, "ERROR\r\n");
		}
	}
}

struct FNEvent{
	int fd;
	bool is_listen;
	FNEvent(int _fd, bool _is_listen): fd(_fd), is_listen(_is_listen){
	}
	int operator()(){
		if(is_listen){
			sockaddr_in remote;
			socklen_t len = sizeof(sockaddr_in);
			int client_fd = accept4(fd, 
				reinterpret_cast<sockaddr*>(&remote),
				&len,
				SOCK_CLOEXEC);
			std::cout << "New Client Connected with descriptor " << client_fd << std::endl;
			if(client_fd <= 0) throw ServerError(errno, std::string(__FILE__) + " - " + std::to_string(__LINE__));
			return client_fd;
		}else{
			if(fd <= 0) throw ServerError(errno, std::string(__FILE__) + " - " + std::to_string(__LINE__));
			struct flock lk;
			lk.l_type = F_RDLCK;
			lk.l_whence = SEEK_CUR;
			lk.l_start = 0;
			lk.l_len = 0;
			if(fcntl(fd, F_SETLKW, &lk) < 0) throw ServerError(errno, std::string(__FILE__) + " - " + std::to_string(__LINE__));
			handle_command(fd);
			lk.l_type = F_UNLCK;
			if(fcntl(fd, F_SETLKW, &lk) < 0) throw ServerError(errno, std::string(__FILE__) + " - " + std::to_string(__LINE__));
			return 0;
		}
	}	
};




inline int init( const int maxEvents, int &listen_fd ){
	int epfd = epoll_create1(EPOLL_CLOEXEC);
	if(epfd < 0) throw ServerError(errno, std::string(__FILE__) + " - " + std::to_string(__LINE__));
	int lsfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (lsfd < 0) throw ServerError(errno, std::string(__FILE__) + " - " + std::to_string(__LINE__));
	int _true = 1;
	if(setsockopt(lsfd, SOL_SOCKET, SO_REUSEADDR, &_true, sizeof(_true)) < 0) throw ServerError(errno, std::string(__FILE__) + " - " + std::to_string(__LINE__));
	const unsigned short port = stoi(config["port"]);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(lsfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(sockaddr_in)) < 0){
    	close(epfd);
    	close(lsfd);
    	throw ServerError(errno, std::string(__FILE__) + " - " + std::to_string(__LINE__));
    } 
    epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = lsfd;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, lsfd, &event)){
    	close(epfd);
    	close(lsfd);
    	throw ServerError(errno, std::string(__FILE__) + " - " + std::to_string(__LINE__));
    }
    if(listen(lsfd, maxEvents) < 0){
    	close(epfd);
    	close(lsfd);
    	throw ServerError(errno, std::string(__FILE__) + " - " + std::to_string(__LINE__));
    }
    listen_fd = lsfd;
    return epfd;
}

tbb::task_group tg;

void terminate(int actype){
	std::cout << "BLKCached is exiting..." << std::endl;
	tg.wait();
	sleep(2);
	quit = 1;
}

int main(){
	signal(SIGTERM, terminate);
	signal(SIGPIPE, SIG_IGN);
	parseConfig("bds.conf");
	std::cout << "BLKCached running on port: " << config["port"] << "..." << std::endl;
	try{
		int listen_fd = -1;
		int poll_fd = init(32, listen_fd);
		if(listen_fd < 0) throw ServerError();
		while(!quit){
			epoll_event evts[32];
			int nevents = epoll_wait(poll_fd, evts, 32, INF);
			if(nevents <= 0){
				close(listen_fd);
				close(poll_fd);
				throw ServerError(errno, std::string(__FILE__) + " - " + std::to_string(__LINE__));
			}
			for(int i = 0; i < nevents; i++){
				int evt_fd = evts[i].data.fd;
				tg.run([&evt_fd, &poll_fd, &listen_fd]{
					int client_fd = FNEvent(evt_fd, evt_fd == listen_fd)();
					if(client_fd > 0){
						epoll_event event;
					    event.events = EPOLLIN | EPOLLET;
					    event.data.fd = client_fd;
					    if(epoll_ctl(poll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) throw ServerError(errno, std::string(__FILE__) + " - " + std::to_string(__LINE__));
					}
				});
			}
		}
	}catch(ServerError &e){
		std::cout << e.what() << std::endl;
		tg.wait();
		sleep(1);
		return 1;
	}catch(SocketError &e){
		std::cout << e.what() << std::endl;
		tg.wait();
		sleep(1);
		return 1;
	}
	tg.wait();
	sleep(1);
	return 0;
}