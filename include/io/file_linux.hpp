#pragma once
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <poll.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <ios>

// #include <gsl/span>
// #include <filesystem>
// namespace fs = std::filesystem;
// namespace sys = std;
// using namespace std::chrono_literals;

#if !defined(STATICFORCEINLINE)
#   define STATICFORCEINLINE [[gnu::always_inline]] static inline
#endif

namespace io::impl {
    using handle_t = int;
    using openmode = std::ios_base::openmode;

    static constexpr const impl::handle_t invalid_handle = -1;
    
    static inline auto tty_int2speed(int number) noexcept -> speed_t {
        //speed_t result = B115200;
        switch (number) {
            case    2400: return    B2400; break;
            case    4800: return    B4800; break;
            case    9600: return    B9600; break;
            case   19200: return   B19200; break;
            case   38400: return   B38400; break;
            case   57600: return   B57600; break;
            case  115200: return  B115200; break;
            case  230400: return  B230400; break;
            case  460800: return  B460800; break;
            case  500000: return  B500000; break;
            case  576000: return  B576000; break;
            case  921600: return  B921600; break;
            case 1000000: return B1000000; break;
            case 1152000: return B1152000; break;
            case 1500000: return B1500000; break;
            case 2000000: return B2000000; break;
            case 2500000: return B2500000; break;
            case 3000000: return B3000000; break;
            case 3500000: return B3500000; break;
            case 4000000: return B4000000; break;
        }
        return B57600;
    }

    static inline auto tty_speed2int(speed_t speed) noexcept -> int {
        //speed_t result = B115200;
        switch (speed) {
            case    B2400 : return    2400; break;
            case    B4800 : return    4800; break;
            case    B9600 : return    9600; break;
            case   B19200 : return   19200; break;
            case   B38400 : return   38400; break;
            case   B57600 : return   57600; break;
            case  B115200 : return  115200; break;
            case  B230400 : return  230400; break;
            case  B460800 : return  460800; break;
            case  B500000 : return  500000; break;
            case  B576000 : return  576000; break;
            case  B921600 : return  921600; break;
            case B1000000 : return 1000000; break;
            case B1152000 : return 1152000; break;
            case B1500000 : return 1500000; break;
            case B2000000 : return 2000000; break;
            case B2500000 : return 2500000; break;
            case B3000000 : return 3000000; break;
            case B3500000 : return 3500000; break;
            case B4000000 : return 4000000; break;
        }
        return -1;
    }

    static auto oflags_from_string(const std::string& mode) -> int {
        if (mode.contains('r')) {
            // "r"   : O_RDONLY // if exists: Read from start, else Fail
            // "rb"  : O_RDONLY // if exists: Read from start, else Fail
            // "r+"  : O_RDWR   // if exists: Read from start, else Fail
            // "r+b" : O_RDWR   // if exists: Read from start, else Fail
            return mode.contains('+') ? O_RDWR : O_RDONLY;
        }
        if (mode.contains('a')) {
            // "a"   : O_RDONLY|O_APPEND // Create if nessecary, append to end
            // "ab"  : O_RDONLY|O_APPEND // Create if nessecary, append to end
            // "a+"  : O_RDWR  |O_APPEND // Create if nessecary, append to end
            // "a+b" : O_RDWR  |O_APPEND // Create if nessecary, append to end
            return mode.contains('+') ? (O_RDWR|O_CREAT|O_APPEND) : (O_WRONLY|O_CREAT|O_APPEND);
        }
        if (mode.contains('w')) {
            if (mode.contains('x')) {
                // "wx"  : O_WRONLY|O_EXCL          // if exists: Fail, else Create new 
                // "wbx" : O_WRONLY|O_EXCL          // if exists: Fail, else Create new
                // "w+x" :   O_RDWR|O_EXCL|O_TRUNC  // if exists: Fail, else Create new
                // "w+bx":   O_RDWR|O_EXCL|O_TRUNC  // if exists: Fail, else Create new
                return mode.contains('+') ? (O_RDWR|O_TRUNC|O_EXCL) : (O_WRONLY|O_EXCL);
            } else {
                // "w"   : O_WRONLY|O_CREAT|O_TRUNC // Create new or destroy contents if already existing 
                // "wb"  : O_WRONLY|O_CREAT|O_TRUNC // Create new or destroy contents if already existing 
                // "w+"  :   O_RDWR|O_CREAT|O_TRUNC // Create new or destroy contents if already existing 
                // "w+b" :   O_RDWR|O_CREAT|O_TRUNC // Create new or destroy contents if already existing 
                return mode.contains('+') ? (O_RDWR|O_CREAT|O_TRUNC) : (O_WRONLY|O_CREAT|O_TRUNC);
            }
        }
        return O_RDONLY;
    }

    struct Handle {
        handle_t fd = invalid_handle;
        int      flags = 0;

        /******************************************************************************/
        // Default Construction
        constexpr Handle() = default;
        constexpr Handle(handle_t _handle, int _flags) noexcept : fd(_handle), flags(_flags) {}
        Handle(handle_t _handle) noexcept : fd(_handle), flags(::fcntl(fd, F_GETFL)) {}
        // Moveablity
        constexpr Handle(Handle& x) noexcept  : fd(x.fd), flags(x.flags) { x.fd = impl::invalid_handle;x.flags = 0; }
        constexpr Handle(Handle&& x) noexcept : fd(x.fd), flags(x.flags) { x.fd = impl::invalid_handle;x.flags = 0; }
        auto operator=(Handle&& x) noexcept -> Handle& {
            if (this != &x) {
                // if this->fd is not invalid_handle, close it before it gets overwritten
                if (fd != impl::invalid_handle) close();
                fd = x.fd;
                flags = x.flags;
                x.fd = impl::invalid_handle; // Change source to invalid (true move)
                x.flags = 0;
            }
            return *this;
        }
        // Copyability (none allowed)
        constexpr Handle(const Handle& x) noexcept = delete;
        constexpr Handle(const Handle&& x) noexcept = delete;
        auto operator=(const Handle& x) noexcept = delete;
        // Destructor
        ~Handle() {
            close();
        }
        /******************************************************************************/
        inline void close() noexcept {
            if (fd!=invalid_handle) {
                ::close(fd);
                flags = 0;
            }
        }
        inline auto write(const void* buf, std::size_t size) noexcept->std::ptrdiff_t {
            auto result = ::write(fd,buf,size);
            if (result<=0) {
                fprintf(stderr,"Error [%d] writing to file: %s\n",errno, ::strerror(errno));
            }
            return result;
        }
        inline auto read(void* buf, std::size_t size) noexcept->std::ptrdiff_t {
            auto result = ::read(fd,buf,size);
            if (result<=0) {
                fprintf(stderr,"Error [%d] writing to file: %s\n",errno, ::strerror(errno));
            }
            return result;
        }
        /******************************************************************************/
        [[nodiscard]] auto valid() const noexcept -> bool {
            return (fd!=impl::invalid_handle);
        }
        [[nodiscard]] auto readable() const noexcept -> bool {
            int amode = flags & O_ACCMODE;
            return ( fd!=impl::invalid_handle) && ((amode==O_RDONLY)||(amode==O_RDWR) );
        }
        [[nodiscard]] auto writeable() const noexcept -> bool {
            int amode = flags & O_ACCMODE;
            return ( fd!=impl::invalid_handle) && ((amode==O_WRONLY)||(amode==O_RDWR) );
        }
        [[nodiscard]] auto poll_read(int timeout_ms=0) noexcept -> bool {
            struct pollfd fds[1];
            fds[0].fd = fd;
            fds[0].events = POLLIN;
            auto res = ::poll(fds,1,timeout_ms);
            if ((res==1)&&(fds[0].revents & POLLIN)) {
                return true;
            }
            if (res==-1) {
                fprintf(stderr,"Error [%d] polling file: %s\n", errno, strerror(errno));
            }
            return false;
        }
    };
    
    static inline auto open_file(const std::string& path,const std::string& mode) noexcept->Handle{
        auto oflags = oflags_from_string(mode);
        mode_t omode = (oflags&O_CREAT) ? (S_IRWXU|S_IRGRP|S_IROTH) : 0;
        auto fd = ::open(path.c_str(),oflags,omode);
        int flags;
        if (fd>0) {
            flags = ::fcntl(fd, F_GETFL);
        } else {
            fprintf(stderr,"Error [%d] writing to file: %s\n",errno, ::strerror(errno));
        }
        return {fd, flags};
    }

    static inline auto open_serial(const std::string& path,int baud) noexcept->Handle{
        auto fd = ::open(path.c_str(),O_RDWR | O_NOCTTY );
        int flags;
        if (fd>0) {
            struct termios nterm;
            memset(&nterm, 0, sizeof(nterm));
            //Input Modes
            nterm.c_iflag = (IGNPAR|IGNBRK);	// Ignore framing/parity errors, ignore break conditions
            //Output Modes
            nterm.c_oflag = 0; // No flags, just raw
            // Local flags
            nterm.c_lflag = 0; // ICANON would line buffer, might simplify things? But raw mode is ~ICANNON
            // Control mode flags
            nterm.c_cflag  = tty_int2speed(baud);
            //    CS8 - 8-bits per byte
            // CLOCAL - Ignore modem status lines (e.g. CD)
            //  CREAD - Allow reads
            nterm.c_cflag |= (CS8|CLOCAL|CREAD);
            nterm.c_cc[VTIME] = 0; /* Block forever (use poll() to manage timing) */
            nterm.c_cc[VMIN] = 1;  /* blocking read until N chars received */
            /* Discards all data received but not read with the TCIFLUSH flag. */
            tcflush(fd, TCIFLUSH);
            tcsetattr(fd, TCSANOW, &nterm);
            flags = ::fcntl(fd, F_GETFL);
        } else {
            fprintf(stderr,"Error [%d] opening TTY: %s\n",errno, ::strerror(errno));
        }
        return {fd, flags};
    }
}