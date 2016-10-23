#ifndef __ZERO_UNIQUE_SOCKET_HPP__
#define __ZERO_UNIQUE_SOCKET_HPP__ 1
namespace zero
{
  struct unique_socket
  {
    //constexpr unique_socket() noexcept : handle(nullptr) {}

    explicit unique_socket(int type) noexcept;

    ~unique_socket() noexcept;

    unique_socket(unique_socket &&o) noexcept : handle(o.release()) { }
    
    unique_socket& operator=(unique_socket &&o) noexcept {
      if (handle != nullptr) close();
      handle = o.release();
      return *this;
    }

    void *get() const noexcept { return handle; }
    void *release() noexcept { auto h = handle; handle = nullptr; return h; }

    int close() noexcept;

    int getsockopt(int option, void *optval, size_t *optvallen);
    int setsockopt(int option, const void*optval, size_t optvallen);

    int bind(const char *addr);
    int unbind(const char *addr);

    int connect(const char *addr);
    int disconnect(const char *addr);

    int recv(void *buf, size_t len, int flags);
    int send(const void *buf, size_t len, int flags);
    int send_const(const void *buf, size_t len, int flags);

    int monitor(const char *addr, int events);
 
  private:
    void *handle;

    // Disable default constructor.
    unique_socket() = delete;
    
    // Disable copy from lvalue.
    unique_socket(const unique_socket&) = delete;
    unique_socket& operator=(const unique_socket&) = delete;
  };
} // namespace zero
#endif//__ZERO_UNIQUE_SOCKET_HPP__
