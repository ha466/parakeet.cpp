// Unit test for pk::write_file_atomic. No model needed.
#include "common.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace fs = std::filesystem;

static int failures = 0;
#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::fprintf(stderr, "FAIL: %s (line %d)\n", #cond, __LINE__);  \
            ++failures;                                                      \
        }                                                                    \
    } while (0)

static std::string read_all(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

int main() {
    const fs::path dir = fs::temp_directory_path() / "pk_test_write_atomic";
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir);
    const fs::path file = dir / "reg.bin";
    const std::string tmp = file.string() + ".tmp";

    // A new file gets exactly the bytes, including a NUL byte.
    const std::string first("first\0bytes", 11);
    std::string err;
    CHECK(pk::write_file_atomic(file.string(), first, &err));
    CHECK(err.empty());
    CHECK(read_all(file) == first);
    CHECK(!fs::exists(tmp));

    // Overwriting replaces the whole file and leaves no tmp file.
    const std::string second = "second, longer than the first one";
    CHECK(pk::write_file_atomic(file.string(), second, &err));
    CHECK(read_all(file) == second);
    CHECK(!fs::exists(tmp));
    CHECK(pk::write_file_atomic(file.string(), "x", nullptr));   // err may be null
    CHECK(read_all(file) == "x");
    CHECK(pk::write_file_atomic(file.string(), second, nullptr));

    // Writing an empty byte string creates or truncates file to 0 bytes.
    const fs::path empty_file = dir / "empty.bin";
    CHECK(pk::write_file_atomic(empty_file.string(), "", &err));
    CHECK(err.empty());
    CHECK(fs::exists(empty_file));
    CHECK(fs::file_size(empty_file) == 0);

    CHECK(pk::write_file_atomic(file.string(), "", &err));
    CHECK(err.empty());
    CHECK(fs::file_size(file) == 0);
    // Restore file content
    CHECK(pk::write_file_atomic(file.string(), second, &err));
    CHECK(read_all(file) == second);

    // A path in a directory that does not exist fails with a reason, and the
    // existing file is untouched.
    err.clear();
    CHECK(!pk::write_file_atomic((dir / "missing" / "reg.bin").string(), "new", &err));
    CHECK(!err.empty());
    CHECK(err.find("cannot write") != std::string::npos);
    CHECK(read_all(file) == second);
    CHECK(!fs::exists(dir / "missing"));

    // Parent path is a regular file (ENOTDIR on opening .tmp).
    const fs::path reg_parent = dir / "parent_is_file";
    {
        std::ofstream out(reg_parent);
        out << "im a file";
    }
    err.clear();
    CHECK(!pk::write_file_atomic((reg_parent / "child.bin").string(), "data", &err));
    CHECK(!err.empty());
    CHECK(err.find("cannot write") != std::string::npos);

    // A target whose .tmp path is blocked by a directory fails safely on fopen.
    const fs::path blocked_target = dir / "blocked.bin";
    const fs::path blocked_tmp = dir / "blocked.bin.tmp";
    fs::create_directories(blocked_tmp);
    err.clear();
    CHECK(!pk::write_file_atomic(blocked_target.string(), "data", &err));
    CHECK(!err.empty());
    CHECK(err.find("cannot write") != std::string::npos);
    CHECK(fs::is_directory(blocked_tmp));
    CHECK(!fs::exists(blocked_target));

    // Writing to a read-only directory fails (skipped if running as root).
    bool is_root = false;
#ifndef _WIN32
    if (geteuid() == 0) {
        is_root = true;
    }
#endif
    if (!is_root) {
        const fs::path ro_dir = dir / "readonly_dir";
        fs::create_directories(ro_dir);
        fs::permissions(ro_dir, fs::perms::owner_read | fs::perms::owner_exec, fs::perm_options::replace);
        err.clear();
        CHECK(!pk::write_file_atomic((ro_dir / "file.bin").string(), "data", &err));
        CHECK(!err.empty());
        // Restore write permissions for cleanup
        fs::permissions(ro_dir, fs::perms::owner_all, fs::perm_options::replace);
    }

    // A target that is a directory fails, the directory survives, no tmp file.
    const fs::path sub = dir / "sub";
    fs::create_directories(sub);
    err.clear();
    CHECK(!pk::write_file_atomic(sub.string(), "new", &err));
    CHECK(!err.empty());
    CHECK(fs::is_directory(sub));
    CHECK(!fs::exists(sub.string() + ".tmp"));
    CHECK(read_all(file) == second);

    // An empty path is refused.
    err.clear();
    CHECK(!pk::write_file_atomic("", "x", &err));
    CHECK(err == "path is empty");

    // Null err parameter is safely handled across all failure modes.
    CHECK(!pk::write_file_atomic("", "x", nullptr));
    CHECK(!pk::write_file_atomic((dir / "missing" / "reg.bin").string(), "x", nullptr));
    CHECK(!pk::write_file_atomic(blocked_target.string(), "x", nullptr));
    CHECK(!pk::write_file_atomic(sub.string(), "x", nullptr));

    fs::remove_all(dir, ec);
    if (failures) { std::fprintf(stderr, "%d failure(s)\n", failures); return 1; }
    std::printf("test_write_atomic: PASS\n");
    return 0;
}
