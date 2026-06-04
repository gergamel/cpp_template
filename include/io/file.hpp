#pragma once
#include <concepts>

#if defined(_MSC_VER)
    #define NOMINMAX
    #include <windows.h>
    #include <handleapi.h>
    #include <winnt.h>
    #include "file_windows.hpp"
    using io::impl::handle_t;
    // using impl::handle_t = HANDLE;
    // static constexpr const impl::handle_t INVALID_HANDLE_VALUE = nullptr; // TODO: Is this right?
#elif defined(__linux__)
    #include "file_linux.hpp"
    using io::impl::handle_t;
#   if !defined(FORCEINLINE)
#       define FORCEINLINE [[gnu::always_inline]] inline
#   endif
#endif

namespace io {

template<typename T>
concept FileHandleImpl = requires(T v, const void* wbuf, void* rbuf, std::size_t size) {
    std::is_nothrow_default_constructible<T>::value;
    std::is_nothrow_move_constructible<T>::value;
    std::is_nothrow_move_assignable<T>::value;
    std::is_nothrow_constructible<T,T&>::value;
    std::is_nothrow_constructible<T,T&&>::value;
    !std::is_nothrow_constructible<T,const T&>::value;
    !std::is_nothrow_constructible<T,const T&&>::value;
    !std::is_nothrow_copy_constructible<T>::value;
    !std::is_nothrow_copy_assignable<T>::value;
    v.open();
    v.close();
    { v.valid() } -> std::convertible_to<bool>;
    { v.readable() } -> std::convertible_to<bool>;
    { v.writeable() } -> std::convertible_to<bool>;
    { v.write(wbuf,size) }-> std::convertible_to<ptrdiff_t>;
    { v.read(rbuf,size) }-> std::convertible_to<ptrdiff_t>;
};

class File {
private:
    impl::Handle _f;
public:
    /******************************************************************************/
    // Default Construction
    File() = default;
    File(impl::Handle& hdl) noexcept : _f(std::move(hdl)) {}
    // Moveablity
    File(File& x) noexcept : _f(x._f) { x._f = impl::invalid_handle; }
    File(File&& x) noexcept : _f(x._f) { x._f = impl::invalid_handle; }
    auto operator=(File&& x) noexcept -> File& {
        if (this != &x) {
            if (_f.valid()) _f.close(); // Close existing
            _f = std::move(x._f);
            x._f = impl::invalid_handle;
        }
        return *this;
    }
    // Copyability (none allowed)
    constexpr File(const File& x) noexcept = delete;
    constexpr File(const File&& x) noexcept = delete;
    auto operator=(const File& x) noexcept = delete;
    // Destructor
    ~File() { _f.close(); }
    /******************************************************************************/
    [[nodiscard]] auto valid() const noexcept -> bool {
        return _f.valid();
    }
    [[nodiscard]] auto writeable() const noexcept -> bool {
        return _f.writeable();
    }
    [[nodiscard]] auto readable() const noexcept -> bool {
        return _f.readable();
    }
    [[nodiscard]] auto poll_read(int timeout_ms=0) noexcept -> bool {
        return _f.poll_read(timeout_ms);
    }
    // STATICFORCEINLINE auto Serial(const char *path, int baudrate)->File {
    //     return File(impl::open_port(path,baudrate));
    // }
    // STATICFORCEINLINE auto Pipe(const char* path,bool open_existing)->File {
    //     return File(impl::open_pipe(path,open_existing));
    // }
    // STATICFORCEINLINE auto File(const char* path) {
    //     return File(impl::open_pipe(path,open_existing))
    // }
    /******************************************************************************/
    FORCEINLINE auto read(char* buf, std::size_t size) noexcept->std::ptrdiff_t {
        return _f.read(buf,size);
    }
    // template <typename T, std::size_t N>
    // FORCEINLINE auto write(const T (&buf)[N]) noexcept->std::ptrdiff_t {
    //     return _f.write(reinterpret_cast<const void*>(&buf[0]),N);
    // }
    template <typename T>
    FORCEINLINE auto write(const T* buf, std::size_t size) noexcept->std::ptrdiff_t {
        return _f.write(reinterpret_cast<const void*>(buf),size);
    }
    FORCEINLINE auto write(const std::string& s) noexcept->std::ptrdiff_t {
        auto buf = s.c_str();
        auto size = s.size();
        // if (buf[size-1]==0) { size--; } // If the string ends with a null terminator, don't write it
        return _f.write(buf,size);
    }
    /******************************************************************************/
    auto readline() -> std::string{
        std::string line;
        line.reserve(256);
        char c;
        while (true) {
            ptrdiff_t n = read(&c,1);
            if (n<0) {
                // fprintf(stderr,"Error [%d] reading %s\n", errno, strerror(errno));
                break;
            }
            if (n==0) continue; // timeout
            if (c=='\n') break;
            if (c=='\r') continue;
            line += c;
        }
        return line;
    }
};

static inline auto open_file(const std::string& path,const std::string& mode) noexcept->File{
    auto h = io::impl::open_file(path,mode);
    return {h};
}

// static inline auto open_serial(const std::string& path,int baud) noexcept->File{
//     auto h = io::impl::open_serial(path,baud);
//     return {h};
// }

// class File {
//   private:
//     impl::handle_t  _f;
//     File(impl::handle_t f) noexcept : _f(f) {}
//   public:
//     /******************************************************************************/
//     // Default Construction
//     File() noexcept : _f(invalid_handle) {}
//     // Moveablity
//     constexpr File(File& x) noexcept : _f(x._f) { x._f = invalid_handle; }
//     constexpr File(File&& x) noexcept : _f(x._f) { x._f = invalid_handle; }
//     auto operator=(File&& x) noexcept -> File& {
//         if (this != &x) {
//             if (_f != invalid_handle) impl::close(_f); // Close existing
//             _f = x._f;
//             x._f = INVALID_HANDLE_VALUE;
//         }
//         return *this;
//     }
//     // Copyability (none allowed)
//     constexpr File(const File& x) noexcept = delete;
//     constexpr File(const File&& x) noexcept = delete;
//     auto operator=(const File& x) noexcept = delete;
//     // Destructor
//     ~File() { impl::close(_f); }
//     /******************************************************************************/
//     static FORCEINLINE auto Serial(const char *path, int baudrate)->File {
//         return File(impl::open_port(path,baudrate));
//     }
//     static FORCEINLINE auto Pipe(const char* path,bool open_existing)->File {
//         return File(impl::open_pipe(path,open_existing));
//     }
//     // static constexpr FORCEINLINE auto File(const char* path) {
//     //     return File(impl::open_pipe(path,open_existing))
//     // }
//     /******************************************************************************/
//     FORCEINLINE auto read(char* buf, std::size_t size) noexcept->std::ptrdiff_t {
//         return impl::read(_f,buf,size);
//     }
//     template <typename T, std::size_t N>
//     FORCEINLINE auto write(const T (&buf)[N]) noexcept->std::ptrdiff_t {
//         return impl::write(_f,reinterpret_cast<const void*>(&buf[0]),N);
//     }
//     template <typename T>
//     FORCEINLINE auto write(const T* buf, std::size_t size) noexcept->std::ptrdiff_t {
//         return impl::write(_f,reinterpret_cast<const void*>(buf),size);
//     }
//     /******************************************************************************/
// };

}
