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
#include <boost/filesystem.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/program_options.hpp>
#include <iostream>
//#define DEBUG
#define S_BLK DEFAULT_BLOCK_SIZE
#include "Block.h"
#include "configParser.h"
#include "Store.h"
#define VERSION "0.0.1"

//typedef BLKCACHE::Store<4096> store;
/**
 * Has kill signal been caught? Atomic and volatile because set by signal not on
 * main thread
 */
static volatile sig_atomic_t quit = 0;

/**
 * @brief      Error Encountered When Setting up Server or Socket
 */

template <class T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
    copy(v.begin(), v.end(), std::ostream_iterator<T>(std::cout, " "));
    return os;
}

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
/**
 * @brief      Error Encountered When Communicating With Socket
 */
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

/**
 * @brief      Shim used bridge C syscall "read" and C++ strings
 *
 * @param[in]  fd    File Descriptor from which to rea
 *
 * @return     The string value read from the File Descriptor
 */
inline std::string srecv(int fd){
	char buffer[2048] = {0};
	if(read(fd,buffer,2048) < 0) throw SocketError(errno, std::string(__FILE__) + " - " + std::to_string(__LINE__));
	std::string in(std::to_string('\0'));
	in = buffer;
	return in;
}
/**
 * @brief      Shim used to bridge C syscall "send" and C++ strings
 *
 * @param[in]  fd      Socket File Descriptor on which to write
 * @param[in]  output  String to write to fd
 */
static inline void ssend(int fd, std::string output){
	int len = output.size() * sizeof(char);
	if(send(fd, output.c_str(), len, MSG_NOSIGNAL) < 0) throw SocketError(errno, std::string(__FILE__) + " - " + std::to_string(__LINE__));
}
/**
 * @brief      Read from File Descriptor and Handle Input
 *
 * @param[in]  fd    Client Socket File Descriptor
 */
inline void handle_command(int fd){
	static auto &s = BLKCACHE::Store<4096>::getInst();
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
/**
 * @brief      Functor called on when event happens on Socket (Called off of the
 *             main thread – in this case on a thread in a thread pool allocated
 *             by Thread Building Blocks Library)
 */
struct FNEvent{
	//Client File Descriptor for Event
	int fd;
	//Is docket listening for new connections?
	bool is_listen;
	/**
	 * @brief      Constructor for new Event Call
	 *
	 * @param[in]  _fd         File Descriptor
	 * @param[in]  _is_listen  Indicates if Socket is Listening for Incoming
	 *                         Connections
	 */
	FNEvent(int _fd, bool _is_listen): fd(_fd), is_listen(_is_listen){
	}
	/**
	 * Call operator - accepts new client if FD is listening for new
	 * connections, otherwise locks the file descriptor for reading & writing
	 * then handles input on the descriptor
	 */
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
			lk.l_type = F_RDLCK | F_WRLCK;
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



/**
 * @brief      Setup Listening Socket and Polling
 *
 * @param[in]  maxEvents  The maximum simultaneous events to catch when pollisng
 * @param      listen_fd  The Socket File Descriptor which is Listening for New
 *                        Connections
 *
 * @return     Polling File Descriptor
 */
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

/**
 * Thread Building Blocks Thread PooL (AKA Task Group)
 */
tbb::task_group tg;

/**
 * @brief      Kill Signal Caught - Cleanup and Quit
 *
 * @param[in]  actype  The Signal Type
 */
void terminate(int actype){
	std::cout << "BLKCached is exiting..." << std::endl;
	tg.wait();
	sleep(2);
	quit = 1;
}

/**
 * @brief      Main Run loop:
 *             1. Set up socket and polling
 *             2. Listen for connections
 *             3. Repeatedly Poll for events on sockets controlled by the
 *                process
 *                3.1 Handle Socket Event off of the Main Thread
 *                	3.1.a If Event was new connection, add connection to polling
 *                	3.1.b if Event was not new connection, handle action
 *             4. On Exit from run loop, finish tasks, close sockets, and exit
 *
 * @return     Zero if No Errors, Non-Zero On Error
 */
int main(int argc, char* argv[]){
	signal(SIGTERM, terminate);
	signal(SIGPIPE, SIG_IGN);
	boost::filesystem::path loc = boost::dll::program_location();

	boost::program_options::options_description opts("General");
	opts.add_options()
		("version,v", "Print version string")
		("help,h", "Print command line help");
	boost::program_options::options_description cfg("Configuration");
	cfg.add_options()
		("poolSize,s", "Size of Block Pool Stored on Disk (Defines maximum data stored to disk)")
		("path,f", "Path to pool file")
		("threads,j", "Maximum number of concurrent threads handling socket i/o")
		("ip,i", "IP Address on which to listen")
		("port,p", "Port on which to listen");
	boost::program_options::options_description cmdln("Command Line");
	cmdln.add_options()
		("cfg,c","Path to config file");

	boost::program_options::options_description clopts;
	clopts.add(opts).add(cfg).add(cmdln);
	boost::program_options::variables_map vm;
	store(boost::program_options::command_line_parser(argc,argv).options(clopts).run(),vm);
	notify(vm);
	std::string cfgPath = vm.count("cfg") ? vm["cfg"].as<std::string>() : loc.parent_path().native()+"/bds.conf";
	notify(vm);
	parseConfig(cfgPath);
	if(vm.count("version")){
		std::cout << VERSION << std::endl;
		return 0;
	}
	if(vm.count("help")){
		std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
		std::cout << clopts << std::endl;
		return 0;
	}
	if(vm.count("poolSize")){
		config["poolSize"] = vm["poolSize"].as<std::string>();
	}
	if(vm.count("path")){
		config["path"] = vm["path"].as<std::string>();
	}
	if(vm.count("threads")){
		config["threads"] = vm["threads"].as<std::string>();
	}
	if(vm.count("ip")){
		config["ip"] = vm["ip"].as<std::string>();
	}
	if(vm.count("port")){
		config["port"] = vm["port"].as<std::string>();
	}
	if(config["path"].front() != '/'){
		config["path"] = loc.parent_path().native() + "/" + config["path"];
	}
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