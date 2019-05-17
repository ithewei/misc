#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_

/***************************************************************
HttpClient based libcurl
***************************************************************/
#include <atomic>

#include <curl/curl.h>

#include "HttpRequest.h"

/*
 * @code
#include "HttpClient.h"

#include <stdio.h>

int main(int argc, char* argv[]) {
    HttpClient session;
    HttpRequest req;
    req.method = "GET";
    req.url = "www.baidu.com";
    HttpResponse res;
    int ret = session.Send(req, &res);
    if (ret != 0) {
        printf("%s %s failed => %d:%s\n", req.method.c_str(), req.url.c_str(), ret, HttpClient::strerror(ret));
    }
    else {
        printf("%s %d %s\r\n", res.version.c_str(), res.status_code, res.status_message.c_str());
        for (auto& header : res.headers) {
            printf("%s: %s\r\n", header.first.c_str(), header.second.c_str());
        }
        printf("\r\n");
        printf("%s", res.body.c_str());
        printf("\n");
    }
    return ret;
}
 */
class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    int Send(const HttpRequest& req, HttpResponse* res);
    static const char* strerror(int errcode);

    void SetTimeout(int sec) {m_timeout = sec;}
    void AddHeader(string key, string value) {
        m_headers[key] = value;
    }
    void DelHeader(string key) {
        auto iter = m_headers.find(key);
        if (iter != m_headers.end()) {
            m_headers.erase(iter);
        }
    }
    void ClearHeader() {
        m_headers.clear();
    }

protected:
    int curl(const HttpRequest& req, HttpResponse* res);

private:
    static std::atomic_flag s_bInit;
    int m_timeout; // unit:s default:10s
    KeyValue m_headers;
};

#endif  // HTTP_CLIENT_H_
