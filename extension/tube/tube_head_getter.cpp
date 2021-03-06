#include "tube_head_getter.hpp"
#include <cassert>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <agent/parser.hpp>
#include <agent/placeholders.hpp>
#include <json/accessor.hpp>
#include "tube_utils.hpp"

tube_head_getter::tube_head_getter(boost::asio::io_service &ios)
: agent_v2(ios)
{}

void tube_head_getter::async_head(json::var_t const &desc, 
                                  head_handler_type handler)
{
  using namespace boost::system;

  if(!validate_desc(desc)) {
    error_code ec(errc::invalid_argument, system_category());
    json::var_t rt;
    io_service().post(boost::bind(handler, ec, rt));
    return;
  }
  http::entity::url origin;
  http::request req;
  json::object_t const &sources = cmbof(desc)["sources"].object();
  
  assert( 1 == sources.size() && "No heading is required" );

  origin = cmbof(desc)["sources"].object().begin()->first;
  
  http::get_header(req.headers, "Connection")->value = "close";
  async_request(origin, req, "GET", false,
                boost::bind(&tube_head_getter::handle_page, shared_from_this(),
                            agent_arg::error, 
                            agent_arg::response,
                            agent_arg::buffer, 
                            handler,
                            boost::cref(desc)));
}

bool tube_head_getter::validate_desc(json::var_t const &desc)
{
  try {
    if(!cmbof(desc)["oid"] || !cmbof(desc)["sources"] )
      return false;
    cmbof(desc)["oid"].string();
    cmbof(desc)["sources"].object();
  } catch (boost::bad_get &) {
    return false;
  }
  return true;
}

void tube_head_getter::handle_page(boost::system::error_code const &ec, 
                                   http::response const &resp, 
                                   boost::asio::const_buffer buffer,
                                   head_handler_type handler,
                                   json::var_t const &desc)
{
  using http::entity::field;
  using namespace boost::system;
  try {
    if(boost::asio::error::eof == ec) {
      std::string url, signature;
      std::string const &itag = tube::find_itag(desc);
      if(tube::get_link(buffer, itag, url, signature)) {
        http::entity::url video_url;
        http::request req;

        url.append("%26signature%3D").append(signature);
        auto beg(url.begin()), end(url.end());
        http::parser::parse_url_esc_string(beg, end, video_url);
        req.headers << field("Range", "bytes=0-31");
        http::get_header(req.headers, "Connection")->value = "close";
        async_request(video_url, req, "GET", false,
                      boost::bind(
                        &tube_head_getter::handle_content_head, shared_from_this(), 
                        agent_arg::error, 
                        agent_arg::response,
                        agent_arg::buffer,
                        handler));
      } else { // get link failure
        json::var_t dummy;
        error_code invalid_arg(errc::invalid_argument, system_category());
        io_service().post(boost::bind(handler, invalid_arg, dummy));
      }
    } else if(ec) { // network failure
      json::var_t dummy;
      io_service().post(boost::bind(handler, ec, dummy));
    } else {}
  } catch(boost::bad_get &) {
    json::var_t dummy;
    error_code invalid_arg(errc::invalid_argument, system_category());
    io_service().post(boost::bind(handler, invalid_arg, dummy));
  }
}

void tube_head_getter::handle_content_head(boost::system::error_code const &ec,
                                           http::response const &resp,
                                           boost::asio::const_buffer buffer,
                                           head_handler_type handler)
{
  using boost::lexical_cast;
  json::var_t rt;
  if(boost::asio::error::eof == ec) {
    auto content_range = http::find_header(resp.headers, "Content-Range");
    auto npos = resp.headers.end();
    if(content_range != npos) {
      auto pos = content_range->value.find("/");
      if(pos != std::string::npos) {
        auto content_length = 
          lexical_cast<boost::intmax_t>(content_range->value.substr(pos + 1));
        mbof(rt)["content_length"].value(content_length);
      }
    }
    mbof(rt)["header_size"].value(tube::get_mp4_header_size(buffer));
    boost::system::error_code ok;
    io_service().post(boost::bind(handler, ok, rt));
  } else if(ec) {
    io_service().post(boost::bind(handler, ec, rt));
  } else {}
}

// ---- extension registration ----

MAGENT_EXT_HEAD_GETTER_IMPL(tube_head_getter, tube::is_supported)

