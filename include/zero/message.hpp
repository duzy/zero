#ifndef __ZERO_MESSAGE_HPP__
#define __ZERO_MESSAGE_HPP__ 1
#  include <boost/ptr_container/ptr_list.hpp>
namespace zero
{
  struct message : boost::ptr_list<Frame>
  {
    message();
    ~message() = default;

    /**
     *  Returns true if the message is not empty and any Frame of it has any bytes.
     */
    bool has_data() const;

    /**
     *  Returns route id or nullptr(if not a routed message) .
     */
    const Frame *get_routing_frame() const;
  
    /**
     *  `recv` receive Frames from `source`, returns zero on success or non-zero when partially
     *  or none received.
     */
    int recv(void *source, int flags);

    /**
     *  send message to `dest`, returns zero after successfully sent or non-zero on partially
     *  sent or none sent.
     */
    int send(void *dest, int flags);

  private:
    bool HasRouteID;
  };
} // namespace zero
#endif//__ZERO_MESSAGE_HPP__
