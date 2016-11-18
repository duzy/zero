#ifndef __ZERO_UNIQUE_SOCKET_HPP__
#define __ZERO_UNIQUE_SOCKET_HPP__ 1
namespace zero
{
  enum class socket
  {
    PAIR  = 0,
    PUB   = 1,
    SUB   = 2,
    REQ   = 3,
    REP   = 4,
    DEALER = 5,
    ROUTER = 6,
    PULL   = 7,
    PUSH   = 8,
    XPUB   = 9,
    XSUB   = 10,
    STREAM = 11,
    XREQ = DEALER,
    XREP = ROUTER,

    /*
    SERVER = 12,
    CLIENT = 13,
    RADIO = 14,
    DISH = 15,
    GATHER = 16,
    SCATTER = 17,
    DGRAM = 18,
    */
  };

  enum class sockopt
  {
    AFFINITY    = 4,
    IDENTITY    = 5,
    SUBSCRIBE   = 6,
    UNSUBSCRIBE = 7,
    RATE        = 8,
    RECOVERY_IVL= 9,
    SNDBUF      = 11,
    RCVBUF      = 12,
    RCVMORE     = 13,
    FD          = 14,
    EVENTS      = 15,
    TYPE        = 16,
    LINGER      = 17,
    RECONNECT_IVL = 18,
    BACKLOG     = 19,
    RECONNECT_IVL_MAX = 21,
    MAXMSGSIZE  = 22,
    SNDHWM      = 23,
    RCVHWM      = 24,
    MULTICAST_HOPS = 25,
    RCVTIMEO    = 27,
    SNDTIMEO    = 28,
    LAST_ENDPOINT = 32,
    ROUTER_MANDATORY = 33,
    TCP_KEEPALIVE = 34,
    TCP_KEEPALIVE_CNT = 35,
    TCP_KEEPALIVE_IDLE = 36,
    TCP_KEEPALIVE_INTVL = 37,
    IMMEDIATE   = 39,
    XPUB_VERBOSE = 40,
    ROUTER_RAW  = 41,
    IPV6        = 42,
    MECHANISM   = 43,
    PLAIN_SERVER = 44,
    PLAIN_USERNAME = 45,
    PLAIN_PASSWORD = 46,
    CURVE_SERVER   = 47,
    CURVE_PUBLICKEY = 48,
    CURVE_SECRETKEY = 49,
    CURVE_SERVERKEY = 50,
    PROBE_ROUTER    = 51,
    REQ_CORRELATE   = 52,
    REQ_RELAXED     = 53,
    CONFLATE        = 54,
    ZAP_DOMAIN      = 55,
    ROUTER_HANDOVER = 56,
    TOS             = 57,
    CONNECT_RID     = 61,
    GSSAPI_SERVER   = 62,
    GSSAPI_PRINCIPAL = 63,
    GSSAPI_SERVICE_PRINCIPAL = 64,
    GSSAPI_PLAINTEXT = 65,
    HANDSHAKE_IVL   = 66,
    SOCKS_PROXY     = 68,
    XPUB_NODROP     = 69,
  };
  
  struct unique_socket
  {
    explicit unique_socket(socket type) noexcept;

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

    int getsockopt(sockopt option, void *optval, size_t *optvallen);
    int setsockopt(sockopt option, const void*optval, size_t optvallen);

    int bind(const std::string &s) { return bind(s.c_str()); }
    int bind(const char *addr);
    int unbind(const std::string &s) { return unbind(s.c_str()); }
    int unbind(const char *addr);

    int connect(const std::string &s) { return connect(s.c_str()); }
    int connect(const char *addr);
    int disconnect(const std::string &s) { return disconnect(s.c_str()); }
    int disconnect(const char *addr);

    int recv(void *buf, size_t len, int flags = 0);
    int send(const void *buf, size_t len, int flags = 0);
    int send_const(const void *buf, size_t len, int flags = 0);

    int monitor(const std::string &s, int events) { return monitor(s.c_str(), events); }
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
