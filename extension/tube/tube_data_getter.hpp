#ifndef MAGENT_TUBE_DATA_GETTER_HPP_
#define MAGENT_TUBE_DATA_GETTER_HPP_

#include <agent/agent_v2.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "extension.hpp"
#include "data_getter.hpp"

class tube_data_getter
: public data_getter,
  public agent_v2,
  public boost::enable_shared_from_this<tube_data_getter>
{
public:
  tube_data_getter(boost::asio::io_service &ios);
  void async_get(json::var_t const &desc, 
                 std::string const &peer,
                 chunk_pool::chunk chk, 
                 data_handler_type hdl);
  ~tube_data_getter(){}
private:
  void handle_page(boost::system::error_code const &ec, 
                   http::response const &resp, 
                   boost::asio::const_buffer buffer,
                   data_handler_type handler,
                   json::var_t const &desc);
  void get_video(data_handler_type handler, json::var_t const &desc);
  void handle_content(boost::system::error_code const &ec,
                      http::request const &req,
                      http::response const &resp,
                      boost::asio::const_buffer buffer,
                      data_handler_type handler,
                      json::var_t const &desc,
                      size_t buffer_consumed);
  std::string page_url_;
  http::entity::url video_url_;
  chunk_pool::chunk chunk_;
};

#endif
