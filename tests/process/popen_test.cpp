#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include "process/Popen.hpp"
#include <gtest/gtest.h>

using namespace std::chrono_literals; // Enables suffix literals like 's' or 'ms'

TEST(Popen, ConcurrentNoOutput) {
    // Starts the process and immediately moves to the next line of Python code
    auto process = subprocess::Popen({"sleep", "1"});
    std::this_thread::sleep_for(500ms);
    process.wait();
}

TEST(Popen, ReadWhileRunning) {
    auto process = subprocess::Popen({"ls", "-la"});

    // Read lines as they are printed by the process
    subprocess::ProcessStatus status;
    while (true) {
        /*
         * subprocess::Popen::poll() is loosely based on Unix poll(), but
         * returns the following struct instead of updating a pollfd list.
         * It also optionally accepts timeout_ms as an argument
         * 
         * struct ProcessStatus {
         *     bool stdout_data;   // Set if a read of process.stdout would not block
         *     bool stderr_data;   // Set if a read of process.stderr would not block
         *     bool exited;        // Set once the process has exited (you may then read returncode)
         * };
         */
        status = process.poll(100);
        if (status.stdout_data) {
            auto line = process.stdout.readline();
            std::cout << "stdout: " << line;
        }
        if (status.stderr_data) {
            auto line = process.stderr.readline();
            std::cout << "stderr: " << line;
        }
        if (status.exited && !status.stdout_data && !status.stderr_data) {
            break;
        }
    }
}

TEST(Popen, CapturesStdout) {
    auto process = subprocess::Popen({"sh", "-c", "printf 'hello\n'"});
    std::string output;

    while (true) {
        auto status = process.poll(100);
        if (status.stdout_data) {
            output += process.stdout.readline();
        }
        if (status.exited) {
            break;
        }
    }

    EXPECT_EQ(output, "hello\n");
    EXPECT_EQ(process.wait(), 0);
}

TEST(Popen, CapturesStderr) {
    auto process = subprocess::Popen({"sh", "-c", "printf 'error\n' 1>&2"});
    std::string output;

    while (true) {
        auto status = process.poll(100);
        if (status.stderr_data) {
            output += process.stderr.readline();
        }
        if (status.exited) {
            break;
        }
    }

    EXPECT_EQ(output, "error\n");
    EXPECT_EQ(process.wait(), 0);
}

