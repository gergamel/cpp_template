// #include <stdint.h>
#include <expected>
#include <type_traits>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <gsl/span>
#include <gsl/assert>
#include <pcap.h>

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#ifdef NDEBUG
    #define ASSUME(expr) [[assume(expr)]]
#else
    #define ASSUME(expr) (LIKELY(expr) ? static_cast<void>(0) : gsl::details::terminate())
#endif

#define  GET(_NAME) [[nodiscard]] auto _NAME() noexcept
#define CGET(_NAME) [[nodiscard]] auto _NAME() const noexcept

/* Simple, non-owning view into a buffer of fixed length */
template <class ElementType>
class View {
public:
    using element_type = ElementType;
    using value_type = std::remove_cv_t<ElementType>;
    using size_type = std::size_t;
    using pointer = element_type*;
    using const_pointer = const element_type*;
    using reference = element_type&;
    using const_reference = const element_type&;
    using difference_type = std::ptrdiff_t;
    /******************************************************************************/
    inline constexpr View() = default;
    inline constexpr View(pointer ptr, size_type size) noexcept : data_(ptr),size_(size) {}
    template <typename T>
    inline constexpr View(T* ptr, size_type size) noexcept :
        data_(reinterpret_cast<element_type*>(ptr)),
        size_(size)
    {}
    // Moveablity
    inline constexpr View(View& x)  noexcept : data_(x.data_),size_(x.size_) {}
    inline constexpr View(View&& x) noexcept : data_(x.data_),size_(x.size_) {}
    inline constexpr auto operator=(View&& x) noexcept -> View& {
        if (this != &x) {
            data_ = x.data_;
            size_ = x.size_;
        }
        return *this;
    }
    // Copyability (none allowed)
    inline constexpr View(const View& x)  noexcept : data_(x.data_),size_(x.size_) {}
    inline constexpr View(const View&& x) noexcept : data_(x.data_),size_(x.size_) {}
    inline constexpr auto operator=(const View& x)  noexcept -> View& {
        if (this != &x) {
            data_ = x.data_;
            size_ = x.size_;
        }
        return *this;
    }
    // Destructor
    constexpr ~View() = default;
    /******************************************************************************/
    [[nodiscard]] inline constexpr auto size() const noexcept -> size_type { return size_; }
    [[nodiscard]] inline constexpr auto size_bytes() const noexcept -> size_type { return size_ * sizeof(element_type); }
    [[nodiscard]] inline constexpr auto empty() const noexcept -> bool { return size_==0; }
    [[nodiscard]] inline constexpr auto data() const noexcept -> pointer { return data_; }
    inline constexpr auto operator[](const size_type idx) const noexcept -> const_reference {
        ASSUME(idx < size_);
        return data_[idx];
    }
    inline constexpr auto operator[](const size_type idx) noexcept -> reference {
        ASSUME(idx < size_);
        return data_[idx];
    }
private:
    pointer     data_ = nullptr;  
    size_type   size_ = 0;
};

using BytesView = View<const uint8_t>;

template <class ElementType>
class Slice {
public:
    // constants and types
    using element_type = ElementType;
    using value_type = std::remove_cv_t<ElementType>;
    using size_type = std::size_t;
    using pointer = element_type*;
    using const_pointer = const element_type*;
    using reference = element_type&;
    using const_reference = const element_type&;
    using difference_type = std::ptrdiff_t;
    // using iterator = details::span_iterator<ElementType>;
    // using reverse_iterator = std::reverse_iterator<iterator>;
private:
    BytesView   data_;
    pointer     ofs_;
};


namespace net {
    enum class parse_error {
        not_present,
        overflow
    };
    namespace vrt {
        struct Header {
          private:
            BytesView b_;
          public:
            Header(BytesView bv) : b_{std::move(bv)} {
                ASSUME(b_.size()==4);
            }
        };
        class HeaderView {
            /* 0000   18 e4 08 bf|00 00 00 00|00 6a 62 1e 00 00 00 00|
             * 0010   67 bf 58 77|00 00 00 9a 96 7e 40 60|39 c0 31 23 ...
             * 
             * VITA 49 radio transport protocol
             *   VRT header: [18 e4 08 bf] 0x18e408bf
             *       0001 .... .... .... = Packet type: IF data packet with stream ID (1)
             *       .... 1... .... .... = Class ID included: True
             *       .... .0.. .... .... = Trailer included: False
             *       .... .... 11.. .... = Integer timestamp type: Other (3)
             *       .... .... ..10 .... = Fractional timestamp type: Real time (picoseconds) timestamp (2)
             *       .... .... .... 0100 = Sequence number: 4
             *       Length: 2239
             *   Stream ID:[00 00 00 00] 0x00000000
             *   Class ID: [00 6a 62 1e 00 00 00 00] 0x006a621e00000000
             *       Class ID Organizationally Unique ID: 0x6a621e
             *       Class ID Information Class Code: 0
             *       Class ID Packet Class Code: 0
             *   Integer timestamp: [67 bf 58 77] 1740593271
             *   Fractional timestamp (picoseconds): [00 00 00 9a 96 7e 40 60] 663949820000
             *   Data [truncated]: 39c03123...
             */
          private:
            BytesView b_;
          public:
            HeaderView(BytesView bv) : b_{std::move(bv)} {
                ASSUME(b_.size()>=30);
            }
            auto payload() -> BytesView {
                // TODO: Different header flags change the payload offset
                return {&b_[30],b_.size()-30};
            }
            CGET(hdr)       -> Header   { return Header({&b_[0],4}); }
            CGET(stream_id) -> uint32_t { return ntohl(*reinterpret_cast<const uint32_t*>(&b_[4])); }
            CGET(class_id)  -> uint64_t { return ntohl(*reinterpret_cast<const uint64_t*>(&b_[8])); }
            CGET(tv_secs)   -> uint32_t { return ntohl(*reinterpret_cast<const uint32_t*>(&b_[16])); }
            CGET(tv_nsecs)  -> uint64_t { return ntohl(*reinterpret_cast<const uint64_t*>(&b_[20])); }
        };
    }
    namespace udp {
        class HeaderView {
          private:
            BytesView b_;
          public:
            HeaderView(BytesView bv) : b_{std::move(bv)} {
                ASSUME(b_.size()>=8);
            }
            CGET(src) -> uint16_t  { return ntohs(*reinterpret_cast<const uint16_t*>(&b_[0])); }
            CGET(dst) -> uint16_t  { return ntohs(*reinterpret_cast<const uint16_t*>(&b_[2])); }
            CGET(len) -> uint16_t  { return ntohs(*reinterpret_cast<const uint16_t*>(&b_[4])); }
            CGET(chk) -> uint16_t  { return ntohs(*reinterpret_cast<const uint16_t*>(&b_[6])); }
            CGET(payload) -> BytesView {
                return {&b_[8],b_.size()-8};
            }
            // CGET(has_vrt)  -> bool { // TODO ; }
        };
    }
    namespace ipv4 {
        using ver_ihl_t  = uint8_t;
        using tos_t      = uint8_t;
        using len_t      = uint16_t;
        using ident_t    = uint16_t;
        using flg_frag_t = uint16_t;
        using ttl_t      = uint8_t;
        using protocol_t = uint8_t;
        using hdr_chk_t  = uint16_t;
        using addr_t     = uint32_t;
        class HeaderView {
          private:
            BytesView b_;
          public:
            HeaderView(BytesView bv) : b_{std::move(bv)} {
                ASSUME(b_.size()>=20);
            }
            CGET(ver_ihl)  -> ver_ihl_t  { return b_[0]; }
            CGET(tos)      -> tos_t      { return b_[1]; }
            CGET(len)      -> len_t      { return ntohs(*reinterpret_cast<const len_t*>(&b_[2])); }
            CGET(ident)    -> ident_t    { return ntohs(*reinterpret_cast<const ident_t*>(&b_[4])); } 
            CGET(flg_frag) -> flg_frag_t { return ntohs(*reinterpret_cast<const flg_frag_t*>(&b_[6])); } 
            CGET(ttl)      -> ttl_t      { return b_[8]; }
            CGET(protocol) -> protocol_t { return b_[9]; }
            CGET(hdr_chk)  -> hdr_chk_t  { return ntohs(*reinterpret_cast<const hdr_chk_t*>(&b_[10]));}
            CGET(src)      -> addr_t     { return ntohs(*reinterpret_cast<const addr_t*>(&b_[12])); }
            CGET(dst)      -> addr_t     { return ntohs(*reinterpret_cast<const addr_t*>(&b_[16])); }
            CGET(payload)  -> BytesView  {
                return {&b_[20],b_.size()-20};
            }
            CGET(has_udp)  -> bool       { return (protocol()==17); }
            CGET(udp)      -> std::expected<udp::HeaderView,net::parse_error> {
                if (protocol()==17) {
                    return net::udp::HeaderView(payload());
                }
                return std::unexpected(parse_error::not_present);
            }
        };
        // using opts_t  = uint16_t;
    };
    namespace eth {
        using ethtype_t = uint16_t;
        struct addr_t {
            std::array<uint8_t,6> a;
        };
        class HeaderView {
          private:
            BytesView b_;
          public:
            HeaderView(BytesView b) : b_{std::move(b)} {}
            [[nodiscard]] constexpr auto bv() noexcept -> BytesView { return b_; }
            CGET(src)      -> addr_t    { return {b_[0],b_[1],b_[2],b_[3],b_[4],b_[5]}; }
            CGET(dst)      -> addr_t    { return {b_[6],b_[7],b_[8],b_[9],b_[10],b_[11]}; }
            CGET(ethtype)  -> ethtype_t { return ntohs(*reinterpret_cast<const ethtype_t*>(&b_[12])); }CGET(payload) -> BytesView {
                return {&b_[14],b_.size()-14};
            }
            CGET(has_ipv4) -> bool      { return (ethtype()==0x0800); }
            CGET(ipv4)     -> std::expected<ipv4::HeaderView,net::parse_error> {
                if (ethtype()==0x0800) {
                    return net::ipv4::HeaderView(payload());
                }
                return std::unexpected(parse_error::not_present);
            }
        };
    };
}

template <>
struct fmt::formatter<net::eth::addr_t,char>{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }  
    template <typename FmtContext>
    constexpr auto format (net::eth::addr_t const& addr, FmtContext& ctx) const {
        return format_to(ctx.out(),"{}", fmt::join(addr.a,":"));;
    }
};

int main() {
    char errbuf[PCAP_ERRBUF_SIZE];
    
    // Open the pcap file for reading
    pcap_t* handle = pcap_open_offline("example_pcaps/Example2_100Msps_12bits.pcapng", errbuf);
    if (handle == nullptr) {
        fmt::println(stderr,"Error opening PCAP file: {}",errbuf);
        return 1;
    }

    struct pcap_pkthdr* header;
    const u_char* packet;
    int packet_count = 0;

    // Read packets sequentially until the end of the file
    while (pcap_next_ex(handle, &header, &packet) == 1) {
        packet_count++;
        fmt::println("Packet #{} | Captured length: {} B | Total length: {} B | Timestamp: {}.{}",
                  packet_count,
                  header->caplen,
                  header->len,
                  header->ts.tv_sec,header->ts.tv_usec);
        BytesView b{packet,header->len};
        net::eth::HeaderView eth_pkt{b};
        if (const auto ip_pkt = eth_pkt.ipv4(); ip_pkt.has_value()) {
            if (const auto udp_pkt = ip_pkt->udp(); udp_pkt.has_value()) {
                auto vrt_pkt = net::vrt::HeaderView(udp_pkt->payload());

                fmt::println("- ETH<etype={:04x},src={},dst={}>",
                    eth_pkt.ethtype(),
                    eth_pkt.src(),
                    eth_pkt.dst()
                );
                fmt::println("  - IPv4<proto={},src={},dst={}>",
                    ip_pkt->protocol(),
                    ip_pkt->src(),
                    ip_pkt->dst()
                );
                fmt::println("    - UDP<src={},dst={},len={},chk={:04x}>",
                    udp_pkt->src(),
                    udp_pkt->dst(),
                    udp_pkt->len(),
                    udp_pkt->chk()
                );
                fmt::println("      - VRT<stream_id={:04x},class_id={:08x},tv_secs={:04x},tv_nsecs={:08x}>",
                    vrt_pkt.stream_id(),
                    vrt_pkt.class_id(),
                    vrt_pkt.tv_secs(),
                    vrt_pkt.tv_nsecs()
                );
            }
        }
    }

    // Free the file handle resources
    pcap_close(handle);
    return 0;
}
