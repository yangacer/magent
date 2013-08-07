#ifndef MAGENT_TUBE_HEAD_GETTER_HPP_
#define MAGENT_TUBE_HEAD_GETTER_HPP_

#include "extension.hpp"
#include "head_getter.hpp"
#include <string>
#include <boost/enable_shared_from_this.hpp>

/** \brief Head getter for tube service.
 * 
 * Callback parameter:
 * 
 * 1. error_code
 *
 * 2. Head information of type json::var_t
 * @codebeg
 * {
 *  content_length : video_size,
 *  min_segment_size : segguestion_for_minimum_segment_size (0 as unknown)
 * }
 * @endcode
 * 
 * @see head_getter::head_handler_type
 */
class tube_head_getter
: public head_getter,
  public boost::enable_shared_from_this<tube_head_getter>
{
public:
  tube_head_getter(boost::asio::io_service &ios);
  /** Get head info 
   *  @param desc
   *  Recognized description:
   *  @codebeg
   *  {
   *    oid : "id.1080.3d.mp4",
   *    sources : {
   *      "http://www.youtube.com/watch?v=XXXXXXXXXXX" : [ ... ],
   *      ...
   *    }
   *  }
   *  @endcode
   */
  void async_head(json::var_t const &desc, 
                  head_handler_type handler);
  ~tube_head_getter(){}
private:
  bool validate_desc(json::var_t const &desc);
  void handle_page(boost::system::error_code const &ec, 
                   http::response const &resp, 
                   boost::asio::const_buffer buffer,
                   head_handler_type handler,
                   json::var_t const &desc);

  void handle_content_head(boost::system::error_code const &ec,
                           http::response const &resp,
                           boost::asio::const_buffer buffer,
                           head_handler_type handler,
                           json::var_t const &desc);
};

#endif
