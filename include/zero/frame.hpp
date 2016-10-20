#ifndef __ZERO_FRAME_HPP__
#define __ZERO_FRAME_HPP__ 1
#include <cstring>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <zmq.h>
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
    explicit frame() { zmq_msg_init(&msg); }
    explicit frame( std::size_t size ) { zmq_msg_init_size(&msg, size); }
    explicit frame( const std::string &s ) {
      auto size = s.size();
      zmq_msg_init_size(&msg, size);
      std::memcpy(data(), &s[0], size);
    }

    ~frame() { zmq_msg_close(&msg); }

    operator zmq_msg_t&() { return msg; }
    operator const zmq_msg_t&() const { return msg; }

    operator zmq_msg_t*() { return &msg; }
    operator const zmq_msg_t*() const { return &msg; }

    int init() { return zmq_msg_init(&msg); }
    int init_size(size_t size) { return zmq_msg_init_size(&msg, size); }
    int init_data(void *data, size_t size, zmq_free_fn *ffn, void *hint)
    { return zmq_msg_init_data(&msg, data, size, ffn, hint); }

    int init_value(std::uint32_t); /// init frame with a uint32_t value

    std::size_t size() const { return zmq_msg_size(const_cast<zmq_msg_t*>(&msg)); }
    Uint8 *data() { return (Uint8*) zmq_msg_data(&msg); }
    Uint8 *data() const { return (Uint8*) zmq_msg_data(const_cast<zmq_msg_t*>(&msg)); }
    Uint8 *data_end() { return data() + size(); }
    Uint8 *data_end() const { return data() + size(); }

    std::string as_string() const { return std::string(data(), data_end()); }
    std::uint32_t as_uint32() const;

    int copy(frame & dest) { return zmq_msg_copy(&dest.msg, &msg); }
    int move(frame & dest) { return zmq_msg_move(&dest.msg, &msg); }

    int more() { return zmq_msg_more(&msg); }

    int get(int property) const { return zmq_msg_get(const_cast<zmq_msg_t*>(&msg), property); }
    int set(int property, int optval) { return zmq_msg_set(&msg, property, optval); }

    /**
     *  The caller shall not modify or free the returned value,
     *  which shall be owned by the message. The encoding of the property and value
     *  shall be UTF8.
     */
    //const char *gets(const char *property) { return zmq_msg_gets(&msg, property); }

    int send(void *s, int flags) { return zmq_msg_send(&msg, s, flags); }
    int recv(void *s, int flags) { return zmq_msg_recv(&msg, s, flags); }

#if 0
    int set_routing_id(std::uint32_t id) { return zmq_msg_set_routing_id(&msg, id); }
    std::uint32_t routing_id() const { return zmq_msg_routing_id(const_cast<zmq_msg_t*>(&msg)); }
#endif

  private:
    zmq_msg_t msg;

    // Disable copy from lvalue.
    frame(const frame&) = delete;
    frame& operator=(const frame&) = delete;
  };
}
#endif//__ZERO_FRAME_HPP__
