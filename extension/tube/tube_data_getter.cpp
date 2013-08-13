#include "tube_data_getter.hpp"
#include <sstream>
#include <boost/bind.hpp>
#include <agent/placeholders.hpp>
#include <agent/parser.hpp>
#include "tube_utils.hpp"

tube_data_getter::tube_data_getter(boost::asio::io_service &ios)
: agent_v2(ios)
{}

void tube_data_getter::async_get(json::var_t const &desc, 
                                 std::string const &peer,
                                 chunk_pool::chunk chk,
                                 data_handler_type handler)
{
  using namespace boost::system;
  /*
  if(!validate_desc(desc)) {
    error_code ec(errc::invalid_argument, system_category());
    json::var_t rt;
    io_service().post(boost::bind(handler, ec, rt));
    return;
  }
  */
  // TODO do get content if peer == page_url_
  chunk_ = chk;
  if( peer == page_url_ ) {
    get_video(handler, desc);
  } else {
    page_url_ = peer;
    video_url_ = http::entity::url();
    http::entity::url url(page_url_);
    http::request req;
    http::get_header(req.headers, "Connection")->value = "close";
    async_request(url, req, "GET", false,
                  boost::bind(
                    &tube_data_getter::handle_page, shared_from_this(),
                    agent_arg::error, 
                    agent_arg::response,
                    agent_arg::buffer, 
                    handler,
                    desc));
  }
}

/*
bool tube_data_getter::validate_desc(json::var_t const &desc)
{
  return true;
}
*/

void tube_data_getter::get_video(data_handler_type handler, json::var_t const &desc)
{
  using http::entity::field;
  http::request req;
  std::stringstream cvt;
  auto bufsize = boost::asio::buffer_size(chunk_.buffer);
  assert(bufsize > 0);
  cvt << "bytes=" <<  
    chunk_.offset << "-" << 
    (chunk_.offset + bufsize -1)
    ;
  req.headers << field("Range", cvt.str()); //<< field("Connection", "close");
  async_request(video_url_, req, "GET", false,
                boost::bind(
                  &tube_data_getter::handle_content, shared_from_this(), 
                  agent_arg::error,
                  agent_arg::request,
                  agent_arg::response,
                  agent_arg::buffer,
                  handler,
                  desc,
                  0));
}

void tube_data_getter::handle_page(boost::system::error_code const &ec, 
                                   http::response const &resp, 
                                   boost::asio::const_buffer buffer,
                                   data_handler_type handler,
                                   json::var_t const &desc)
{
  using http::entity::field;
  using namespace boost::system;
  try {
    if(boost::asio::error::eof == ec) {
      std::string url, signature;
      std::string const &itag = tube::find_itag(desc);
      if(tube::get_link(buffer, itag, url, signature)) {
        // http::entity::url video_url;
        http::request req;
        std::stringstream cvt;

        url.append("%26signature%3D").append(signature);
        auto beg(url.begin()), end(url.end());
        http::parser::parse_url_esc_string(beg, end, video_url_);
        get_video(handler, desc);
      } else { // get link failure
        error_code invalid_arg(errc::invalid_argument, system_category());
        io_service().post(boost::bind(handler, invalid_arg, page_url_, chunk_));
      }
    } else if(ec) { // network failure
      io_service().post(boost::bind(handler, ec, page_url_, chunk_));
    } else {}
  } catch(boost::bad_get &) {
    error_code invalid_arg(errc::invalid_argument, system_category());
    io_service().post(boost::bind(handler, invalid_arg, page_url_, chunk_));
  }
}

void tube_data_getter::handle_content(boost::system::error_code const &ec,
                                      http::request const &req,
                                      http::response const &resp,
                                      boost::asio::const_buffer buffer,
                                      data_handler_type handler,
                                      json::var_t const &desc,
                                      size_t buffer_consumed)
{
  using boost::lexical_cast;
  using boost::asio::buffer_copy;
  using boost::asio::buffer_size;
  if(!ec || boost::asio::error::eof == ec) {
    buffer_consumed += buffer_copy(chunk_.buffer + buffer_consumed, buffer);
    if(buffer_consumed == buffer_size(chunk_.buffer)) {
      boost::system::error_code ok;
      io_service().post(boost::bind(handler, ok, page_url_, chunk_));
      assert( boost::asio::error::eof == ec );
    }
  } else {
    io_service().post(boost::bind(handler, ec, page_url_, chunk_));
  } 
}

// ---- extension registration ----

extern "C" int magent_ext_support_data(char const *content_type, char const *uri)
{
  return tube::is_supported(content_type, uri);
}

MAGENT_EXT_DATA_GETTER_IMPL(tube_data_getter)