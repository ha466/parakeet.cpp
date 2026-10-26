#include "transcription_json.hpp"
#include "transcription.hpp"

#include <cmath>
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

static void test_json_string_escaping() {
    std::string out;
    // Test special escape sequences
    pk::append_json_string(out, "Hello \"world\" \\\b\f\n\r\t");
    std::string expected = "\"Hello \\\"world\\\" \\\\\\b\\f\\n\\r\\t\"";
    if (out != expected) {
        std::fprintf(stderr, "test_json_string_escaping failed for special chars:\n  got: %s\n  exp: %s\n",
                     out.c_str(), expected.c_str());
        std::exit(1);
    }

    // Test control characters (< 0x20)
    out.clear();
    std::string ctrl;
    ctrl += (char)0x01; // \u0001
    ctrl += (char)0x1F; // \u001f
    pk::append_json_string(out, ctrl);
    expected = "\"\\u0001\\u001f\"";
    if (out != expected) {
        std::fprintf(stderr, "test_json_string_escaping failed for control chars:\n  got: %s\n  exp: %s\n",
                     out.c_str(), expected.c_str());
        std::exit(1);
    }

    // Test UTF-8 multibyte verbatim pass-through
    out.clear();
    pk::append_json_string(out, "café ñoño 日本語");
    expected = "\"café ñoño 日本語\"";
    if (out != expected) {
        std::fprintf(stderr, "test_json_string_escaping failed for UTF-8:\n  got: %s\n  exp: %s\n",
                     out.c_str(), expected.c_str());
        std::exit(1);
    }
}

static void test_json_int() {
    std::string out;
    pk::append_json_int(out, 0);
    pk::append_json_int(out, 42);
    pk::append_json_int(out, -123);
    std::string expected = "042-123";
    if (out != expected) {
        std::fprintf(stderr, "test_json_int failed:\n  got: %s\n  exp: %s\n",
                     out.c_str(), expected.c_str());
        std::exit(1);
    }
}

static void test_json_float() {
    std::string out;
    pk::append_json_float(out, "%.3f", 1.2345f);
    if (out != "1.235") {
        std::fprintf(stderr, "test_json_float failed for normal float:\n  got: %s\n", out.c_str());
        std::exit(1);
    }

    // Test NaN and Infinity
    out.clear();
    pk::append_json_float(out, "%.3f", std::numeric_limits<float>::quiet_NaN());
    if (out != "0") {
        std::fprintf(stderr, "test_json_float failed for NaN:\n  got: %s\n", out.c_str());
        std::exit(1);
    }

    out.clear();
    pk::append_json_float(out, "%.3f", std::numeric_limits<float>::infinity());
    if (out != "0") {
        std::fprintf(stderr, "test_json_float failed for Inf:\n  got: %s\n", out.c_str());
        std::exit(1);
    }

    out.clear();
    pk::append_json_float(out, "%.3f", -std::numeric_limits<float>::infinity());
    if (out != "0") {
        std::fprintf(stderr, "test_json_float failed for -Inf:\n  got: %s\n", out.c_str());
        std::exit(1);
    }
}

static void test_transcription_to_json() {
    // 1. Empty transcription without dropped_words (dropped_words = -1)
    {
        pk::Transcription tr;
        tr.text = "";
        tr.dropped_words = -1;
        std::string json = pk::transcription_to_json(tr, 0.08f);
        std::string expected = "{\"text\":\"\",\"frame_sec\":0.080000,\"words\":[],\"tokens\":[]}";
        if (json != expected) {
            std::fprintf(stderr, "test_transcription_to_json (empty) failed:\n  got: %s\n  exp: %s\n",
                         json.c_str(), expected.c_str());
            std::exit(1);
        }
    }

    // 2. Normal transcription with words, tokens, and guard (dropped_words >= 0)
    {
        pk::Transcription tr;
        tr.text = "hello world";
        tr.dropped_words = 2;

        pk::Word w1;
        w1.text = "hello";
        w1.start = 0.16f;
        w1.end = 0.48f;
        w1.conf = 0.95432f;
        tr.words.push_back(w1);

        pk::Word w2;
        w2.text = "world";
        w2.start = 0.56f;
        w2.end = 0.88f;
        w2.conf = 0.98765f;
        tr.words.push_back(w2);

        pk::TokenInfo t1;
        t1.id = 101;
        t1.frame = 2;
        t1.conf = 0.95f;
        tr.tokens.push_back(t1);

        pk::TokenInfo t2;
        t2.id = 202;
        t2.frame = 7;
        t2.conf = 0.99f;
        tr.tokens.push_back(t2);

        std::string json = pk::transcription_to_json(tr, 0.08f);
        std::string expected =
            "{\"text\":\"hello world\",\"frame_sec\":0.080000,"
            "\"words\":["
            "{\"w\":\"hello\",\"start\":0.160,\"end\":0.480,\"conf\":0.9543},"
            "{\"w\":\"world\",\"start\":0.560,\"end\":0.880,\"conf\":0.9876}"
            "],"
            "\"tokens\":["
            "{\"id\":101,\"t\":0.160,\"conf\":0.9500},"
            "{\"id\":202,\"t\":0.560,\"conf\":0.9900}"
            "],"
            "\"guard\":{\"dropped_words\":2}}";

        if (json != expected) {
            std::fprintf(stderr, "test_transcription_to_json (normal) failed:\n  got: %s\n  exp: %s\n",
                         json.c_str(), expected.c_str());
            std::exit(1);
        }
    }
}

int main() {
    test_json_string_escaping();
    test_json_int();
    test_json_float();
    test_transcription_to_json();
    return 0;
}
