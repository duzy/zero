/*-----*- c++ -*-----*/
#include <zero/frame.hpp>
#include <zero/needle.hpp>
#include <zero/message.hpp>
#include <zero/unique_socket.hpp>
#include <zero/poller.hpp>
#include <zmq.h>
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
} // anonymous namespace

namespace zero
{
#ifdef __ZERO_UNIQUE_SOCKET_HPP__
  unique_socket::unique_socket(socket type) noexcept
    : handle(zmq_socket(zmq.context(), (int) type)) 
  {
    assert(handle != nullptr);

    int rc, sndtimeo = 3*1000, rcvtimeo = 3*1000;
    if ((rc = zmq_setsockopt(handle, ZMQ_SNDTIMEO, &sndtimeo, sizeof(sndtimeo))) < 0) {
      //LOGE("setsockopt(SNDTIMEO): (%d) %s", zmq_errno(), zmq_strerror(zmq_errno()));
    }
    if ((rc = zmq_setsockopt(handle, ZMQ_RCVTIMEO, &rcvtimeo, sizeof(rcvtimeo))) < 0) {
      //LOGE("setsockopt(RCVTIMEO): (%d) %s", zmq_errno(), zmq_strerror(zmq_errno()));
    }
  }

  unique_socket::~unique_socket()
  {
    if (handle != nullptr) zmq_close(release());
  }

  int unique_socket::close() noexcept { return zmq_close(release()); }

  int unique_socket::getsockopt(sockopt option, void *optval, size_t *optvallen) {
    return zmq_getsockopt(handle, (int) option, optval, optvallen);
  }
  int unique_socket::setsockopt(sockopt option, const void*optval, size_t optvallen) {
    return zmq_setsockopt(handle, (int) option, optval, optvallen);
  }

  int unique_socket::bind(const char *addr) { return zmq_bind(handle, addr); }
  int unique_socket::unbind(const char *addr) { return zmq_unbind(handle, addr); }

  int unique_socket::connect(const char *addr) { return zmq_connect(handle, addr); }
  int unique_socket::disconnect(const char *addr) { return zmq_disconnect(handle, addr); }

  int unique_socket::recv(void *buf, size_t len, int flags) { return zmq_recv(handle, buf, len, flags); }
  int unique_socket::send(const void *buf, size_t len, int flags) { return zmq_send(handle, buf, len, flags); }
  int unique_socket::send_const(const void *buf, size_t len, int flags) { return zmq_send_const(handle, buf, len, flags); }

  int unique_socket::monitor(const char *addr, int events) { return zmq_socket_monitor(handle, addr, events); }
#endif//__ZERO_UNIQUE_SOCKET_HPP__

#ifdef __ZERO_FRAME_HPP__
#define M reinterpret_cast<zmq_msg_t*>(&underlying)
#define MC reinterpret_cast<const zmq_msg_t*>(&underlying)
  frame::frame() { zmq_msg_init(M); }
  frame::frame( std::size_t size ) { zmq_msg_init_size(M, size); }
  frame::frame( const std::string &s )
  {
    auto size = s.size();
    zmq_msg_init_size(M, size);
    std::memcpy(data(), &s[0], size);
  }

  frame::~frame() { zmq_msg_close(M); }

  int frame::init() { return zmq_msg_init(M); }
  int frame::init_size(size_t size) { return zmq_msg_init_size(M, size); }
  int frame::init_data(void *data, size_t size, data_free_fn *ffn, void *hint) { return zmq_msg_init_data(M, data, size, /*(zmq_free_fn)*/ ffn, hint); }
  int frame::init_value(std::uint32_t value)
  {
    auto RC = init_size(sizeof value);
    if (0 <= RC) needle(*this).put(value);
    return RC;
  }

  std::size_t frame::size() const { return zmq_msg_size(const_cast<zmq_msg_t*>(MC)); }
  Uint8 *frame::data() { return (Uint8*) zmq_msg_data(M); }
  Uint8 *frame::data() const { return (Uint8*) zmq_msg_data(const_cast<zmq_msg_t*>(MC)); }
  Uint8 *frame::data_end() { return data() + size(); }
  Uint8 *frame::data_end() const { return data() + size(); }

  std::uint32_t frame::as_uint32() const
  {
    std::uint32_t value = 0;
    if (size() == sizeof value) {
      needle(*const_cast<frame*>(this)).get(value);
    }
    return value;
  }

  int frame::copy(frame & dest) { return zmq_msg_copy(reinterpret_cast<zmq_msg_t*>(&dest.underlying), M); }
  int frame::move(frame & dest) { return zmq_msg_move(reinterpret_cast<zmq_msg_t*>(&dest.underlying), M); }

  int frame::more() { return zmq_msg_more(M); }
  
  int frame::get(int property) const { return zmq_msg_get(const_cast<zmq_msg_t*>(MC), property); }
  int frame::set(int property, int optval) { return zmq_msg_set(M, property, optval); }

  //const char *frame::gets(const char *property) { return zmq_msg_gets(M, property); }
  
  int frame::send(void *s, int flags) { return zmq_msg_send(M, s, flags); }
  int frame::recv(void *s, int flags) { return zmq_msg_recv(M, s, flags); }

#if 0
  int frame::set_routing_id(std::uint32_t id) { return zmq_msg_set_routing_id(M, id); }
  std::uint32_t frame::routing_id() const { return zmq_msg_routing_id(const_cast<zmq_msg_t*>(MC)); }
#endif

#undef M
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
#ifdef ZMQ_HAVE_POLLER //ZMQ_MAKE_VERSION(4, 2, 0) <= ZMQ_VERSION

  poller::poller() : handle(zmq_poller_new())
  {
    assert(handle && "invalid poller handle");
  }

  poller::~poller()
  {
    if (zmq_poller_destroy(&handle) < 0) {
      assert(false && "zmq_poller_destroy");
    }
  }

  int poller::add(void *socket, void *user_data, short events) { return zmq_poller_add(handle, socket, user_data, events); }
  int poller::modify(void *socket, short events) { return zmq_poller_modify(handle, socket, events); }
  int poller::remove(void *socket) { return zmq_poller_remove(handle, socket); }
  
  int poller::wait(poller_event_t *event, long timeout)
  {
    zmq_poller_event_t Ev{ nullptr, 0, nullptr, 0 };
    auto EC = zmq_poller_wait(handle, &Ev, timeout);
    event->socket = Ev.socket;
    event->events = Ev.events;
    event->revents = Ev.revents;
    return EC;
  }

#else

  struct poller_impl
  {
    std::list<zmq_pollitem_t> items;
  };

  poller::poller() : handle(new poller_impl())
  {
  }

  poller::~poller()
  {
    delete reinterpret_cast<poller_impl*>(handle);
    handle = nullptr;
  }

  int poller::add(void *socket, void */*user_data*/, short events)
  {
    auto impl = reinterpret_cast<poller_impl*>(handle);
    impl->items.push_back({ socket, 0, events, 0 });
    return 0;
  }

  int poller::modify(void *socket, short events)
  {
    auto impl = reinterpret_cast<poller_impl*>(handle);
    for (auto &item : impl->items) {
      if (item.socket == socket) {
        item.events = events;
      }
    }
    return 0;
  }

  int poller::remove(void *socket) 
  {
    auto impl = reinterpret_cast<poller_impl*>(handle);
    auto it = impl->items.end();
    for (auto i = impl->items.begin(); i != impl->items.end();) {
      if (i->socket == socket) {
        i = impl->items.erase(i);
      } else {
        ++i;        
      }
    }
    return 0;
  }
  
  int poller::wait(poller_event_t *event, long timeout)
  {
    auto impl = reinterpret_cast<poller_impl*>(handle);
    
    event->socket = 0;
    event->events = 0;
    event->revents = 0;

    auto EC = zmq_poll(&impl->items[0], impl->items.size(), timeout);
    if ( EC < 0 ) return EC;
    for (auto &item : impl->items) {
      if (item.events & item.revents) {
        event->socket = item.socket;
        event->events = item.events;
        event->revents = item.revents;
        return 0;
      }
    }
    return EC;
  }

#endif// 4.2.0 <= ZMQ_VERSION
#endif//__ZERO_POLLER_HPP__

  int errnum(void) { return zmq_errno(); }
  const char *strerror(int en) { return zmq_strerror(en); }
  const char *strerror() { return zmq_strerror(zmq_errno()); }
} // namespace zero
