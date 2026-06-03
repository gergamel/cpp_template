#pragma once

#include <initializer_list>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#ifndef _WIN32
#  include <sys/types.h>
#endif

#ifdef stdout
#  undef stdout
#endif
#ifdef stderr
#  undef stderr
#endif

namespace subprocess {

struct ProcessResult {
    int returncode = -1;
    std::string output;
    std::string error;
};

struct ProcessStatus {
    bool stdout_data = false;
    bool stderr_data = false;
    bool exited = false;
};

class ProcessStream {
public:
    ProcessStream() noexcept = default;
    explicit ProcessStream(int fd) noexcept;
    ProcessStream(ProcessStream&& other) noexcept;
    ProcessStream& operator=(ProcessStream&& other) noexcept;
    ProcessStream(ProcessStream const&) = delete;
    ProcessStream& operator=(ProcessStream const&) = delete;
    ~ProcessStream() noexcept;

    bool hasBufferedData() const noexcept;
    std::string readline();
    int fd() const noexcept;
    void reset(int fd) noexcept;

private:
    int fd_ = -1;
    std::string buffer_;
};

class Popen {
public:
    explicit Popen(std::vector<std::string> args);
    explicit Popen(std::initializer_list<std::string> args);
    Popen(Popen&& other) noexcept;
    Popen& operator=(Popen&& other) noexcept;
    Popen(Popen const&) = delete;
    Popen& operator=(Popen const&) = delete;
    ~Popen() noexcept;

    ProcessStatus poll(int timeout_ms = 0) noexcept;
    int wait();

    ProcessStream stdout;
    ProcessStream stderr;

private:
    bool start(std::vector<std::string> const& args);
    void closeDescriptors() noexcept;
    int stdout_fd_ = -1;
    int stderr_fd_ = -1;
#ifndef _WIN32
    pid_t pid_ = -1;
    int exit_code_ = -1;
    bool exited_ = false;
#else
    void* process_handle_ = nullptr;
#endif
};

ProcessResult runPythonCommand(std::string_view script, std::vector<std::string> const& args = {});

} // namespace subprocess
