#ifndef MAGENT_TUBE_UTILS_HPP_
#define MAGENT_TUBE_UTILS_HPP_

#include <string>
#include <boost/asio/buffer.hpp>
#include <boost/cstdint.hpp>
#include "json/variant.hpp"

namespace tube {
namespace json = yangacer::json;

/** Get CDN link from web page.
 *  @param buffer Buffer contains page data.
 *  @param target_itag itag of content format and resolution.
 *  @param url Result URL.
 *  @param signature Result signature.
 *  @return Success/failure.
 */
bool get_link(boost::asio::const_buffer &buffer, 
              std::string const &target_itag, 
              std::string &url, 
              std::string &signature);

/** Get header size of a mp4 video.
 * @param front_data At least 32 bytes data for analysis of header.
 * @return 0 if analysis failed.
 */
boost::intmax_t get_mp4_header_size(boost::asio::const_buffer front_data);

/** Get itag according to content_type, resolution, and 3d flag.
 *  @see tube_head_getter::async_head
 */
std::string find_itag(json::var_t const &desc);

int is_supported(char const *content_type, char const *uri);

} // namespace tube
#endif
