#ifndef GRPC_SERVER_H_
#define GRPC_SERVER_H_

#include "grpc++/grpc++.h"
using grpc::ServerBuilder;
using grpc::Server;
using grpc::Service;

#include "hw/hstring.h"

class GrpcServer {
 public:
    GrpcServer(int port) : _port(port) {
        string srvaddr = asprintf("%s:%d", "0.0.0.0", _port);
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

