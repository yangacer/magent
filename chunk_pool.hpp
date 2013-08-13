#ifndef MAGENT_CHUNK_POOL_HPP_
#define MAGENT_CHUNK_POOL_HPP_

#include <iosfwd>
#include <string>
#include <map>
#include <boost/asio/buffer.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include "json/variant.hpp"
#include "dynamic_bitset.hpp"

namespace json = yangacer::json;
namespace ipc = boost::interprocess;

class chunk_pool;

std::ostream &operator << (std::ostream &os, chunk_pool const &chkp);

class chunk_pool
{
  typedef dynamic_bitset<> bitset_t;
  friend std::ostream &operator<<(std::ostream&, chunk_pool const&);
public:
  struct chunk
  {
    chunk() : buffer(0,0), offset(0) {}
    boost::asio::mutable_buffer buffer;
    boost::intmax_t offset;
    operator void const*() const
    { return boost::asio::buffer_cast<void const*>(buffer); }
  };

  chunk_pool(json::var_t &obj_desc,
             size_t max_conn = 0, 
             size_t max_conn_per_peer = 0);
  chunk get_chunk(std::string &peer);
  void  put_chunk(std::string const &peer, chunk chk);
  void  abort_chunk(std::string const &peer, chunk chk);
  size_t running_agent() const;
  size_t estimate_concurrency() const;
  void set_connection_limit(size_t max_conn, size_t max_conn_per_peer);
  bool is_complete() const;
  bool flush_then_next();
  static size_t chunk_size();
#ifndef NDEBUG
  void dump() const;
#endif
protected:
  bool acquire_peer(std::string &peer);
  void release_peer(std::string const &peer);
  boost::intmax_t segment_size(bitset_t::size_type seg_off) const;
  boost::intmax_t segment_offset(bitset_t::size_type seg_off) const;
private:
  json::var_t &obj_desc_;
  size_t max_connection_;
  size_t max_connection_per_peer_;
  size_t running_agent_;
  ipc::file_mapping mf_;
  ipc::mapped_region mr_;
  bitset_t seg_stored_;
  bitset_t seg_mapped_;
  bitset_t chk_acquired_;
  bitset_t chk_committed_;
  bitset_t::size_type cur_seg_;
  std::map<std::string, size_t> running_cnt_;
};


#endif
