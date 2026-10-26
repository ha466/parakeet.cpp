#include "httplib.h"
#include <cstdio>

static int fails = 0;
static void check(bool ok, const char* msg) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", msg); ++fails; }
}

int main() {
    httplib::Request req_gzip;
    req_gzip.set_header("Accept-Encoding", "gzip, deflate");
    httplib::Response res_json;
    res_json.set_header("Content-Type", "application/json");

    httplib::detail::EncodingType enc1 = httplib::detail::encoding_type(req_gzip, res_json);
    check(enc1 == httplib::detail::EncodingType::Gzip, "encoding_type accepts gzip");

    httplib::Request req_gzip_q0;
    req_gzip_q0.set_header("Accept-Encoding", "gzip;q=0, deflate");
    httplib::detail::EncodingType enc2 = httplib::detail::encoding_type(req_gzip_q0, res_json);
    check(enc2 == httplib::detail::EncodingType::None, "encoding_type rejects gzip;q=0");

    httplib::Request req_none;
    req_none.set_header("Accept-Encoding", "deflate");
    httplib::detail::EncodingType enc3 = httplib::detail::encoding_type(req_none, res_json);
    check(enc3 == httplib::detail::EncodingType::None, "encoding_type none when no gzip");

    if (fails) { std::fprintf(stderr, "%d checks failed\n", fails); return 1; }
    std::printf("test_httplib_gzip: OK\n");
    return 0;
}
