#include "HttpClient.h"

#include <string.h>
#include <string>
using std::string;

#define SPACE_CHARS     " \t\r\n"
static string trim(const string& str) {
    string::size_type pos1 = str.find_first_not_of(SPACE_CHARS);
    if (pos1 == string::npos)   return "";

    string::size_type pos2 = str.find_last_not_of(SPACE_CHARS);
    return str.substr(pos1, pos2-pos1+1);
}

std::atomic_flag HttpClient::s_bInit;

#define DEFAULT_TIMEOUT     10
HttpClient::HttpClient() {
    if (!s_bInit.test_and_set()) {
        curl_global_init(CURL_GLOBAL_ALL);
    }
    m_timeout = DEFAULT_TIMEOUT;
}

HttpClient::~HttpClient() {

}

int HttpClient::Send(const HttpRequest& req, HttpResponse* res) {
    return curl(req, res);
}

static size_t s_formget_cb(void *arg, const char *buf, size_t len) {
    return len;
}

static size_t s_header_cb(char* buf, size_t size, size_t cnt, void* userdata) {
    if (buf == NULL || userdata == NULL)    return 0;

    HttpResponse* res = (HttpResponse*)userdata;

    string str(buf);
    string::size_type pos = str.find_first_of(':');
    if (pos == string::npos) {
        if (res->version.empty() && strncmp(buf, "HTTP", 4) == 0) {
            // status line
            // HTTP/1.1 200 OK\r\n
            char* space1 = strchr(buf, ' ');
            char* space2 = strchr(space1+1, ' ');
            if (space1 && space2) {
                *space1 = '\0';
                *space2 = '\0';
                res->version = buf;
                res->status_code = atoi(space1+1);
                res->status_message = trim(space2+1);
            }
        }
    }
    else {
        // headers
        string key = trim(str.substr(0, pos));
        string value = trim(str.substr(pos+1));
        res->headers[key] = value;
    }
    return size*cnt;
}

static size_t s_body_cb(char *buf, size_t size, size_t cnt, void *userdata) {
    if (buf == NULL || userdata == NULL)    return 0;

    HttpResponse* res = (HttpResponse*)userdata;
    res->body.append(buf, size*cnt);
    return size*cnt;
}

static string escape(const string& param) {
    string res;
    const char* p = param.c_str();
    int len = param.size();
    char escape[4] = {0};
    for (int i = 0; i < len; ++i) {
        if (p[i] == ' ' ||
            p[i] == '?' ||
            p[i] == '=' ||
            p[i] == '&' ||
            p[i] == '%') {
            sprintf(escape, "%%%02X", p[i]);
            res += escape;
        }
        else {
            res += p[i];
        }
    }
    return res;
}

inline bool is_hex(char c) {
    return  (c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'F') ||
            (c >= 'a' && c <= 'f');
}

inline int c2i(char c) {
    return c-'0';
}

static string unescape(const string& escape_param) {
    string res;
    const char* p = escape_param.c_str();
    int len = escape_param.size();
    int i = 0;
    while (i < len) {
        if (p[i] == '%' && i < len-2 && is_hex(p[i+1]) && is_hex(p[i+2])) {
            res += c2i(p[i+1])*16 + c2i(p[i+2]);
            i += 3;
        }
        else {
            res += p[i];
            ++i;
        }
    }
    return res;
}

int HttpClient::curl(const HttpRequest& req, HttpResponse* res) {
    CURL* handle = curl_easy_init();

    // SSL
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0);

    // method
    curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, req.method.c_str());

    // url
    curl_easy_setopt(handle, CURLOPT_URL, req.url.c_str());

    // header
    struct curl_slist *headers = NULL;
    if (m_headers.size() != 0) {
        for (auto& pair : m_headers) {
            string header = pair.first;
            header += ": ";
            header += pair.second;
            headers = curl_slist_append(headers, header.c_str());
        }
    }
    const char* psz = "text/plain";
    switch (req.content_type) {
#define CASE_CONTENT_TYPE(id, str)  \
    case id: psz = str;    break;

        FOREACH_CONTENT_TYPE(CASE_CONTENT_TYPE)
#undef  CASE_CONTENT_TYPE
    }
    string strContentType("Content-type: ");
    strContentType += psz;
    headers = curl_slist_append(headers, strContentType.c_str());
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
    //hlogd("%s %s", req.method.c_str(), req.url.c_str());
    //hlogd("%s", strContentType.c_str());

    // body or params
    struct curl_httppost* httppost = NULL;
    struct curl_httppost* lastpost = NULL;
    switch (req.content_type) {
    case FORM_DATA:
    {
        for (auto& pair : req.form) {
            CURLformoption opt = pair.second.type == FormData::FILENAME ? CURLFORM_FILE : CURLFORM_COPYCONTENTS;
            curl_formadd(&httppost, &lastpost,
                    CURLFORM_COPYNAME, pair.first.c_str(),
                    opt, pair.second.data.c_str(),
                    CURLFORM_END);
        }
        if (httppost) {
            curl_easy_setopt(handle, CURLOPT_HTTPPOST, httppost);
            curl_formget(httppost, NULL, s_formget_cb);
        }
    }
    break;
    case QUERY_STRING:
    case X_WWW_FORM_URLENCODED:
    {
        string params;
        auto iter = req.kvs.begin();
        while (iter != req.kvs.end()) {
            if (iter != req.kvs.begin()) {
                params += '&';
            }
            params += escape(iter->first);
            params += '=';
            params += escape(iter->second);
            iter++;
        }
        if (req.content_type == QUERY_STRING) {
            string url_with_params(req.url);
            url_with_params += '?';
            url_with_params += params;
            curl_easy_setopt(handle, CURLOPT_URL, url_with_params.c_str());
            //hlogd("%s", url_with_params.c_str());
        }
        else {
            curl_easy_setopt(handle, CURLOPT_POSTFIELDS, params.c_str());
            //hlogd("%s", params.c_str());
        }
    }
    break;
    default:
    {
        if (req.text.size() != 0) {
         curl_easy_setopt(handle, CURLOPT_POSTFIELDS, req.text.c_str());
         //hlogd("%s", req.text.c_str());
        }
    }
    break;
    }

    if (m_timeout != 0) {
        curl_easy_setopt(handle, CURLOPT_TIMEOUT, m_timeout);
    }

    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, s_body_cb);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, res);

    curl_easy_setopt(handle, CURLOPT_HEADER, 0);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, s_header_cb);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, res);

    int ret = curl_easy_perform(handle);
    if (ret != 0) {
        //hloge("%d: %s", ret, curl_easy_strerror((CURLcode)ret));
    }

    //if (res->body.length() != 0) {
        //hlogd("Response:%s", res->body.c_str());
    //}
    //double total_time, name_time, conn_time, pre_time;
    //curl_easy_getinfo(handle, CURLINFO_TOTAL_TIME, &total_time);
    //curl_easy_getinfo(handle, CURLINFO_NAMELOOKUP_TIME, &name_time);
    //curl_easy_getinfo(handle, CURLINFO_CONNECT_TIME, &conn_time);
    //curl_easy_getinfo(handle, CURLINFO_PRETRANSFER_TIME, &pre_time);
    //hlogd("TIME_INFO: %lf,%lf,%lf,%lf", total_time, name_time, conn_time, pre_time);

    if (headers) {
        curl_slist_free_all(headers);
    }
    if (httppost) {
        curl_formfree(httppost);
    }

    curl_easy_cleanup(handle);

    return ret;
}

const char* HttpClient::strerror(int errcode) {
    return curl_easy_strerror((CURLcode)errcode);
}
