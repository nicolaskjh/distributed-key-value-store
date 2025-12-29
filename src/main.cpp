#include "server/server.h"
#include <csignal>
#include <iostream>
#include <memory>

std::unique_ptr<kvstore::Server> g_server;

void SignalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << std::endl;
    if (g_server) {
        g_server->Shutdown();
    }
    exit(0);
}

int main(int argc, char** argv) {
    std::string server_address = "0.0.0.0:50051";
    
    if (argc > 1) {
        server_address = argv[1];
    }
    
    std::cout << "Starting Distributed Key-Value Store Server" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    
    try {
        g_server = std::make_unique<kvstore::Server>(server_address);
        g_server->Run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
