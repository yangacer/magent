#include "tube_utils.hpp"
#include <map>
#include <algorithm>
#include <cassert>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include "json/accessor.hpp"

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

#include <cstdio>
boost::intmax_t get_mp4_header_size(boost::asio::const_buffer front_data)
{
#define GET_ATOM_SIZE_(Byte, Size) \
  { Size = 0; \
    for(int i_=0; i_ < 4; ++i_) { \
      Size <<= 8; \
      Size |= (unsigned char)Byte[i_]; \
    } \
  }
  size_t size = boost::asio::buffer_size(front_data);
  assert( size > 0x1c && 
          "Have no sufficient front_data to determine head size of a mp4");
  char const 
    *raw = boost::asio::buffer_cast<char const*>(front_data),
    *beg = raw;
  boost::uint32_t atom_size;
  GET_ATOM_SIZE_(raw, atom_size);
  raw += atom_size; // skip ftyp atom
  GET_ATOM_SIZE_(raw, atom_size);
  if(0 != strncmp(raw + 4, "moov", 4))
    return 0;
  // fprintf(stderr, "%x\n", atom_size);
  atom_size += raw - beg;
  // fprintf(stderr, "%x\n", atom_size);
  return atom_size;
}

std::string find_itag(json::var_t const &desc)
{
  using namespace std;

  static map<string, string> itag;
  if(itag.empty()) {
    // read from file
    ifstream fin("tube.tag", ios::binary | ios::in);
    if(!fin.is_open()) 
      throw runtime_error("Unable to open tube.tag file");
    string line;
    string key, val;
    while(getline(fin, line)) {
      stringstream tokner(line);
      tokner >> key >> val; // extract resolution, 3d
      key += "." + val;
      tokner >> val; // extract type
      key += "." + val;
      tokner >> val; // extract itag
      itag[key] = val;
    }
  }
  stringstream cvt;
  cvt << 
    cmbof(desc)["video"]["resolution"].intmax() << "." <<
    cmbof(desc)["video"]["dimension"].string() << "." <<
    cmbof(desc)["video"]["type"].string()
    ;
  auto iter = itag.find(cvt.str());
  return iter == itag.end() ? "" : iter->second;
}

int is_supported(char const *content_type, char const *uri)
{
  if(strncmp(content_type, "video/", 6) == 0) 
    content_type += 6;
  else
    return 0;

  if(strncmp(content_type, "mp4",   3) != 0 &&
     strncmp(content_type, "flv",   3) != 0 &&
     strncmp(content_type, "webm",  4) != 0)
    return 0;

  if(strncmp(uri, "http://", 7) == 0)
    uri += 7;
  return strncmp(uri, "www.youtube.com", 15) == 0;
}

} // namespace tube_utils
