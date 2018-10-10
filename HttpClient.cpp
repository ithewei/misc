#include "HttpClient.h"
#include "hw/hlog.h"

bool HttpClient::s_bInit = false;

HttpClient::HttpClient() {
	if (!s_bInit) {
        s_bInit = true;
		curl_global_init(CURL_GLOBAL_ALL);
	}
	m_timeout = 0;
	m_bDebug = false;
}

HttpClient::~HttpClient() {
	// curl_global_cleanup();
}

int HttpClient::Send(HttpRequest& req, HttpResponse& res) {
	return curl(req, res);
}

static size_t s_formget_cb(void *arg, const char *buf, size_t len) {
	hlogd("%s", buf);
	return len;
}

static size_t s_write_cb(char *buf, size_t size, size_t cnt, void *userdata) {
	if (buf == NULL || userdata == NULL)	return 0;

	HttpResponse* pRes = (HttpResponse*)userdata;
	pRes->append(buf, size*cnt);
	return size*cnt;
}

int HttpClient::curl(HttpRequest& req, HttpResponse& res) {
	CURL* handle = curl_easy_init();

	// method
	curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, req.method.c_str());

	// url
	curl_easy_setopt(handle, CURLOPT_URL, req.url.c_str());

	// header
	struct curl_slist *headers = NULL;
	if (m_headers.size() != 0) {
		for (int i = 0; i < m_headers.size(); ++i) {
			headers = curl_slist_append(headers, m_headers[i].c_str());
		}
	}
	const char* psz = "text/plain";
	switch (req.content_type) {
#define CASE_CONTENT_TYPE(id, str)	\
	case HttpRequest::id: psz = str;	break;

		FOREACH_CONTENT_TYPE(CASE_CONTENT_TYPE)
#undef	CASE_CONTENT_TYPE
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
	switch (req.content_type) {
	case HttpRequest::NONE:
		break;
	case HttpRequest::FORM_DATA: {
		struct curl_httppost* httppost = NULL;
		struct curl_httppost* lastpost = NULL;
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

	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, s_write_cb);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &res);

	int ret = curl_easy_perform(handle);
	if (ret != 0) {
		hloge("%s", curl_easy_strerror((CURLcode)ret));
	}

	if (headers) {
		curl_slist_free_all(headers);
	}

	curl_easy_cleanup(handle);

	if (m_bDebug) {
		hlogd("Response:%s", res.c_str());
	}

	return ret;
}