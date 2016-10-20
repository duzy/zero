#ifndef __ZERO_UNIQUE_SOCKET_HPP__
#define __ZERO_UNIQUE_SOCKET_HPP__ 1
#include <cassert>
namespace zero
{
  struct unique_socket
  {
    explicit unique_socket(int type);

    unique_socket(unique_socket &&o) noexcept : _handle(o.release()) {}
  
    ~unique_socket() noexcept { 
      if (_handle != nullptr) zmq_close(_handle);
      //std::clog << "close: " << _handle << std::endl;
    }

    unique_socket& operator=(unique_socket &&o) noexcept {
      if (_handle != nullptr) zmq_close(_handle);
      _handle = o.release();
      return *this;
    }
  
    operator void*() const {
      assert(_handle != NULL);
      return _handle; 
    }

    void *release() noexcept {
      void *h = _handle;
      _handle = nullptr;
      return h;
    }

    int getsockopt(int option, void *optval, size_t *optvallen) {
      return zmq_getsockopt(_handle, option, optval, optvallen);
    }
    int setsockopt(int option, const void*optval, size_t optvallen) {
      return zmq_setsockopt(_handle, option, optval, optvallen);
    }

    int bind(const char *addr) { return zmq_bind(_handle, addr); }
    int unbind(const char *addr) { return zmq_unbind(_handle, addr); }

    int connect(const char *addr) { return zmq_connect(_handle, addr); }
    int disconnect(const char *addr) { return zmq_disconnect(_handle, addr); }

    int recv(void *buf, size_t len, int flags) {
      return zmq_recv(_handle, buf, len, flags);
    }
    int send(const void *buf, size_t len, int flags) {
      return zmq_send(_handle, buf, len, flags);
    }
    int send_const(const void *buf, size_t len, int flags) {
      return zmq_send_const(_handle, buf, len, flags);
    }

    int monitor(const char *addr, int events) {
      return zmq_socket_monitor(_handle, addr, events); 
    }
 
  private:
    void *_handle;

    // Disable default constructor.
    // unique_socket() = delete;

    // Disable copy from lvalue.
    unique_socket(const unique_socket&) = delete;
    unique_socket& operator=(const unique_socket&) = delete;
  };
} // namespace zero
#endif//__ZERO_UNIQUE_SOCKET_HPP__
