#include "tube_utils.hpp"

namespace tube {

bool get_link(boost::asio::const_buffer &buffer, 
              std::string const &target_itag, 
              std::string &url, 
              std::string &signature)
{
  using namespace std;
  using namespace boost::asio;

  char const *beg(buffer_cast<char const*>(buffer));
  char const *end(beg + buffer_size(buffer));
  char const *iter;

  string pattern("\"url_encoded_fmt_stream_map\": \""),
         delim("\\u0026");
  string itag, field, value;
  int required = 0;

  iter = search(beg, end, pattern.begin(), pattern.end());
  if(iter != end) {
    beg = iter + pattern.size();
    iter = find(beg, end, '"');
    if( iter != end ) {
      end = iter;
      while( beg < end ) {
        char const *group_end = find(beg, end, ',');
        required = 0;
        for(int i=0;i<6 && required < 3;++i) {
          value = get_value(beg, group_end, field, delim);
          if(value.empty()) 
            return false;
          if( field == "itag") {
            itag = value;
            ++required;
          } else if( field == "sig" ) {
            signature = value;
            ++required;
          } else if ( field == "url" ) {
            url = value;
            ++required;
          }
        }
        if(itag == target_itag && required == 3) return true;
        if(group_end == end) break;
        beg = group_end + 1;
      }
    }
  }
  return false;
}


} // namespace tube_utils
