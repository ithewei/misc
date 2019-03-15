#ifndef HTTP_STATUS_H_
#define HTTP_STATUS_H_

#define HTTP_09 "HTTP/0.9"
#define HTTP_10 "HTTP/1.0"
#define HTTP_11 "HTTP/1.1"
#define HTTP_20 "HTTP/2.0"

#define FOREACH_HTTP_STATUS(F)\
    F(100, HTTP_CONTINUE, "Continue")\
    F(101, HTTP_SWITCH_PROTOCOLS, "Switch Protocols")\
    F(102, HTTP_PROCESSING, "Processing")\
    \
    F(200, HTTP_OK, "OK")\
    F(201, HTTP_CREATED, "Created")\
    F(202, HTTP_ACCEPTED, "Accepted")\
    F(204, HTTP_NO_CONTENT, "No Content")\
    F(206, HTTP_PARTIAL_CONTENT, "Partial Content")\
    \
    F(300, HTTP_SPECIAL_RESPONSE, "Special Response")\
    F(301, HTTP_MOVED_PERMANENTLY, "Moved Permanently")\
    F(302, HTTP_MOVED_TEMPORARILY, "Moved Temporarily")\
    F(303, HTTP_SEE_OTHER, "See Other")\
    F(304, HTTP_NOT_MODIFIED, "Not Modified")\
    F(307, HTTP_TEMPORARY_REDIRECT, "Temporary Redirect")\
    F(308, HTTP_PERMANENT_REDIRECT, "Permanent Redirect")\
    \
    F(400, HTTP_BAD_REQUEST, "Bad Request")\
    F(401, HTTP_UNAUTHORIZED, "Unauthorized")\
    F(403, HTTP_FORBIDDEN, "Forbidden")\
    F(404, HTTP_NOT_FOUND, "Not Found")\
    F(405, HTTP_NOT_ALLOWED, "Not Allowed")\
    F(408, HTTP_REQUEST_TIME_OUT, "Request Timeout")\
    F(409, HTTP_CONFLICT, "Conflict")\
    F(411, HTTP_LENGTH_REQUIRED, "Length Required")\
    F(412, HTTP_PRECONDITION_FAILED, "Precondition Failed")\
    F(413, HTTP_REQUEST_ENTITY_TOO_LARGE, "Request Entity Too Large")\
    F(414, HTTP_REQUEST_URI_TOO_LARGE, "Request Uri Too large")\
    F(415, HTTP_UNSUPPORTED_MEDIA_TYPE, "Unsupported Media Type")\
    F(416, HTTP_RANGE_NOT_SATISFIABLE, "Range Not Satisfiable")\
    F(421, HTTP_MISDIRECTED_REQUEST, "Misdirected request")\
    F(429, NGX_HTTP_TOO_MANY_REQUESTS, "Too Many Requests")\
    \
    F(500, HTTP_INTERNAL_ERROR, "Internal Error")

#define ENUM_HTTP_STATUS(code, macro, str) macro = code,

typedef enum http_status_e {
    FOREACH_HTTP_STATUS(ENUM_HTTP_STATUS)
} http_status_e;

inline const char* get_http_status_string(int code) {
#define CASE_HTTP_STATUS(code, macro, str) case code: return str;
    switch (code) {
    FOREACH_HTTP_STATUS(CASE_HTTP_STATUS)
    default:
        return "Undefined";
    }
#undef  CASE_HTTP_STATUS
}

#include <string>
/*
<!DOCTYPE html>
<html>
<head>
  <title>404 Not Found</title>
</head>
<body>
  <center><h1>404 Not Found</h1></center>
  <hr>
  <center>httpd/1.19.3.15</center>
</body>
</html>
 */
inline void make_http_status_page(int status_code, const char* status_string, std::string& page) {
    char szCode[8];
    snprintf(szCode, sizeof(szCode), "%d ", status_code);
    page += R"(<!DOCTYPE html>
<html>
<head>
  <title>)";
    page += szCode; page += status_string;
    page += R"(</title>
</head>
<body>
  <center><h1>)";
    page += szCode; page += status_string;
    page += R"(</h1></center>
  <hr>
  <center>)";
    page += "httpd/1.19.3.15";
    page += R"(<center>
</body>
</html>)";
}

inline void make_http_status_page(int status_code, std::string& page) {
    make_http_status_page(status_code, get_http_status_string(status_code), page);
}

#endif // HTTP_STATUS_H_
