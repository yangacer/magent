#ifndef MAGENT_CHUNK_POOL_HPP_
#define MAGENT_CHUNK_POOL_HPP_

#include <set>
#include <boost/asio/buffer.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include "json/variant.hpp"

namespace json = yangacer::json;
namespace ipc = boost::interprocess;

class chunk_pool
{
public:
  struct chunk
  {
    boost::asio::mutable_buffer buffer;
    boost::intmax_t offset;
  };

  chunk_pool(json::var_t &obj_desc);
  //chunk get_chunk();
  //void  put_chunk(chunk chk);
  //void  abort_chunk(chunk chk);
protected:
  void generate_segment_map();
private:
  json::var_t &obj_desc_;
  std::set<boost::intmax_t> seg_to_fetch_;
  ipc::file_mapping mf_;
  ipc::mapped_region mr_;
  boost::intmax_t first_seg_size_;
  boost::intmax_t regular_seg_size_;
};


#endif
