#include "process/Popen.hpp"

#ifndef _WIN32
#  include <array>
#  include <cerrno>
#  include <cstring>
#  include <fcntl.h>
#  include <poll.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <unistd.h>
#else
#  include <cstdio>
#  include <cstdlib>
#  include <memory>
#  include <windows.h>
#endif

namespace subprocess {

#ifndef _WIN32

ProcessStream::ProcessStream(int fd) noexcept : fd_(fd) {}

ProcessStream::ProcessStream(ProcessStream&& other) noexcept
    : fd_(other.fd_), buffer_(std::move(other.buffer_))
{
    other.fd_ = -1;
}

ProcessStream& ProcessStream::operator=(ProcessStream&& other) noexcept
{
    if (this != &other) {
        if (fd_ >= 0) {
            ::close(fd_);
        }
        fd_ = other.fd_;
        buffer_ = std::move(other.buffer_);
        other.fd_ = -1;
    }
    return *this;
}

ProcessStream::~ProcessStream() noexcept
{
    if (fd_ >= 0) {
        ::close(fd_);
    }
}

bool ProcessStream::hasBufferedData() const noexcept
{
    return !buffer_.empty();
}

int ProcessStream::fd() const noexcept
{
    return fd_;
}

void ProcessStream::reset(int fd) noexcept
{
    if (fd_ != fd) {
        if (fd_ >= 0) {
            ::close(fd_);
        }
        fd_ = fd;
    }
}

std::string ProcessStream::readline()
{
    if (fd_ < 0 && buffer_.empty()) {
        return {};
    }

    std::string result;
    for (;;) {
        const auto newline_position = buffer_.find('\n');
        if (newline_position != std::string::npos) {
            result = buffer_.substr(0, newline_position + 1);
            buffer_.erase(0, newline_position + 1);
            return result;
        }

        if (fd_ < 0) {
            break;
        }

        char chunk[512];
        const ssize_t bytes_read = ::read(fd_, chunk, sizeof(chunk));
        if (bytes_read < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            ::close(fd_);
            fd_ = -1;
            break;
        }

        if (bytes_read == 0) {
            ::close(fd_);
            fd_ = -1;
            break;
        }

        buffer_.append(chunk, static_cast<size_t>(bytes_read));
    }

    if (!buffer_.empty()) {
        result = std::move(buffer_);
        buffer_.clear();
        return result;
    }

    return {};
}

static bool makeNonBlocking(int fd) noexcept
{
    const int flags = ::fcntl(fd, F_GETFL);
    if (flags < 0) {
        return false;
    }
    return ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0;
}

Popen::Popen(std::vector<std::string> args)
{
    if (args.empty()) {
        throw std::invalid_argument("Popen requires at least one command argument");
    }
    if (!start(args)) {
        throw std::system_error(errno, std::generic_category(), "failed to start subprocess");
    }
}

Popen::Popen(std::initializer_list<std::string> args)
    : Popen(std::vector<std::string>(args))
{
}

Popen::Popen(Popen&& other) noexcept
    : stdout(std::move(other.stdout)), stderr(std::move(other.stderr)), stdout_fd_(other.stdout_fd_), stderr_fd_(other.stderr_fd_), pid_(other.pid_), exit_code_(other.exit_code_), exited_(other.exited_)
{
    other.stdout_fd_ = -1;
    other.stderr_fd_ = -1;
    other.pid_ = -1;
    other.exit_code_ = -1;
    other.exited_ = false;
}

Popen& Popen::operator=(Popen&& other) noexcept
{
    if (this != &other) {
        if (pid_ > 0) {
            ::waitpid(pid_, nullptr, 0);
        }
        closeDescriptors();
        stdout = std::move(other.stdout);
        stderr = std::move(other.stderr);
        stdout_fd_ = other.stdout_fd_;
        stderr_fd_ = other.stderr_fd_;
        pid_ = other.pid_;
        exit_code_ = other.exit_code_;
        exited_ = other.exited_;
        other.stdout_fd_ = -1;
        other.stderr_fd_ = -1;
        other.pid_ = -1;
        other.exit_code_ = -1;
        other.exited_ = false;
    }
    return *this;
}

Popen::~Popen() noexcept
{
    if (pid_ > 0) {
        ::waitpid(pid_, nullptr, 0);
    }
    closeDescriptors();
}

ProcessStatus Popen::poll(int timeout_ms) noexcept
{
    ProcessStatus status;
    status.stdout_data = stdout.hasBufferedData();
    status.stderr_data = stderr.hasBufferedData();

    struct pollfd descriptors[2];
    int descriptor_count = 0;

    if (stdout.fd() >= 0) {
        descriptors[descriptor_count++] = {stdout.fd(), POLLIN | POLLHUP, 0};
    }
    if (stderr.fd() >= 0) {
        descriptors[descriptor_count++] = {stderr.fd(), POLLIN | POLLHUP, 0};
    }

    if (descriptor_count > 0) {
        const int events = ::poll(descriptors, descriptor_count, timeout_ms);
        if (events >= 0) {
            for (int index = 0; index < descriptor_count; ++index) {
                const auto revents = descriptors[index].revents;
                if (revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) {
                    if (index == 0) {
                        status.stdout_data = true;
                    } else {
                        status.stderr_data = true;
                    }
                }
            }
        }
    }

    if (pid_ > 0) {
        int child_status = 0;
        const pid_t result = ::waitpid(pid_, &child_status, WNOHANG);
        if (result == pid_) {
            status.exited = true;
            exited_ = true;
            exit_code_ = WIFEXITED(child_status) ? WEXITSTATUS(child_status) : -1;
            pid_ = -1;
        }
    } else if (exited_) {
        status.exited = true;
    }

    return status;
}

int Popen::wait()
{
    if (pid_ > 0) {
        int child_status = 0;
        const pid_t result = ::waitpid(pid_, &child_status, 0);
        pid_ = -1;
        if (result != -1 && WIFEXITED(child_status)) {
            exit_code_ = WEXITSTATUS(child_status);
            exited_ = true;
            return exit_code_;
        }
        return -1;
    }

    if (exited_) {
        return exit_code_;
    }

    return -1;
}

void Popen::closeDescriptors() noexcept
{
    stdout.reset(-1);
    stderr.reset(-1);
    stdout_fd_ = -1;
    stderr_fd_ = -1;
}

bool Popen::start(std::vector<std::string> const& args)
{
    int stdout_pipe[2] = {-1, -1};
    int stderr_pipe[2] = {-1, -1};

    if (::pipe(stdout_pipe) != 0 || ::pipe(stderr_pipe) != 0) {
        if (stdout_pipe[0] >= 0) {
            ::close(stdout_pipe[0]);
        }
        if (stdout_pipe[1] >= 0) {
            ::close(stdout_pipe[1]);
        }
        if (stderr_pipe[0] >= 0) {
            ::close(stderr_pipe[0]);
        }
        if (stderr_pipe[1] >= 0) {
            ::close(stderr_pipe[1]);
        }
        return false;
    }

    const pid_t child_pid = ::fork();
    if (child_pid < 0) {
        ::close(stdout_pipe[0]);
        ::close(stdout_pipe[1]);
        ::close(stderr_pipe[0]);
        ::close(stderr_pipe[1]);
        return false;
    }

    if (child_pid == 0) {
        ::dup2(stdout_pipe[1], STDOUT_FILENO);
        ::dup2(stderr_pipe[1], STDERR_FILENO);
        ::close(stdout_pipe[0]);
        ::close(stdout_pipe[1]);
        ::close(stderr_pipe[0]);
        ::close(stderr_pipe[1]);

        std::vector<char*> argv;
        argv.reserve(args.size() + 1);
        for (auto const& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        ::execvp(argv[0], argv.data());
        std::exit(127);
    }

    ::close(stdout_pipe[1]);
    ::close(stderr_pipe[1]);

    if (!makeNonBlocking(stdout_pipe[0]) || !makeNonBlocking(stderr_pipe[0])) {
        ::close(stdout_pipe[0]);
        ::close(stderr_pipe[0]);
        return false;
    }

    stdout.reset(stdout_pipe[0]);
    stderr.reset(stderr_pipe[0]);
    stdout_fd_ = stdout_pipe[0];
    stderr_fd_ = stderr_pipe[0];
    pid_ = child_pid;
    return true;
}

#endif // _WIN32

ProcessResult runPythonCommand(std::string_view script, std::vector<std::string> const& args)
{
    std::string command;
#ifdef _WIN32
    command = "python -c \"";
#else
    command = "python3 -c \"";
#endif
    command += std::string(script);
    command += "\"";

    for (auto const& arg : args) {
        command += ' ';
        command += arg;
    }

    std::string output;
#ifdef _WIN32
    FILE* pipe = _popen(command.c_str(), "r");
#else
    FILE* pipe = popen(command.c_str(), "r");
#endif
    if (!pipe) {
        return ProcessResult{-1, "", "failed to open python subprocess"};
    }

    std::array<char, 256> buffer;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }

#ifdef _WIN32
    const int exitCode = _pclose(pipe);
#else
    const int exitCode = pclose(pipe);
#endif

    return ProcessResult{exitCode, output, ""};
}

} // namespace subprocess
