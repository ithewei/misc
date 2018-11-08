#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_

/***************************************************************
HttpClient based libcurl
***************************************************************/

#include <string>
#include <vector>
#include <map>

#include <curl/curl.h>

#include "hw/hstring.h"

// F(id, str)
#define FOREACH_CONTENT_TYPE(F) \
    F(TEXT_PLAIN,               "text/plain")   \
    F(TEXT_HTML,                "text/html")    \
    F(TEXT_XML,                 "text/xml")     \
    F(APPLICATION_JSON,         "application/json") \
    F(APPLICATION_XML,          "application/xml")  \
    F(APPLICATION_JAVASCRIPT,   "application/javascript")   \
    \
    F(FORM_DATA,                "multipart/form-data")  \
    \
    F(X_WWW_FORM_URLENCODED,    "application/x-www-form-urlencoded")    \
    F(QUERY_STRING,             "text/plain")

#define ENUM_CONTENT_TYPE(id, _)    id,

typedef std::map<std::string, std::string> KeyValue;

struct FormData{
    enum FormDataType {
        CONTENT,
        FILENAME
    } type;
    string       data;
    FormData() {
        type = CONTENT;
    }
    FormData(const char* data, FormDataType type = CONTENT) {
        this->type = type;
        this->data = data;
    }
    FormData(int n) {
        this->type = CONTENT;
        this->data = std::to_string(n);
    }
    FormData(long long n) {
        this->type = CONTENT;
        this->data = std::to_string(n);
    }
    FormData(float f) {
        this->type = CONTENT;
        this->data = std::to_string(f);
    }
};

typedef std::map<std::string, FormData>     Form;

struct HttpRequest {
    string              method;
    string              url;

    enum ContentType {
        NONE,
        FOREACH_CONTENT_TYPE(ENUM_CONTENT_TYPE)
        LAST
    } content_type;

    string          text;
    KeyValue        kvs;
    Form            form;
};
typedef std::string HttpResponse;

class HttpClient {
 public:
    HttpClient();
    ~HttpClient();

    int Send(const HttpRequest& req, HttpResponse* res);

    void setDebug(bool b) { m_bDebug = b; }
    void setTimeout(int second) { m_timeout = second; }
    void addHeader(string header) { m_headers.push_back(header); }
    void resetHeader() { m_headers.clear(); }

 protected:
    int curl(const HttpRequest& req, HttpResponse* res);

 private:
    static bool s_bInit;
    bool m_bDebug;
    vector<string> m_headers;
    int m_timeout;
};

#endif  // HTTP_CLIENT_H_

