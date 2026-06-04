#pragma once

#include <atomic>
#include <cctype> // Required for isdigit
#include <cstdio>
#include <algorithm>
// #include <filesystem>
#include <string>
#include <vector>
#include <format>

// Pull in ATL support
//  #include <atlbase.h>
//  #include <atlstr.h>
// Other includes

// NOMINMAX is needed before including Windows.h or you will get the
// following error everywhere you use std::min and std::max:
//  error C2589: '(': illegal token on right side of '::'
#define NOMINMAX
#include <Windows.h>
#include <Wbemcli.h>
#include <comdef.h>
#include <setupapi.h>
#include <tchar.h>
#include <winspool.h>
#include <handleapi.h>
#include <winbase.h>
#include <winnt.h>

#ifndef STATICFORCEINLINE
#if (_MSC_VER >= 1200)
#define STATICFORCEINLINE static __forceinline
#else
#define STATICFORCEINLINE static __inline
#endif
#endif

static inline bool IsNumeric(const std::string &s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

namespace io::impl {
    using handle_t = HANDLE;
    static const handle_t invalid_handle = INVALID_HANDLE_VALUE;

    struct Handle {
        handle_t fd = invalid_handle;
        // int      flags = 0; flags(::fcntl(fd, F_GETFL))

        /******************************************************************************/
        // Default Construction
        constexpr Handle() = default;
        constexpr Handle(handle_t _handle) noexcept : fd(_handle) {}
        // Moveablity
        constexpr Handle(Handle& x) noexcept  : fd(x.fd) { x.fd = impl::invalid_handle; }
        constexpr Handle(Handle&& x) noexcept : fd(x.fd) { x.fd = impl::invalid_handle; }
        auto operator=(Handle&& x) noexcept -> Handle& {
            if (this != &x) {
                // if this->fd is not invalid_handle, close it before it gets overwritten
                if (fd != impl::invalid_handle) close();
                fd = x.fd;
                x.fd = impl::invalid_handle; // Change source to invalid (true move)
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
            if (fd!=INVALID_HANDLE_VALUE) {
                CloseHandle(fd);
            }
        }
        inline auto write(const void* buf, std::size_t size) noexcept->std::ptrdiff_t {
            auto nbytes = static_cast<DWORD>(size);
            DWORD wlen;
            WriteFile(fd,buf,nbytes,&wlen,NULL);
            return static_cast<std::ptrdiff_t>(wlen);
            // auto result = ::write(fd,buf,size);
            // if (result<=0) {
            //     fprintf(stderr,"Error [%d] writing to file: %s\n",errno, ::strerror(errno));
            // }
            // return result;
        }
        inline auto read(void* buf, std::size_t size) noexcept->std::ptrdiff_t {
            auto nbytes = static_cast<DWORD>(size);
            DWORD rlen;
            ReadFile(fd,buf,nbytes,&rlen,NULL);
            return static_cast<std::ptrdiff_t>(rlen);
            // auto result = ::read(fd,buf,size);
            // if (result<=0) {
            //     fprintf(stderr,"Error [%d] writing to file: %s\n",errno, ::strerror(errno));
            // }
            // return result;
        }
        /******************************************************************************/
        [[nodiscard]] auto valid() const noexcept -> bool {
            return (fd!=impl::invalid_handle);
        }
        [[nodiscard]] auto readable() const noexcept -> bool {
            // int amode = flags & O_ACCMODE;
            // return ( fd!=impl::invalid_handle) && ((amode==O_RDONLY)||(amode==O_RDWR) );
            // TODO: Windows implementation
            return true;
        }
        [[nodiscard]] auto writeable() const noexcept -> bool {
            // int amode = flags & O_ACCMODE;
            // return ( fd!=impl::invalid_handle) && ((amode==O_WRONLY)||(amode==O_RDWR) );
            // TODO: Windows implementation
            return true;
        }
        [[nodiscard]] auto poll_read(int timeout_ms [[maybe_unused]]=0) noexcept -> bool {
            // struct pollfd fds[1];
            // fds[0].fd = fd;
            // fds[0].events = POLLIN;
            // auto res = ::poll(fds,1,timeout_ms);
            // if ((res==1)&&(fds[0].revents & POLLIN)) {
            //     return true;
            // }
            // if (res==-1) {
            //     fprintf(stderr,"Error [%d] polling file: %s\n", errno, strerror(errno));
            // }
            // return false;
            // TODO: Windows implementation
            return true;
        }
    };

    static auto open_file(const std::string& path, const std::string& mode)->Handle {
        DWORD access = 0;
        DWORD create = 0;
        // CREATE_NEW        If the specified file exists, the function fails and the last-error code is set to ERROR_FILE_EXISTS (80).
        // CREATE_ALWAYS     If the specified file exists and is writable, the function overwrites the file, the function succeeds, and last-error code is set to ERROR_ALREADY_EXISTS (183).
        // OPEN_EXISTING     If the specified file or device does not exist, the function fails and the last-error code is set to ERROR_FILE_NOT_FOUND (2).
        // OPEN_ALWAYS       If the specified file exists, the function succeeds and the last-error code is set to ERROR_ALREADY_EXISTS (183).
        // TRUNCATE_EXISTING If the specified file does not exist, the function fails and the last-error code is set to ERROR_FILE_NOT_FOUND (2).
        if (mode.contains('r')) {
            create = OPEN_EXISTING;
            access = GENERIC_READ;
            if (mode.contains('+')) {
                access |= GENERIC_WRITE;
            }
        } else if (mode.contains('a')) {
            create = CREATE_ALWAYS;
            access = GENERIC_WRITE;
            if (mode.contains('+')) {
                access |= GENERIC_READ;
            }
        } else if (mode.contains('w')) {
            create = CREATE_ALWAYS;
            access = GENERIC_WRITE;
            if (mode.contains('+')) {
                access |= GENERIC_READ;
            }
        }
        HANDLE h = CreateFile(
            path.c_str(),
            access,
            0,
            nullptr,
            create,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );
        return Handle{h};
    }

    static FORCEINLINE auto seek(handle_t fd, std::size_t size) noexcept->std::ptrdiff_t {
        auto low32 = static_cast<LONG>(size);
        // Move the file pointer to the 100th byte from the beginning
        DWORD dwPtr = SetFilePointer(
            fd,                     // Handle to the file
            low32,        // Distance to move (low-order 32 bits)
            nullptr, // Pointer to high-order 32 bits (NULL if distance < 2GB)
            FILE_BEGIN       // Starting point (FILE_BEGIN, FILE_CURRENT, or FILE_END)
        );
        if (dwPtr == INVALID_SET_FILE_POINTER) {
            // printf("Seek failed (Error %lu)\n", GetLastError());
        } else {
            // printf("File pointer moved to position: %lu\n", dwPtr);
        }
        return static_cast<std::ptrdiff_t>(dwPtr);
    }

    // TODO: Figure out how to determine if a handle is writeable or readable. Maybe use GetFileType and GetHandleInformation?
    // static FORCEINLINE auto handle_info(handle_t fd) {
    //     DWORD flags = 0;
    //     GetHandleInformation(handle_t fd, &flags);
    // }
    // typedef struct _BY_HANDLE_FILE_INFORMATION { // bhfi  
    //     DWORD    dwFileAttributes; 
    //     FILETIME ftCreationTime; 
    //     FILETIME ftLastAccessTime; 
    //     FILETIME ftLastWriteTime; 
    //     DWORD    dwVolumeSerialNumber; 
    //     DWORD    nFileSizeHigh; 
    //     DWORD    nFileSizeLow; 
    //     DWORD    nNumberOfLinks; 
    //     DWORD    nFileIndexHigh; 
    //     DWORD    nFileIndexLow; 
    // } BY_HANDLE_FILE_INFORMATION;
}