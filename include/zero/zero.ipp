/*-----*- c++ -*-----*/
#include <zero/frame.hpp>
#include <zero/message.hpp>
#include <zero/unique_socket.hpp>
#include <zero/poller.hpp>
#include <boost/noncopyable.hpp>

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
    explicit needle(zero::Uint8 *P) : pointer(P) {}
    explicit needle(zero::frame &F) : pointer(F.data()) {}

    unsigned put(zero::Uint8 v)
    {
      assert(pointer != NULL);
      *pointer = v;
      pointer++;
      return 1;
    }

    unsigned put(zero::Uint16 v)
    {
      assert(pointer != NULL);
      pointer [0] = zero::Uint8(((v >> 8)  & 0xFF));
      pointer [1] = zero::Uint8(((v)       & 0xFF)) ;
      pointer += 2;
      return 2;
    }

    unsigned put(zero::Uint32 v)
    {
      assert(pointer != NULL);
      pointer [0] = zero::Uint8(((v >> 24) & 0xFF));
      pointer [1] = zero::Uint8(((v >> 16) & 0xFF));
      pointer [2] = zero::Uint8(((v >> 8)  & 0xFF));
      pointer [3] = zero::Uint8(((v)       & 0xFF));
      pointer += 4;
      return 4;
    }

    unsigned put(zero::Uint64 v)
    {
      assert(pointer != NULL);
      pointer [0] = zero::Uint8(((v >> 56) & 0xFF));
      pointer [1] = zero::Uint8(((v >> 48) & 0xFF));
      pointer [2] = zero::Uint8(((v >> 40) & 0xFF));
      pointer [3] = zero::Uint8(((v >> 32) & 0xFF));
      pointer [4] = zero::Uint8(((v >> 24) & 0xFF));
      pointer [5] = zero::Uint8(((v >> 16) & 0xFF));
      pointer [6] = zero::Uint8(((v >> 8)  & 0xFF));
      pointer [7] = zero::Uint8(((v)       & 0xFF));
      pointer += 8;
      return 8;
    }

    unsigned get(zero::Uint8 & v)
    {
      assert(pointer != NULL);
      v = *pointer;
      pointer++;
      return 1;
    }

    unsigned get(zero::Uint16 & v)
    {
      assert(pointer != NULL);
      v = ((zero::Uint16)(pointer [0]) << 8)
    + ((zero::Uint16)(pointer [1])) ;
      pointer += 2;
      return 2;
    }

    unsigned get(zero::Uint32 & v)
    {
      assert(pointer != NULL);
      v = ((zero::Uint32)(pointer [0]) << 24)
    + ((zero::Uint32)(pointer [1]) << 16)
    + ((zero::Uint32)(pointer [2]) << 8)
    + ((zero::Uint32)(pointer [3]) ) ;
      pointer += 4;
      return 4;
    }

    unsigned get(zero::Uint64 & v)
    {
      assert(pointer != NULL);
      v = ((zero::Uint64)(pointer [0]) << 56)
    + ((zero::Uint64)(pointer [1]) << 48)
    + ((zero::Uint64)(pointer [2]) << 40)
    + ((zero::Uint64)(pointer [3]) << 32)
    + ((zero::Uint64)(pointer [4]) << 24)
    + ((zero::Uint64)(pointer [5]) << 16)
    + ((zero::Uint64)(pointer [6]) << 8)
    +  (zero::Uint64)(pointer [7]) ;
      pointer += 8;
      return 8;
    }

    unsigned put(zero::Int8  v)  { return put(static_cast<zero::Uint8> (v)); }
    unsigned put(zero::Int16 v)  { return put(static_cast<zero::Uint16>(v)); }
    unsigned put(zero::Int32 v)  { return put(static_cast<zero::Uint32>(v)); }
    unsigned put(zero::Int64 v)  { return put(static_cast<zero::Uint64>(v)); }
    unsigned get(zero::Int8  &v) { return get(*reinterpret_cast<zero::Uint8*> (&v)); }
    unsigned get(zero::Int16 &v) { return get(*reinterpret_cast<zero::Uint16*>(&v)); }
    unsigned get(zero::Int32 &v) { return get(*reinterpret_cast<zero::Uint32*>(&v)); }
    unsigned get(zero::Int64 &v) { return get(*reinterpret_cast<zero::Uint64*>(&v)); }

    template <class SizeType>
    unsigned put_string(const std::string & s)
    {
      assert(pointer != NULL);
      std::size_t size = s.size();
      auto n = put(SizeType(size));
      std::memcpy(pointer, &s[0], size);
      pointer += size;
      return n + size;
    }

    template <class SizeType>
    unsigned get_string(std::string & s)
    {
      assert(pointer != NULL);
      SizeType size;
      auto n = get(size);
      s.resize(size);
      std::memcpy(&s[0], pointer, size);
      pointer += size;
      return n + size;
    }

    unsigned put(const zero::TinyString & s)    { return put_string<zero::Uint8>(s); }
    unsigned put(const zero::ShortString & s)   { return put_string<zero::Uint16>(s); }
    unsigned put(const zero::LongString & s)    { return put_string<zero::Uint32>(s); }
    unsigned get(zero::TinyString & s)          { return get_string<zero::Uint8>(s); }
    unsigned get(zero::ShortString & s)         { return get_string<zero::Uint16>(s); }
    unsigned get(zero::LongString & s)          { return get_string<zero::Uint32>(s); }

    unsigned put(zero::Uint8 *data, std::size_t size)
    {
      assert(pointer != NULL);
      std::memcpy(pointer, data, size);
      pointer += size;
      return size;
    }

    unsigned get(zero::Uint8 *data, std::size_t size)
    {
      assert(pointer != NULL);
      std::memcpy(data, pointer, size);
      pointer += size;
      return size;
    }

    operator zero::Uint8 *() const { return pointer; }

    needle & operator =(zero::Uint8 *p) { pointer  = p; return *this; }
    needle & operator+=(unsigned n) { pointer += n; return *this; }
 
  private:
    zero::Uint8 *pointer;
  };

  struct frame_builder : private boost::noncopyable
  {
    explicit frame_builder() : needle_(nullptr), stop(nullptr) {}
    explicit frame_builder(zero::frame &F) : needle_(F.data()), stop(needle_ + F.size()) {}
    explicit frame_builder(zero::frame &F, unsigned Offset) : needle_(F.data()), stop(needle_+F.size())
    {
      if (Offset < F.size()) {
        needle_ += Offset;
      } else {
        needle_ = stop;
      }
    }

    void reset() { needle_ = stop = nullptr; }
    void reset(zero::frame &F) { stop = (needle_ = F.data()) + F.size(); }
    void reset(zero::frame &F, unsigned Offset) {
      reset(F);
      if (Offset < F.size()) {
        needle_ += Offset;
      } else {
        needle_ = stop;
      }
    }

    unsigned put(zero::Uint8 v)
    {
      if (stop < needle_ + 1) return -1;
      return needle_.put(v);
    }

    unsigned put(zero::Uint16 v)
    {
      if (stop < needle_ + 2) return -1;
      return needle_.put(v);
    }

    unsigned put(zero::Uint32 v)
    {
      if (stop < needle_ + 4) return -1;
      return needle_.put(v);
    }

    unsigned put(zero::Uint64 v)
    {
      if (stop < needle_ + 8) return -1;
      return needle_.put(v);
    }

    unsigned get(zero::Uint8 & v)
    {
      if (stop < needle_ + 1) return -1;
      return needle_.get(v);
    }

    unsigned get(zero::Uint16 & v)
    {
      if (stop < needle_ + 2) return -1;
      return needle_.get(v);
    }

    unsigned get(zero::Uint32 & v)
    {
      if (stop < needle_ + 4) return -1;
      return needle_.get(v);
    }

    unsigned get(zero::Uint64 & v)
    {
      if (stop < needle_ + 8) return -1;
      return needle_.get(v);
    }

    unsigned put(zero::Int8 v)
    {
      if (stop < needle_ + 1) return -1;
      return needle_.put(v);
    }

    unsigned put(zero::Int16 v)
    {
      if (stop < needle_ + 2) return -1;
      return needle_.put(v);
    }

    unsigned put(zero::Int32 v)
    {
      if (stop < needle_ + 4) return -1;
      return needle_.put(v);
    }

    unsigned put(zero::Int64 v)
    {
      if (stop < needle_ + 8) return -1;
      return needle_.put(v);
    }

    unsigned get(zero::Int8 & v)
    {
      if (stop < needle_ + 1) return -1;
      return needle_.get(v);
    }

    unsigned get(zero::Int16 & v)
    {
      if (stop < needle_ + 2) return -1;
      return needle_.get(v);
    }

    unsigned get(zero::Int32 & v)
    {
      if (stop < needle_ + 4) return -1;
      return needle_.get(v);
    }

    unsigned get(zero::Int64 & v)
    {
      if (stop < needle_ + 8) return -1;
      return needle_.get(v);
    }

    unsigned put(const zero::TinyString & s)
    {
      if (stop < needle_ + sizeof(zero::Uint8) + s.size()) return -1;
      return needle_.put(s); 
    }
  
    unsigned put(const zero::ShortString & s)
    {
      if (stop < needle_ + sizeof(zero::Uint16) + s.size()) return -1;
      return needle_.put(s); 
    }
  
    unsigned put(const zero::LongString & s)
    {
      if (stop < needle_ + sizeof(zero::Uint32) + s.size()) return -1;
      return needle_.put(s); 
    }
  
    unsigned get(zero::TinyString & s)
    {
      if (stop < needle_ + sizeof(zero::Uint8)) return -1;
      return needle_.get(s); 
    }
  
    unsigned get(zero::ShortString & s)
    {
      if (stop < needle_ + sizeof(zero::Uint16)) return -1;
      return needle_.get(s); 
    }

    unsigned get(zero::LongString & s)
    {
      if (stop < needle_ + sizeof(zero::Uint32)) return -1;
      return needle_.get(s); 
    }

    unsigned put(zero::Uint8 *data, std::size_t size)
    {
      if (stop < needle_ + size) return -1;
      return needle_.put(data, size);
    }

    unsigned get(zero::Uint8 *data, std::size_t size)
    {
      if (stop < needle_ + size) return -1;
      return needle_.get(data, size);
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
    needle needle_;
    zero::Uint8 *stop;
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
      //LOGE("setsockopt(SNDTIMEO): (%d) %s", zmq_errno(), zmq_strerror(zmq_errno()));
    }
    if ((rc = zmq_setsockopt(_handle, ZMQ_RCVTIMEO, &rcvtimeo, sizeof(rcvtimeo))) < 0) {
      //LOGE("setsockopt(RCVTIMEO): (%d) %s", zmq_errno(), zmq_strerror(zmq_errno()));
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
  message::message()
    : ptr_list(), HasRouteID(false)
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
