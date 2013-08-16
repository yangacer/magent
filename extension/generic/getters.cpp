#include <string>
#include <sstream>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <agent/agent_v2.hpp>
#include <agent/placeholders.hpp>
#include <json/json.hpp>
#include "extension.hpp"
#include "head_getter.hpp"
#include "data_getter.hpp"
#include "chunk_pool.hpp"

namespace json = yangacer::json;

struct generic_head_getter
: head_getter,
  agent_v2,
  boost::enable_shared_from_this<generic_head_getter>
{
  generic_head_getter(boost::asio::io_service &ios) : agent_v2(ios) {}
  void async_head(json::var_t const &desc, head_handler_type handler);
  ~generic_head_getter(){}
private:
  void handle_head(boost::system::error_code const &ec,
                   http::response const &resp,
                   head_handler_type handler);
};

struct generic_data_getter
: data_getter,
  agent_v2,
  boost::enable_shared_from_this<generic_data_getter>
{
  generic_data_getter(boost::asio::io_service &ios) 
    : agent_v2(ios), buffer_consumed_(0) {}
  void async_get(json::var_t const &desc,
                 std::string const &peer,
                 chunk_pool::chunk chk,
                 data_handler_type hdl);
private:
  void handle_content(boost::system::error_code const &ec,
                      http::response const &resp,
                      boost::asio::const_buffer buffer,
                      quality_config &qos,
                      data_handler_type handler,
                      json::var_t const &desc);
  std::string peer_;
  chunk_pool::chunk chunk_;
  size_t buffer_consumed_;
};

// --- generic_head_getter implementation ---

void generic_head_getter::async_head(json::var_t const &desc, head_handler_type handler)
{
  http::entity::url origin;
  http::request req;
  json::object_t const &sources = cmbof(desc)["sources"].object();
  
  assert( 1 == sources.size() && "No heading is required" );

  origin = cmbof(desc)["sources"].object().begin()->first;
  
  req.headers << http::entity::field("Range", "bytes=0-0");
  http::get_header(req.headers, "Connection")->value = "close";
  async_request(origin, req, "GET", false,
                boost::bind(&generic_head_getter::handle_head, shared_from_this(),
                            agent_arg::error, 
                            agent_arg::response,
                            handler));
}

void generic_head_getter::handle_head(boost::system::error_code const &ec,
                                      http::response const &resp,
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
    mbof(rt)["header_size"].test(boost::intmax_t(0));
    boost::system::error_code ok;
    io_service().post(boost::bind(handler, ok, rt));
  } else if(ec) {
    io_service().post(boost::bind(handler, ec, rt));
  } else {}
}

// ---- generic_data_getter implementation ----

void generic_data_getter::async_get(json::var_t const &desc,
                                    std::string const &peer,
                                    chunk_pool::chunk chk,
                                    data_handler_type handler)
{
  using http::entity::field;
  http::request req;
  std::stringstream cvt;

  peer_ = peer;
  chunk_ = chk;

  auto bufsize = boost::asio::buffer_size(chunk_.buffer);
  assert(bufsize > 0);
  cvt << "bytes=" <<  
    chunk_.offset << "-" << 
    (chunk_.offset + bufsize -1)
    ;
  req.headers << field("Range", cvt.str());
  http::get_header(req.headers, "Connection")->value = "close";
  async_request(peer, req, "GET", true,
                boost::bind(&generic_data_getter::handle_content, shared_from_this(),
                            agent_arg::error,
                            agent_arg::response,
                            agent_arg::buffer,
                            agent_arg::qos,
                            handler,
                            boost::cref(desc)));
}

void generic_data_getter::handle_content(boost::system::error_code const &ec,
                                         http::response const &resp,
                                         boost::asio::const_buffer buffer,
                                         quality_config &qos,
                                         data_handler_type handler,
                                         json::var_t const &desc)
{
  using boost::lexical_cast;
  using boost::asio::buffer_copy;
  using boost::asio::buffer_size;
  auto speed = cmbof(desc)["qos"].intmax(); 
  qos.read_max_bps = speed;
  if((!ec || boost::asio::error::eof == ec) && resp.status_code - 200 < 100) {
    buffer_consumed_ += buffer_copy(chunk_.buffer + buffer_consumed_, buffer);
    if(boost::asio::error::eof == ec ) {
      assert(buffer_consumed_ == buffer_size(chunk_.buffer));
      boost::system::error_code ok;
      io_service().post(boost::bind(handler, ok, peer_, chunk_));
      assert( boost::asio::error::eof == ec );
    }
  } else {
    io_service().post(boost::bind(handler, ec, peer_, chunk_));
  } 
}

// --- extension registration ----

int is_supported(char const*, char const* uri) 
{
  if(strncmp(uri, "http://", 7) == 0 ) return 1;
  return 0; 
}

MAGENT_EXT_HEAD_GETTER_IMPL(generic_head_getter, is_supported)
MAGENT_EXT_DATA_GETTER_IMPL(generic_data_getter, is_supported)

