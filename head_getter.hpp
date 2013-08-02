#ifndef MAGENT_HEAD_GETTER_HPP_
#define MAGENT_HEAD_GETTER_HPP_

#include "agent/agent_v2.hpp"
#include "json/variant.hpp"

namespace json = yangacer::json;

class head_getter : protected agent_v2
{
public:
  typedef boost::function<
    void(boost::system::error_code const&, 
         json::var_t const &)
    > head_handler_type;

  head_getter(boost::asio::io_service &ios) : agent_v2(ios) {}
  virtual void async_head(http::entity::url const &origin, head_handler_type handler) = 0;
};

#endif
