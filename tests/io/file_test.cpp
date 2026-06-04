#include <gtest/gtest.h>
#include "io/file.hpp"

TEST(filedesc, construction) {
    using T = io::File;
    // Construction
    static_assert(std::is_nothrow_default_constructible<T>::value,      "Shall be no-throw default");
    // static_assert(std::is_nothrow_constructible<T, const std::string&>::value,      "Shall be no-throw constructable from const string");
    // Moveablity
    static_assert(std::is_nothrow_move_constructible<T>::value,         "Shall be no-throw move constructable");
    static_assert(std::is_nothrow_move_assignable<T>::value,            "Shall be no-throw move assignable");
    static_assert(std::is_nothrow_constructible<T,T&>::value,           "Shall be no-throw move constructable from ref");
    static_assert(std::is_nothrow_constructible<T,T&&>::value,          "Shall be no-throw move constructable from rref");
    // Copyability (owning instances should not be copyable)
    static_assert(!std::is_nothrow_constructible<T,const T&>::value,    "Shall not be copy constructable");
    static_assert(!std::is_nothrow_constructible<T,const T&&>::value,   "Shall not be copy copy assignable");
    static_assert(!std::is_nothrow_copy_constructible<T>::value,        "Shall not be copy constructable");
    static_assert(!std::is_nothrow_copy_assignable<T>::value,           "Shall not be copy assignable");
}

TEST(filedesc, from_fd_create_wronly) {
    auto f = io::open_file("x.tmp","w");
    EXPECT_TRUE(f.valid()) << "File should be valid";
    EXPECT_TRUE(f.writeable()) << "File created with WRONLY flag should be writeable";
    EXPECT_FALSE(f.readable()) << "File created with WRONLY flag should NOT be readable";
    auto wlen = f.write("Test\n",5);
    EXPECT_EQ(wlen,5);
}

TEST(filedesc, from_fd_create_rdonly) {
    char buf[32] = {0};
    auto f = io::open_file("x.tmp","r");
    EXPECT_TRUE(f.valid()) << "File should be valid";
    EXPECT_FALSE(f.writeable()) << "File created with RDONLY flag should NOT be writeable";
    EXPECT_TRUE(f.readable()) << "File created with RDONLY flag should be readable";
    auto rlen = f.read(buf,sizeof(buf));
    EXPECT_EQ(rlen,5);
    EXPECT_EQ(strcmp(buf,"Test\n"),0);
}

TEST(filedesc, raii) {
    {
        // Create an fd at this scope, then wrap it in a File at a lower scope.
        // The File should take ownership of the fd, closing it automatically
        // when it goes out of scope.
        auto h = io::impl::open_file("x.tmp","w");
        {
            auto f = io::File(h);
            EXPECT_TRUE(f.valid()) << "File should be valid";
            EXPECT_TRUE(f.writeable()) << "File created with WRONLY flag should be writeable";
            EXPECT_FALSE(f.readable()) << "File created with WRONLY flag should NOT be readable";
        }
        // The file should have been closed by the RAII f leaving scope....
        // int e = fcntl(fd, F_GETFL);
        // ...which means fcntl should fail...
        EXPECT_FALSE(h.valid()) << "Original fd should have been closed by RAII File, so fcntl should have failed";

        // Similarly, trying to pass that fd into a new File should result in...
        auto f = io::File(h);
        EXPECT_FALSE(f.valid());
        EXPECT_FALSE(f.writeable());
        EXPECT_FALSE(f.readable());
    }
}

auto get_file(const std::string& s) {
    auto result = io::open_file(s,"r+");
    return result;
}

TEST(filedesc, move_return) {
    {
        auto f1 = get_file("x.tmp");
        EXPECT_TRUE(f1.valid())     << "Should be valid";
        EXPECT_TRUE(f1.writeable()) << "Should be writeable";
        EXPECT_TRUE(f1.readable())  << "Should be readable";

        io::File f2(f1);
        EXPECT_TRUE(f2.valid()) << "Move constructed instance should be valid";
        EXPECT_FALSE(f1.valid())        << "Original used for move construction should now be invalid";
        // auto wlen = f.write("Test\n",5);
        // EXPECT_EQ(wlen,5);
    }
}

TEST(filedesc, move_construct) {
    {
        auto f1= io::open_file("x.tmp","w+");
        EXPECT_TRUE(f1.valid());
        EXPECT_TRUE(f1.writeable());
        EXPECT_TRUE(f1.readable());

        io::File f2(f1);
        EXPECT_FALSE(f1.valid());
        EXPECT_TRUE(f2.valid());
        // auto wlen = f.write("Test\n",5);
        // EXPECT_EQ(wlen,5);
    }
    {
        auto f1= io::open_file("x.tmp","w+");
        EXPECT_TRUE(f1.valid());
        EXPECT_TRUE(f1.writeable());
        EXPECT_TRUE(f1.readable());

        io::File f2(std::move(f1));
        EXPECT_FALSE(f1.valid());
        EXPECT_TRUE(f2.valid());
        // auto wlen = f.write("Test\n",5);
        // EXPECT_EQ(wlen,5);
    }
}

TEST(filedesc, move_assign) {
    {
        auto f1= io::open_file("x.tmp","w+");
        EXPECT_TRUE(f1.valid());
        EXPECT_TRUE(f1.writeable());
        EXPECT_TRUE(f1.readable());

        io::File f2 = f1;
        EXPECT_FALSE(f1.valid());
        EXPECT_TRUE(f2.valid());
        // auto wlen = f.write("Test\n",5);
        // EXPECT_EQ(wlen,5);
    }
    {
        auto f1= io::open_file("x.tmp","w+");
        EXPECT_TRUE(f1.valid());
        EXPECT_TRUE(f1.writeable());
        EXPECT_TRUE(f1.readable());

        io::File f2 = std::move(f1);
        EXPECT_FALSE(f1.valid());
        EXPECT_TRUE(f2.valid());
        // auto wlen = f.write("Test\n",5);
        // EXPECT_EQ(wlen,5);
    }
}

// TEST(filereader, construction) {
//     using namespace std;
//     using T = os::FileReader<char>;
//     static_assert(is_nothrow_default_constructible<T>::value,                   "Shall be no-throw default constructable");
//     static_assert(is_nothrow_constructible<T, const char*>::value,              "Shall be no-throw constructable from static c-str");
//     static_assert(is_nothrow_constructible<T, const std::string&>::value,       "Shall be no-throw constructable from const string");
//     static_assert(is_nothrow_constructible<T, const std::string_view&>::value,  "Shall be no-throw constructable from string_view");
//     static_assert(is_nothrow_constructible<T, io::File&>::value,            "Shall be no-throw move constructable from io::File");
//     static_assert(is_nothrow_constructible<T, io::File&&>::value,           "Shall be no-throw move constructable from io::File");
//     // Moveablity   
//     static_assert(is_nothrow_move_constructible<T>::value,          "Shall be no-throw move constructable");
//     static_assert(is_nothrow_move_assignable<T>::value,             "Shall be no-throw move assignable");
//     static_assert(is_nothrow_constructible<T,T&>::value,            "Shall be no-throw move constructable from ref");
//     static_assert(is_nothrow_constructible<T,T&&>::value,           "Shall be no-throw move constructable from rref");
//     // Copyability
//     static_assert(!is_nothrow_constructible<T,const T&>::value,     "Shall not be copy constructable");
//     static_assert(!is_nothrow_constructible<T,const T&&>::value,    "Shall not be copy copy assignable");
//     static_assert(!is_nothrow_copy_constructible<T>::value,         "Shall not be copy constructable");
//     static_assert(!is_nothrow_copy_assignable<T>::value,            "Shall not be copy assignable");
// }

// TEST(filereader, from_fd) {
//     using rdr_t = os::FileReader<char>;
//     {
//         int fd = open("x.tmp",O_RDONLY);
//         rdr_t r(fd);
//         EXPECT_TRUE(r.valid());
//         EXPECT_FALSE(r.writeable());
//         EXPECT_TRUE(r.readable());

//         // size_t read_available(size_t max_size)
//         // size_t write_available(size_t max_size)

//     }
// }

// // TEST(filedesc, raii) {
// //     {
// //         int fd = open("x.tmp",O_CREAT|O_WRONLY|O_TRUNC,S_IRWXU|S_IRGRP|S_IROTH);
// //         {
// //             auto f = io::open_file(fd);
// //             EXPECT_TRUE(f.valid());
// //             EXPECT_TRUE(f.writeable());
// //             EXPECT_FALSE(f.readable());
// //         }
// //         // The file should have been closed by the RAII f leaving scope....
// //         int e = fcntl(fd, F_GETFL);
// //         // ...which means fcntl should fail...
// //         EXPECT_EQ(e,-1);

// //         // Similarly, trying to pass that fd into a new File should result in...
// //         auto f = io::open_file(fd);
// //         EXPECT_FALSE(f.valid());
// //         EXPECT_FALSE(f.writeable());
// //         EXPECT_FALSE(f.readable());
// //     }
// // }

// // TEST(filedesc, move_return) {
// //     {
// //         auto f1= io::open_file = get_file("x.tmp");
// //         EXPECT_TRUE(f1.valid());
// //         EXPECT_TRUE(f1.writeable());
// //         EXPECT_TRUE(f1.readable());

// //         io::File f2(f1);
// //         EXPECT_FALSE(f1.valid());
// //         EXPECT_TRUE(f2.valid());
// //         // auto wlen = f.write("Test\n",5);
// //         // EXPECT_EQ(wlen,5);
// //     }
// // }

// // TEST(filedesc, move_construct) {
// //     {
// //         auto f1= io::open_file("x.tmp",O_CREAT|O_RDWR);
// //         EXPECT_TRUE(f1.valid());
// //         EXPECT_TRUE(f1.writeable());
// //         EXPECT_TRUE(f1.readable());

// //         io::File f2(f1);
// //         EXPECT_FALSE(f1.valid());
// //         EXPECT_TRUE(f2.valid());
// //         // auto wlen = f.write("Test\n",5);
// //         // EXPECT_EQ(wlen,5);
// //     }
// //     {
// //         auto f1= io::open_file("x.tmp",O_CREAT|O_RDWR);
// //         EXPECT_TRUE(f1.valid());
// //         EXPECT_TRUE(f1.writeable());
// //         EXPECT_TRUE(f1.readable());

// //         io::File f2(std::move(f1));
// //         EXPECT_FALSE(f1.valid());
// //         EXPECT_TRUE(f2.valid());
// //         // auto wlen = f.write("Test\n",5);
// //         // EXPECT_EQ(wlen,5);
// //     }
// // }

// // TEST(filedesc, move_assign) {
// //     {
// //         auto f1= io::open_file("x.tmp",O_CREAT|O_RDWR);
// //         EXPECT_TRUE(f1.valid());
// //         EXPECT_TRUE(f1.writeable());
// //         EXPECT_TRUE(f1.readable());

// //         io::File f2 = f1;
// //         EXPECT_FALSE(f1.valid());
// //         EXPECT_TRUE(f2.valid());
// //         // auto wlen = f.write("Test\n",5);
// //         // EXPECT_EQ(wlen,5);
// //     }
// //     {
// //         auto f1= io::open_file("x.tmp",O_CREAT|O_RDWR);
// //         EXPECT_TRUE(f1.valid());
// //         EXPECT_TRUE(f1.writeable());
// //         EXPECT_TRUE(f1.readable());

// //         io::File f2 = std::move(f1);
// //         EXPECT_FALSE(f1.valid());
// //         EXPECT_TRUE(f2.valid());
// //         // auto wlen = f.write("Test\n",5);
// //         // EXPECT_EQ(wlen,5);
// //     }
// // }