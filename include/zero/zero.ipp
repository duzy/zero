/*-----*- c++ -*-----*/
#include <zero/frame.hpp>
#include <zero/message.hpp>
#include <zero/unique_socket.hpp>
#include <zero/poller.hpp>
namespace
{
  static const struct zmq_wrapper
  {
    zmq_wrapper() : the_context( zmq_ctx_new() ) {}
    ~zmq_wrapper() { zmq_ctx_term(the_context); }
    inline void *context() const { return the_context; }
  private:
    void * the_context;
  } zmq;

  static inline int socktype(void *sock)
  {
    int type = 0;
    std::size_t len = sizeof type;
    if (zmq_getsockopt(sock, ZMQ_TYPE, &type, &len) == 0)
      return type;
    return -1;
  }

  struct needle : private boost::noncopyable
  {
    explicit needle(Uint8 *P) : needle(P) {}
    explicit needle(frame &F) : needle(F.data()) {}

    unsigned put(Uint8 v)
    {
      assert(needle != NULL);
      *needle = v;
      needle++;
      return 1;
    }

    unsigned put(Uint16 v)
    {
      assert(needle != NULL);
      needle [0] = Uint8(((v >> 8)  & 0xFF));
      needle [1] = Uint8(((v)       & 0xFF)) ;
      needle += 2;
      return 2;
    }

    unsigned put(Uint32 v)
    {
      assert(needle != NULL);
      needle [0] = Uint8(((v >> 24) & 0xFF));
      needle [1] = Uint8(((v >> 16) & 0xFF));
      needle [2] = Uint8(((v >> 8)  & 0xFF));
      needle [3] = Uint8(((v)       & 0xFF));
      needle += 4;
      return 4;
    }

    unsigned put(Uint64 v)
    {
      assert(needle != NULL);
      needle [0] = Uint8(((v >> 56) & 0xFF));
      needle [1] = Uint8(((v >> 48) & 0xFF));
      needle [2] = Uint8(((v >> 40) & 0xFF));
      needle [3] = Uint8(((v >> 32) & 0xFF));
      needle [4] = Uint8(((v >> 24) & 0xFF));
      needle [5] = Uint8(((v >> 16) & 0xFF));
      needle [6] = Uint8(((v >> 8)  & 0xFF));
      needle [7] = Uint8(((v)       & 0xFF));
      needle += 8;
      return 8;
    }

    unsigned get(Uint8 & v)
    {
      assert(needle != NULL);
      v = *needle;
      needle++;
      return 1;
    }

    unsigned get(Uint16 & v)
    {
      assert(needle != NULL);
      v = ((Uint16)(needle [0]) << 8)
    + ((Uint16)(needle [1])) ;
      needle += 2;
      return 2;
    }

    unsigned get(Uint32 & v)
    {
      assert(needle != NULL);
      v = ((Uint32)(needle [0]) << 24)
    + ((Uint32)(needle [1]) << 16)
    + ((Uint32)(needle [2]) << 8)
    + ((Uint32)(needle [3]) ) ;
      needle += 4;
      return 4;
    }

    unsigned get(Uint64 & v)
    {
      assert(needle != NULL);
      v = ((Uint64)(needle [0]) << 56)
    + ((Uint64)(needle [1]) << 48)
    + ((Uint64)(needle [2]) << 40)
    + ((Uint64)(needle [3]) << 32)
    + ((Uint64)(needle [4]) << 24)
    + ((Uint64)(needle [5]) << 16)
    + ((Uint64)(needle [6]) << 8)
    +  (Uint64)(needle [7]) ;
      needle += 8;
      return 8;
    }

    unsigned put(Int8  v)  { return put(static_cast<Uint8> (v)); }
    unsigned put(Int16 v)  { return put(static_cast<Uint16>(v)); }
    unsigned put(Int32 v)  { return put(static_cast<Uint32>(v)); }
    unsigned put(Int64 v)  { return put(static_cast<Uint64>(v)); }
    unsigned get(Int8  &v) { return get(*reinterpret_cast<Uint8*> (&v)); }
    unsigned get(Int16 &v) { return get(*reinterpret_cast<Uint16*>(&v)); }
    unsigned get(Int32 &v) { return get(*reinterpret_cast<Uint32*>(&v)); }
    unsigned get(Int64 &v) { return get(*reinterpret_cast<Uint64*>(&v)); }

    template <class SizeType>
    unsigned put_string(const std::string & s)
    {
      assert(needle != NULL);
      std::size_t size = s.size();
      auto n = put(SizeType(size));
      std::memcpy(needle, &s[0], size);
      needle += size;
      return n + size;
    }

    template <class SizeType>
    unsigned get_string(std::string & s)
    {
      assert(needle != NULL);
      SizeType size;
      auto n = get(size);
      s.resize(size);
      std::memcpy(&s[0], needle, size);
      needle += size;
      return n + size;
    }

    unsigned put(const TinyString & s)    { return put_string<Uint8>(s); }
    unsigned put(const ShortString & s)   { return put_string<Uint16>(s); }
    unsigned put(const LongString & s)    { return put_string<Uint32>(s); }
    unsigned get(TinyString & s)          { return get_string<Uint8>(s); }
    unsigned get(ShortString & s)         { return get_string<Uint16>(s); }
    unsigned get(LongString & s)          { return get_string<Uint32>(s); }

    unsigned put(Uint8 *data, std::size_t size)
    {
      assert(needle != NULL);
      std::memcpy(needle, data, size);
      needle += size;
      return size;
    }

    unsigned get(Uint8 *data, std::size_t size)
    {
      assert(needle != NULL);
      std::memcpy(data, needle, size);
      needle += size;
      return size;
    }

    operator Uint8 *() const { return needle; }

    needle & operator =(Uint8 *p)   { needle  = p; return *this; }
    needle & operator+=(unsigned n) { needle += n; return *this; }
 
  private:
    Uint8 *needle;
  };

  struct frame_builder : private boost::noncopyable
  {
    explicit frame_builder() : needle(nullptr), stop(nullptr) {}
    explicit frame_builder(frame &F) : needle(F.data()), stop(needle + F.size()) {}
    explicit frame_builder(frame &F, unsigned Offset) : needle(F.data()), stop(needle+F.size())
    {
      if (Offset < F.size()) {
        needle += Offset;
      } else {
        needle = stop;
      }
    }

    void reset() { needle = stop = nullptr; }
    void reset(frame &F) { stop = (needle = F.data()) + F.size(); }
    void reset(frame &F, unsigned Offset) {
      reset(F);
      if (Offset < F.size()) {
        needle += Offset;
      } else {
        needle = stop;
      }
    }

    unsigned put(Uint8 v)
    {
      if (stop < needle + 1) return -1;
      return needle.put(v);
    }

    unsigned put(Uint16 v)
    {
      if (stop < needle + 2) return -1;
      return needle.put(v);
    }

    unsigned put(Uint32 v)
    {
      if (stop < needle + 4) return -1;
      return needle.put(v);
    }

    unsigned put(Uint64 v)
    {
      if (stop < needle + 8) return -1;
      return needle.put(v);
    }

    unsigned get(Uint8 & v)
    {
      if (stop < needle + 1) return -1;
      return needle.get(v);
    }

    unsigned get(Uint16 & v)
    {
      if (stop < needle + 2) return -1;
      return needle.get(v);
    }

    unsigned get(Uint32 & v)
    {
      if (stop < needle + 4) return -1;
      return needle.get(v);
    }

    unsigned get(Uint64 & v)
    {
      if (stop < needle + 8) return -1;
      return needle.get(v);
    }

    unsigned put(Int8 v)
    {
      if (stop < needle + 1) return -1;
      return needle.put(v);
    }

    unsigned put(Int16 v)
    {
      if (stop < needle + 2) return -1;
      return needle.put(v);
    }

    unsigned put(Int32 v)
    {
      if (stop < needle + 4) return -1;
      return needle.put(v);
    }

    unsigned put(Int64 v)
    {
      if (stop < needle + 8) return -1;
      return needle.put(v);
    }

    unsigned get(Int8 & v)
    {
      if (stop < needle + 1) return -1;
      return needle.get(v);
    }

    unsigned get(Int16 & v)
    {
      if (stop < needle + 2) return -1;
      return needle.get(v);
    }

    unsigned get(Int32 & v)
    {
      if (stop < needle + 4) return -1;
      return needle.get(v);
    }

    unsigned get(Int64 & v)
    {
      if (stop < needle + 8) return -1;
      return needle.get(v);
    }

    unsigned put(const TinyString & s)
    {
      if (stop < needle + sizeof(Uint8) + s.size()) return -1;
      return needle.put(s); 
    }
  
    unsigned put(const ShortString & s)
    {
      if (stop < needle + sizeof(Uint16) + s.size()) return -1;
      return needle.put(s); 
    }
  
    unsigned put(const LongString & s)
    {
      if (stop < needle + sizeof(Uint32) + s.size()) return -1;
      return needle.put(s); 
    }
  
    unsigned get(TinyString & s)
    {
      if (stop < needle + sizeof(Uint8)) return -1;
      return needle.get(s); 
    }
  
    unsigned get(ShortString & s)
    {
      if (stop < needle + sizeof(Uint16)) return -1;
      return needle.get(s); 
    }

    unsigned get(LongString & s)
    {
      if (stop < needle + sizeof(Uint32)) return -1;
      return needle.get(s); 
    }

    unsigned put(Uint8 *data, std::size_t size)
    {
      if (stop < needle + size) return -1;
      return needle.put(data, size);
    }

    unsigned get(Uint8 *data, std::size_t size)
    {
      if (stop < needle + size) return -1;
      return needle.get(data, size);
    }
 
    template <class T> T get() throw(std::logic_error)
    {
      T t;
      if (get(t) <= 0) {
        throw std::out_of_range("out of range");
      }
      return t;
    }
  
  private:
    needle needle;
    Uint8 *stop;
  };
} // anonymous namespace

namespace zero
{
#ifdef __ZERO_UNIQUE_SOCKET_HPP__
  unique_socket::unique_socket(int type) : _handle(zmq_socket(zmq.context(), type)) 
  {
    assert(_handle != nullptr);

    int rc, sndtimeo = 3*1000, rcvtimeo = 3*1000;
    if ((rc = zmq_setsockopt(_handle, ZMQ_SNDTIMEO, &sndtimeo, sizeof(sndtimeo))) < 0) {
      LOGE("setsockopt(SNDTIMEO): (%d) %s", zmq_errno(), zmq_strerror(zmq_errno()));
    }
    if ((rc = zmq_setsockopt(_handle, ZMQ_RCVTIMEO, &rcvtimeo, sizeof(rcvtimeo))) < 0) {
      LOGE("setsockopt(RCVTIMEO): (%d) %s", zmq_errno(), zmq_strerror(zmq_errno()));
    }
  }

#if 0
  unique_socket::~unique_socket()
  {
    if (_handle != nullptr) zmq_close(_handle);
  }

  unique_socket::unique_socket(unique_socket &&o) noexcept : _handle(o.release()) 
  {
  }

  unique_socket& unique_socket::operator=(unique_socket &&o) noexcept 
  {
    if (_handle != nullptr) zmq_close(_handle);
    _handle = o.release();
    return *this;
  }
#endif
#endif//__ZERO_UNIQUE_SOCKET_HPP__

#ifdef __ZERO_FRAME_HPP__
  int frame::init_value(std::uint32_t value)
  {
    auto RC = init_size(sizeof value);
    if (0 <= RC) {
      needle(*this).put(value);
    }
    return RC;
  }

  std::uint32_t frame::as_uint32() const
  {
    std::uint32_t value = 0;
    if (size() == sizeof value) {
      needle(*const_cast<frame*>(this)).get(value);
    }
    return value;
  }
#endif//__ZERO_FRAME_HPP__

#ifdef __ZERO_MESSAGE_HPP__
  message::message() : ptr_list(), HasRouteID(false)
  {
  }

  bool message::has_data() const
  {
    if (!empty()) for (auto & f : *this) if (0 < f.size()) return true;
    return false;
  }

  const frame *message::get_routing_frame() const
  {
    return (HasRouteID && !empty()) ? &front() : nullptr;
  }

  int message::recv(void *source, int flags)
  {
    int nbytes = 0, more, rc;

    HasRouteID = false;
  
    while (true) {
      std::unique_ptr<frame> f( new (frame) );
      if ((rc = f->recv(source, flags)) < 0) {
        return rc;
      }

      nbytes += rc;
      more = f->more();
      push_back(f.release());

      if (!more) break;
    }

    if (0 < rc && !empty()) {
      if (ZMQ_ROUTER == socktype(source)) {
        HasRouteID = true;
      }
    }
    return nbytes;
  }

  int message::send(void *dest, int flags)
  {
    int RC = -1;
#if 0
    for (size_type I=0, E=size(); I<E; ++I) {
      auto F = (I + 1 < E ? ZMQ_SNDMORE : 0) | flags;
      if ((RC = (*this)[I].send(dest, F)) != 0) break;
    }
#else
    unsigned I = 0, E = size();
    for (auto &R : *this) {
      auto F = (I + 1 < E ? ZMQ_SNDMORE : 0) | flags;
      if ((RC = R.send(dest, F)) != 0) break;
      I += 1;
    }
#endif
    return RC;
  }
#endif//__ZERO_MESSAGE_HPP__
#ifdef __ZERO_POLLER_HPP__

#endif//__ZERO_POLLER_HPP__
} // namespace zero
