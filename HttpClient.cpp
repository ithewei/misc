#include "HttpClient.h"

#include "hw/hlog.h"
#include "hw/hstring.h"

atomic_flag HttpClient::s_bInit;

HttpClient::HttpClient() {
    if (!s_bInit.test_and_set()) {
        curl_global_init(CURL_GLOBAL_ALL);
    }
    m_timeout = 0;
    m_bDebug = false;
}

HttpClient::~HttpClient() {

}

int HttpClient::Send(const HttpRequest& req, HttpResponse* res) {
    return curl(req, res);
}

static size_t s_formget_cb(void *arg, const char *buf, size_t len) {
    hlogd("%s", buf);
    return len;
}

static size_t s_header_cb(char* buf, size_t size, size_t cnt, void* userdata) {
    if (buf == NULL || userdata == NULL)    return 0;

    HttpResponse* res = (HttpResponse*)userdata;

    string str(buf);
    string::size_type pos = str.find_first_of(':');
    if (pos == string::npos) {
        if (res->status.empty()) {
            res->status = trim(str);
        }
    } else {
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
            string header = asprintf("%s: %s", pair.first.c_str(), pair.second.c_str());
            headers = curl_slist_append(headers, header.c_str());
        }
    }
    const char* psz = "text/plain";
    switch (req.content_type) {
#define CASE_CONTENT_TYPE(id, str)  \
    case HttpRequest::id: psz = str;    break;

        FOREACH_CONTENT_TYPE(CASE_CONTENT_TYPE)
#undef  CASE_CONTENT_TYPE
    }
    string strContentType("Content-type: ");
    strContentType += psz;
    headers = curl_slist_append(headers, strContentType.c_str());
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
    if (m_bDebug) {
        hlogd("%s %s", req.method.c_str(), req.url.c_str());
        hlogd("%s", strContentType.c_str());
    }

    // body or params
    struct curl_httppost* httppost = NULL;
    struct curl_httppost* lastpost = NULL;
    switch (req.content_type) {
    case HttpRequest::NONE:
        break;
    case HttpRequest::FORM_DATA: {
        auto iter = req.form.begin();
        while (iter != req.form.end()) {
            CURLformoption opt = CURLFORM_COPYCONTENTS;
            if (iter->second.type == FormData::FILENAME) {
                opt = CURLFORM_FILE;
            }
            curl_formadd(&httppost, &lastpost,
                CURLFORM_COPYNAME, iter->first.c_str(),
                opt, iter->second.data.c_str(),
                CURLFORM_END);
            iter++;
        }
        if (httppost) {
            curl_easy_setopt(handle, CURLOPT_HTTPPOST, httppost);
            if (m_bDebug) {
                curl_formget(httppost, NULL, s_formget_cb);
            }
        }
    }
        break;
    case HttpRequest::QUERY_STRING:
    case HttpRequest::X_WWW_FORM_URLENCODED: {
        string params;
        auto iter = req.kvs.begin();
        while (iter != req.kvs.end()) {
            if (iter != req.kvs.begin()) {
                params += '&';
            }
            params += iter->first;
            params += '=';
            params += iter->second;
        }
        if (req.content_type == HttpRequest::QUERY_STRING) {
            string url_with_params(req.url);
            url_with_params += '?';
            url_with_params += params;
            curl_easy_setopt(handle, CURLOPT_URL, url_with_params.c_str());
            if (m_bDebug) {
                hlogd("%s", url_with_params.c_str());
            }
        } else {
            curl_easy_setopt(handle, CURLOPT_POSTFIELDS, params.c_str());
            if (m_bDebug) {
                hlogd("%s", params.c_str());
            }
        }
    }
        break;
    default: {
        curl_easy_setopt(handle, CURLOPT_POSTFIELDS, req.text.c_str());
        if (m_bDebug) {
            hlogd("%s", req.text.c_str());
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
        hloge("%d: %s", ret, curl_easy_strerror((CURLcode)ret));
    }

    if (m_bDebug) {
        if (res->body.length() != 0) {
            hlogd("Response:%s", res->body.c_str());
        }
        double total_time, name_time, conn_time, pre_time;
        curl_easy_getinfo(handle, CURLINFO_TOTAL_TIME, &total_time);
        curl_easy_getinfo(handle, CURLINFO_NAMELOOKUP_TIME, &name_time);
        curl_easy_getinfo(handle, CURLINFO_CONNECT_TIME, &conn_time);
        curl_easy_getinfo(handle, CURLINFO_PRETRANSFER_TIME, &pre_time);
        hlogd("TIME_INFO: %lf,%lf,%lf,%lf", total_time, name_time, conn_time, pre_time);
    }

    if (headers) {
        curl_slist_free_all(headers);
    }
    if (httppost) {
        curl_formfree(httppost);
    }

    curl_easy_cleanup(handle);

    return ret;
}

