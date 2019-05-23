//config_parser
#import <string>
#import <sstream>
#import <fstream>
#import <unordered_map>

#ifndef BLKCACHE_CFG_PARSE
#define BLKCACHE_CFG_PARSE

static std::unordered_map<std::string,std::string> config;
void parseConfig(std::string config_path){
	std::ifstream config_file(config_path);
	std::string line;
	while(getline(config_file, line)){
		line = line.substr(0, line.find("#", 0));
		line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
		if(line[0] == '#' || line.empty())
                continue;
        auto delimiterPos = line.find("=");
        auto name = line.substr(0, delimiterPos);
		auto value = line.substr(delimiterPos + 1);
		config[name] = value;
		//std::cout << name << " = " << value << std::endl;
	}
}

#endif