#pragma once

#include <string>
#include <fstream>
#include <mutex>

namespace kvstore {

class AOFPersistence {
public:
    explicit AOFPersistence(const std::string& filename);
    ~AOFPersistence();

    bool Enable();
    void Disable();
    bool IsEnabled() const { return enabled_; }

    void LogSet(const std::string& key, const std::string& value);
    void LogDelete(const std::string& key);
    void LogExpire(const std::string& key, int seconds);

    using ReplayCallback = std::function<void(const std::string& cmd, const std::string& key, const std::string& value)>;
    bool Replay(ReplayCallback callback);

private:
    std::string filename_;
    std::ofstream file_;
    std::mutex mutex_;
    bool enabled_;

    void WriteCommand(const std::string& command);
};

} // namespace kvstore
