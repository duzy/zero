#ifndef __ZERO_FRAME_HPP__
#define __ZERO_FRAME_HPP__ 1
#include <cstring>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace zero
{
  using Int8 = std::int8_t;
  using Int16 = std::int16_t;
  using Int32 = std::int32_t;
  using Int64 = std::int64_t;
  using Uint8 = std::uint8_t;
  using Uint16 = std::uint16_t;
  using Uint32 = std::uint32_t;
  using Uint64 = std::uint64_t;
  using Octets = std::vector<std::uint8_t>;
  using Chunk = std::vector<std::uint8_t>;
  struct TinyString : std::string {
    using std::string::string;
    using std::string::operator=;
  };
  struct ShortString : std::string {
    using std::string::string;
    using std::string::operator=;
  };
  struct LongString : std::string {
    using std::string::string;
    using std::string::operator=;
  };
  
  struct frame
  {
    typedef union underlying_t {unsigned char _ [64]; void *p; } underlying_t;
    typedef void (data_free_fn) (void *data, void *hint);
      
    explicit frame();
    explicit frame( std::size_t size );
    explicit frame( const std::string &s );

    ~frame();

    int init();
    int init_size(size_t size);
    int init_data(void *data, size_t size, data_free_fn *ffn = nullptr, void *hint = nullptr);
    int init_value(std::uint32_t); /// init frame with a uint32_t value

    std::size_t size() const;
    Uint8 *data();
    Uint8 *data() const;
    Uint8 *data_end();
    Uint8 *data_end() const;

    std::string as_string() const { return std::string(data(), data_end()); }
    std::uint32_t as_uint32() const;

    int copy(frame & dest);
    int move(frame & dest);

    int more();

    int get(int property) const;
    int set(int property, int optval);

    /**
     *  The caller shall not modify or free the returned value,
     *  which shall be owned by the message. The encoding of the property and value
     *  shall be UTF8.
     */
    //const char *gets(const char *property);

    int send(void *s, int flags = 0);
    int recv(void *s, int flags = 0);

#if 0
    int set_routing_id(std::uint32_t id);
    std::uint32_t routing_id() const;
#endif

  private:
    underlying_t underlying;

    // Disable copy from lvalue.
    frame(const frame&) = delete;
    frame& operator=(const frame&) = delete;
  };

  int errnum(void); // errno
  const char *strerror(int en);
  const char *strerror();
}//namespace zero
#endif//__ZERO_FRAME_HPP__
