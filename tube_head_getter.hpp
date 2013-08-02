#ifndef MAGENT_TUBE_HEAD_GETTER_HPP_
#define MAGENT_TUBE_HEAD_GETTER_HPP_

#include "head_getter.hpp"

class tube_head_getter
: public head_getter
{
public:
  tube_head_getter(boost::asio::io_service &ios);
  void async_head(http::entity::url const &origin, 
                  head_handler_type handler);
private:
  void handle_page(boost::system::error_code const &ec, 
                   http::response const &resp, 
                   boost::asio::const_buffer buffer,
                   head_handler_type handler);

  void handle_content_head(boost::system::error_code const &ec,
                           http::response const &resp);
};

#endif
