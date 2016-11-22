#ifndef __ZERO_MESSAGE_HPP__
#define __ZERO_MESSAGE_HPP__ 1
#ifndef __JNI_GLUE_GEN__
#  include <boost/ptr_container/ptr_list.hpp>
#endif
#include "zero/frame.hpp"
namespace zero
{
  struct message
#ifndef __JNI_GLUE_GEN__
    : boost::ptr_list<frame>
#endif
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
    const frame *get_routing_frame() const;
  
    /**
     *  `recv` receive Frames from `source`, returns zero on success or non-zero when partially
     *  or none received.
     */
    int recv(void *source, int flags = 0);

    /**
     *  send message to `dest`, returns zero after successfully sent or non-zero on partially
     *  sent or none sent.
     */
    int send(void *dest, int flags = 0);

    template<typename Socket> int send(const Socket &sock, int flags = 0) { return this->send(sock.get(), flags); }
    template<typename Socket> int recv(const Socket &sock, int flags = 0) { return this->recv(sock.get(), flags); }
    
  private:
    bool HasRouteID;
  };
} // namespace zero
#endif//__ZERO_MESSAGE_HPP__
