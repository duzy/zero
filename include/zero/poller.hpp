#if ZMQ_MAKE_VERSION(4, 2, 0) <= ZMQ_VERSION
#ifndef __ZERO_POLLER_HPP__
#define __ZERO_POLLER_HPP__ 1
namespace zero
{
  struct poller
  {
    poller() : _handle(zmq_poller_new()) {
      assert(_handle && "invalid poller handle");
    }

    ~poller() {
      if (zmq_poller_destroy(&_handle) < 0) {
        /// ...
      }
    }

    int add(void *socket, void *user_data, short events) { return zmq_poller_add(_handle, socket, user_data, events); }
    int modify(void *socket, short events) { return zmq_poller_modify(_handle, socket, events); }
    int remove(void *socket) { return zmq_poller_remove(_handle, socket); }
  
    int wait(zmq_poller_event_t *event, long timeout) { return zmq_poller_wait(_handle, event, timeout); }

  private:
    void *_handle;

    /// Disable copy from lvalue.
    poller(const poller &) = delete;
    poller & operator=(const poller &) = delete;
  };
}//namespace zero
#endif//__ZERO_POLLER_HPP__
#endif// 4.2.0 <= ZMQ_VERSION
