#ifndef HTTP_CLIENT_H_
#define HTTP_CLIENT_H_

/***************************************************************
HttpClient based libcurl
***************************************************************/

#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;

#include <atomic>
using std::atomic_flag;

#include <curl/curl.h>

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

struct FormData {
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
    FormData(string& str, FormDataType type = CONTENT) {
        this->type = type;
        this->data = str;
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
    FormData(double lf) {
        this->type = CONTENT;
        this->data = std::to_string(lf);
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

    void SetDebug(bool b) {m_bDebug = b;}
    void SetTimeout(int sec) {m_timeout = sec;}
    void ClearHeader() {m_headers.clear();}
    void AddHeader(string header) {m_headers.emplace_back(header);}

protected:
    int curl(const HttpRequest& req, HttpResponse* res);

private:
    static atomic_flag s_bInit;
    bool m_bDebug;
    int m_timeout;
    vector<string> m_headers;
};

#endif  // HTTP_CLIENT_H_

