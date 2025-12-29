#include "aof_persistence.h"
#include <iostream>
#include <sstream>
#include <functional>

namespace kvstore {

AOFPersistence::AOFPersistence(const std::string& filename)
    : filename_(filename), enabled_(false) {
}

AOFPersistence::~AOFPersistence() {
    Disable();
}

bool AOFPersistence::Enable() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    file_.open(filename_, std::ios::app | std::ios::out);
    
    if (!file_.is_open()) {
        std::cerr << "Failed to open AOF file: " << filename_ << std::endl;
        return false;
    }
    
    enabled_ = true;
    std::cout << "AOF enabled: " << filename_ << std::endl;
    return true;
}

void AOFPersistence::Disable() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
    
    enabled_ = false;
}

void AOFPersistence::LogSet(const std::string& key, const std::string& value) {
    if (!enabled_) return;
    
    std::string escaped_value = value;
    size_t pos = 0;
    while ((pos = escaped_value.find('\n', pos)) != std::string::npos) {
        escaped_value.replace(pos, 1, "\\n");
        pos += 2;
    }
    
    std::ostringstream oss;
    oss << "SET " << key << " " << escaped_value << "\n";
    WriteCommand(oss.str());
}

void AOFPersistence::LogDelete(const std::string& key) {
    if (!enabled_) return;
    
    std::ostringstream oss;
    oss << "DELETE " << key << "\n";
    WriteCommand(oss.str());
}

void AOFPersistence::LogExpire(const std::string& key, int seconds) {
    if (!enabled_) return;
    
    std::ostringstream oss;
    oss << "EXPIRE " << key << " " << seconds << "\n";
    WriteCommand(oss.str());
}

void AOFPersistence::WriteCommand(const std::string& command) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (file_.is_open()) {
        file_ << command;
        file_.flush();
    }
}

bool AOFPersistence::Replay(ReplayCallback callback) {
    std::ifstream replay_file(filename_);
    
    if (!replay_file.is_open()) {
        std::cout << "No AOF file to replay: " << filename_ << std::endl;
        return true;
    }
    
    std::cout << "Replaying AOF file: " << filename_ << std::endl;
    
    std::string line;
    int command_count = 0;
    
    while (std::getline(replay_file, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string cmd, key, value;
        
        iss >> cmd;
        iss >> key;
        
        if (cmd == "SET") {
            std::getline(iss, value);
            if (!value.empty() && value[0] == ' ') {
                value = value.substr(1);
            }
            size_t pos = 0;
            while ((pos = value.find("\\n", pos)) != std::string::npos) {
                value.replace(pos, 2, "\n");
                pos += 1;
            }
        } else if (cmd == "EXPIRE") {
            iss >> value;
        }
        
        callback(cmd, key, value);
        command_count++;
    }
    
    replay_file.close();
    std::cout << "Replayed " << command_count << " commands from AOF" << std::endl;
    return true;
}

} // namespace kvstore
