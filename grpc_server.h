#ifndef GRPC_SERVER_H_
#define GRPC_SERVER_H_

#include <stdio.h>
#include <grpcpp/grpcpp.h>
using grpc::ServerBuilder;
using grpc::Server;
using grpc::Service;

class GrpcServer {
public:
    GrpcServer(int port) : _port(port) {
        char srvaddr[128];
        snprintf(srvaddr, sizeof(srvaddr), "%s:%d", "0.0.0.0", port);
        build_.AddListeningPort(srvaddr, grpc::InsecureServerCredentials());
    }

    void RegisterService(Service* service) {
        build_.RegisterService(service);
    }

    void Run() {
        auto server = build_.BuildAndStart();
        server->Wait();
    }

public:
    int _port;
private:
    ServerBuilder build_;
};

#endif  // GRPC_SERVER_H_

