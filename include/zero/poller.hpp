#ifndef __ZERO_POLLER_HPP__
#define __ZERO_POLLER_HPP__ 1
namespace zero
{
  struct poller_event_t
  {
    void *socket;
    short events;
  };

  struct poller
  {
    poller();
    ~poller();

    int add(void *socket, void *user_data, short events);
    int modify(void *socket, short events);
    int remove(void *socket);
  
    int wait(poller_event_t *event, long timeout);

  private:
    void *handle;

    /// Disable copy from lvalue.
    poller(const poller &) = delete;
    poller & operator=(const poller &) = delete;
  };
}//namespace zero
#endif//__ZERO_POLLER_HPP__
