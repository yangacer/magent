#ifndef MAGENT_TUBE_UTILS_HPP_
#define MAGENT_TUBE_UTILS_HPP_

#include <string>
#include <algorithm>
#include <boost/asio/buffer.hpp>

namespace tube {

template<typename Iter>
std::string get_value(
  Iter& beg, Iter& end,
  std::string &field, std::string const &delim)
{ 
  Iter i;
  std::string rt;
  i = std::find(beg, end, '=');
  if( i != end) {
    field.assign(beg, i);
    beg = i + 1;
    i = std::search(beg, end, delim.begin(), delim.end());
    rt.assign(beg, i);
    beg = (i != end) ? i + delim.size() : end ;
  }
  return rt;
}

bool get_link(boost::asio::const_buffer &buffer, 
              std::string const &target_itag, 
              std::string &url, 
              std::string &signature);
} // namespace tube
#endif
