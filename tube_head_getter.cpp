#include "tube_head_getter.hpp"
#include <boost/bind.hpp>
#include <agent/parser.hpp>
#include <agent/placeholders.hpp>
#include "tube_utils.hpp"

tube_head_getter::tube_head_getter(boost::asio::io_service &ios)
: head_getter(ios)
{}

void tube_head_getter::async_head(http::entity::url const &origin, 
                                  head_handler_type handler)
{
  http::request req;
  http::get_header(req.headers, "Connection")->value = "close";
  async_request(origin, req, "GET", false,
                boost::bind(&tube_head_getter::handle_page, this,
                            agent_arg::error, 
                            agent_arg::response,
                            agent_arg::buffer, 
                            handler));
}

void tube_head_getter::handle_page(boost::system::error_code const &ec, 
                                   http::response const &resp, 
                                   boost::asio::const_buffer buffer,
                                   head_handler_type handler)
{
  using http::entity::field;

  if(boost::asio::error::eof == ec) {
    std::string url, signature;
    // TODO changeable itag
    if(tube::get_link(buffer, "22", url, signature)) {
      url.append("%26signature%3D").append(signature);
      auto beg(url.begin()), end(url.end());
      http::entity::url video_url;
      http::parser::parse_url_esc_string(beg, end, video_url);
      http::request req;
      std::stringstream cvt;
      cvt << "bytes=0-0";
      req.headers << field("Range", cvt.str());
      async_request(video_url, req, "GET", true,
                    boost::bind(
                      &tube_head_getter::handle_content_head, this, 
                      agent_arg::error, 
                      agent_arg::response));
    }
  } else if(ec) {
    handler(ec, json::var_t());
  } else {}
}

void tube_head_getter::handle_content_head(boost::system::error_code const &ec,
                                           http::response const &resp)
{}
