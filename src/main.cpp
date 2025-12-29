#include "server/server.h"
#include <csignal>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

std::unique_ptr<kvstore::Server> g_server;

void SignalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << std::endl;
    if (g_server) {
        g_server->Shutdown();
    }
    exit(0);
}

void PrintUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n"
              << "Options:\n"
              << "  --master                Start as master node (default)\n"
              << "  --replica               Start as replica node\n"
              << "  --address <addr:port>   Server address (default: 0.0.0.0:50051)\n"
              << "  --master-address <addr:port>  Master address (required for replicas)\n"
              << "  --replicas <addr1,addr2,...>   Comma-separated replica addresses (for master)\n"
              << "\nExamples:\n"
              << "  Master:  " << program_name << " --master --address 0.0.0.0:50051 --replicas localhost:50052,localhost:50053\n"
              << "  Replica: " << program_name << " --replica --address 0.0.0.0:50052 --master-address localhost:50051\n"
              << std::endl;
}

int main(int argc, char** argv) {
    std::string server_address = "0.0.0.0:50051";
    std::string master_address;
    std::vector<std::string> replica_addresses;
    bool is_master = true;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            PrintUsage(argv[0]);
            return 0;
        } else if (arg == "--master") {
            is_master = true;
        } else if (arg == "--replica") {
            is_master = false;
        } else if (arg == "--address" && i + 1 < argc) {
            server_address = argv[++i];
        } else if (arg == "--master-address" && i + 1 < argc) {
            master_address = argv[++i];
        } else if (arg == "--replicas" && i + 1 < argc) {
            std::string replicas_str = argv[++i];
            size_t pos = 0;
            while ((pos = replicas_str.find(',')) != std::string::npos) {
                replica_addresses.push_back(replicas_str.substr(0, pos));
                replicas_str.erase(0, pos + 1);
            }
            if (!replicas_str.empty()) {
                replica_addresses.push_back(replicas_str);
            }
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            PrintUsage(argv[0]);
            return 1;
        }
    }
    
    if (!is_master && master_address.empty()) {
        std::cerr << "Error: --master-address is required for replica nodes" << std::endl;
        PrintUsage(argv[0]);
        return 1;
    }
    
    std::cout << "Starting Distributed Key-Value Store Server" << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "Role: " << (is_master ? "MASTER" : "REPLICA") << std::endl;
    std::cout << "Address: " << server_address << std::endl;
    
    if (!is_master) {
        std::cout << "Master: " << master_address << std::endl;
    }
    
    if (is_master && !replica_addresses.empty()) {
        std::cout << "Replicas: ";
        for (size_t i = 0; i < replica_addresses.size(); ++i) {
            std::cout << replica_addresses[i];
            if (i < replica_addresses.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
    
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
    
    try {
        g_server = std::make_unique<kvstore::Server>(server_address, is_master);
        
        if (is_master) {
            for (const auto& replica_addr : replica_addresses) {
                g_server->AddReplica(replica_addr);
            }
        } else {
            g_server->SetMaster(master_address);
        }
        
        g_server->Run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
