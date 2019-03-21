#include "consul.h"

#include "HttpClient.h"

#include "json.hpp"
using json = nlohmann::json;

#include "hstring.h"
#include "herr.h"

#define PROTOCOL    "http://"
#define API_VERSION "v1"

const char url_register[] = "/agent/service/register";
const char url_deregister[] = "/agent/service/deregister";
const char url_discover[] = "/catalog/service";

string make_url(const char* ip, int port, const char* url) {
    return asprintf(PROTOCOL "%s:%d/" API_VERSION "%s",
            ip, port,
            url);
}

/*
{
  "ID": "redis1",
  "Name": "redis",
  "Tags": [
    "primary",
    "v1"
  ],
  "Address": "127.0.0.1",
  "Port": 8000,
  "Meta": {
    "redis_version": "4.0"
  },
  "EnableTagOverride": false,
  "Check": {
    "DeregisterCriticalServiceAfter": "90m",
    "Args": ["/usr/local/bin/check_redis.py"],
    "HTTP": "http://localhost:5000/health",
    "Interval": "10s",
    "TTL": "15s"
  },
  "Weights": {
    "Passing": 10,
    "Warning": 1
  }
}
 */
int register_service(consul_node_t* node, consul_service_t* service, consul_health_t* health) {
    HttpRequest req;
    req.method = "PUT";
    req.url = make_url(node->ip, node->port, url_register);
    req.content_type = HttpRequest::APPLICATION_JSON;

    json jservice;
    jservice["Name"] = service->name;
    if (strlen(service->ip) != 0) {
        jservice["Address"] = service->ip;
    }
    jservice["Port"] = service->port;

    json jcheck;
    jcheck[health->protocol] = health->url;
    jcheck["Interval"] = asprintf("%dms", health->interval);
    jcheck["DeregisterCriticalServiceAfter"] = asprintf("%dms", health->interval * 3);
    jservice["Check"] = jcheck;

    req.text = jservice.dump();

    HttpResponse res;
    HttpClient cli;
    int ret = cli.Send(req, &res);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int deregister_service(consul_node_t* node, const char* service_name) {
    string url = make_url(node->ip, node->port, url_deregister);
    url += '/';
    url += service_name;

    HttpRequest req;
    req.method = "PUT";
    req.url = url;
    req.content_type = HttpRequest::APPLICATION_JSON;

    HttpResponse res;
    HttpClient cli;
    int ret = cli.Send(req, &res);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int discover_services(consul_node_t* node, const char* service_name, std::vector<consul_service_t>& services) {
    string url = make_url(node->ip, node->port, url_discover);
    url += '/';
    url += service_name;

    HttpRequest req;
    req.method = "GET";
    req.url = url;

    HttpResponse res;

    HttpClient cli;
    int ret = cli.Send(req, &res);
    if (ret != 0) {
        return ret;
    }

    json jroot = json::parse(res.body.c_str(), NULL, false);
    if (!jroot.is_array()) {
        return ERR_INVALID_JSON;
    }
    if (jroot.size() == 0) {
        return 0;
    }

    consul_service_t service;
    services.clear();
    for (int i = 0; i < jroot.size(); ++i) {
        auto jservice = jroot[i];
        if (!jservice.is_object()) {
            continue;
        }
        auto jname = jservice["ServiceName"];
        if (!jname.is_string()) {
            continue;
        }
        auto jip = jservice["ServiceAddress"];
        if (!jip.is_string()) {
            continue;
        }
        auto jport = jservice["ServicePort"];
        if (!jport.is_number_integer()) {
            continue;
        }

        string name = jname;
        string ip = jip;
        int    port = jport;

        strncpy(service.name, name.c_str(), sizeof(service.name));
        strncpy(service.ip, name.c_str(), sizeof(service.ip));
        service.port = port;
        services.emplace_back(service);
    }

    return 0;
}
