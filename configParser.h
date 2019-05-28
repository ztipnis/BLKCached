//config_parser
#include <string>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <algorithm>

#ifndef BLKCACHE_CFG_PARSE
#define BLKCACHE_CFG_PARSE
/**
 * map that stores the config info from the config file
 */
static std::unordered_map<std::string,std::string> config;
/**
 * @brief      parse the config file
 *
 * @param[in]  config_path  The path to the config file
 */
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