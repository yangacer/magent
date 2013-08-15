#include "chunk_pool.hpp"
#include <algorithm>
#include <string>
#include <json/accessor.hpp>
#include <ostream>
#include <fstream>
#include <boost/filesystem.hpp>

chunk_pool::chunk_pool(
  json::var_t &obj_desc,
  size_t max_conn, 
  size_t max_conn_per_peer)
: obj_desc_(obj_desc), 
  max_connection_(max_conn),
  max_connection_per_peer_(max_conn_per_peer),
  running_agent_(0),
  cur_seg_(0)
{
  auto &local_segmap = 
    mbof(obj_desc_)["localhost"].test(std::string());
  if(local_segmap.empty()) {
    auto &segmap = cmbof(obj_desc_)["segment_map"].array();
    boost::intmax_t cnt = 0;
    for(auto i=segmap.begin(); i!=segmap.end(); ++i)
      cnt += cmbof(*i)["count"].intmax();
    local_segmap.assign(cnt, '0');
  }
  new (&seg_stored_) bitset_t(local_segmap);
  cur_seg_ = seg_stored_.find_first(false);
  seg_mapped_.resize(seg_stored_.size());

  auto content_length =
    cmbof(obj_desc_)["content_length"].intmax();
  auto const &oid =
    cmbof(obj_desc_)["oid"].string();
  
  { 
    std::ofstream ofs(oid); 
    if(!ofs.is_open()) throw std::runtime_error(oid);
    ofs.close();
  }

  if(content_length)
    boost::filesystem::resize_file(oid, content_length);
  mf_ = ipc::file_mapping(oid.c_str(), ipc::read_write);
  auto &sources = cmbof(obj_desc_)["sources"].object();
  for(auto i=sources.begin();i!=sources.end(); ++i)
    running_cnt_[i->first] = 0;
}

chunk_pool::~chunk_pool()
{}

bool chunk_pool::acquire_peer(std::string &peer)
{
  if(running_agent_ > max_connection_)
    return false;
  auto prefer = running_cnt_.find(peer);
  if(prefer != running_cnt_.end() &&
     prefer->second < max_connection_per_peer_ ) 
  {
    prefer->second++;
    running_agent_++;
    return true;
  }
  for(auto i=running_cnt_.begin(); i != running_cnt_.end(); ++i) {
    if(i->second < max_connection_per_peer_ ) {
      peer = i->first;
      i->second++;
      running_agent_++;
      return true;
    }
  }
  return false;
}

void chunk_pool::release_peer(std::string const &peer)
{
  assert(running_cnt_.count(peer));
  running_cnt_[peer]--;
  running_agent_--;
}

boost::intmax_t chunk_pool::segment_size(bitset_t::size_type seg_off) const
{
  auto &segmap = cmbof(obj_desc_)["segment_map"].array();
  auto i= segmap.begin();
  for(; i!=segmap.end(); ++i) {
    auto cnt = (decltype(seg_off))cmbof(*i)["count"].intmax();
    if(seg_off < cnt) break;
    seg_off -= cnt;
  }
  return cmbof(*i)["size"].intmax();
}

boost::intmax_t chunk_pool::segment_offset(bitset_t::size_type seg_off) const
{
  boost::intmax_t rt(0);
  for(auto i = seg_off - seg_off; i < seg_off; ++i) 
    rt += segment_size(i);
  return rt;
}

chunk_pool::chunk chunk_pool::get_chunk(std::string &peer)
{
  typedef boost::asio::mutable_buffer mbuf_t;
  // XXX Assume cur_seg_ is only incresed when previous one has been
  // completed
  //
  assert(cur_seg_ < seg_stored_.size());

  auto seg_size = segment_size(cur_seg_);
  auto seg_offset = segment_offset(cur_seg_);

  if(!seg_mapped_[cur_seg_]) {
    assert(seg_size > 0);
    mr_ = ipc::mapped_region(mf_, ipc::read_write, seg_offset, seg_size);
    seg_mapped_.set(cur_seg_);
    auto chk_num = seg_size / chunk_pool::chunk_size();
    chk_num += (seg_size % chunk_pool::chunk_size()) ? 1 : 0;
    chk_acquired_.resize(chk_num);
    chk_acquired_.reset();
    chk_committed_.resize(chk_num);
    chk_committed_.reset();
  }
  // determine which chunk to get
  bitset_t::size_type pos = chk_committed_.find_first(false);
  while( bitset_t::npos != pos &&
         chk_acquired_[pos] == true &&
         (pos = chk_committed_.find_next(pos, false)));
  
  if( pos == bitset_t::npos )
    return chunk();
  assert(chk_committed_[pos] == false);

  if( !acquire_peer(peer) ) 
    return chunk();   

  chk_acquired_.set(pos);
  chunk rt;
  
  auto chk_byte_off = pos * chunk_pool::chunk_size();
  auto chk_size = chunk_pool::chunk_size();

  rt.offset = seg_offset + chk_byte_off;
  chk_size = (rt.offset + chk_size > seg_offset + seg_size) ?
    seg_size % chk_size : chk_size
    ;
  assert(chk_size > 0);
  rt.buffer = mbuf_t((char*)mr_.get_address() + chk_byte_off, chk_size);
  return rt; 
}

void chunk_pool::put_chunk(std::string const &peer, chunk chk)
{
  auto chk_byte_off = (chk.offset - segment_offset(cur_seg_))/chunk_pool::chunk_size();
  chk_acquired_[chk_byte_off] = false;
  chk_committed_[chk_byte_off] = true;
  release_peer(peer);
}

void chunk_pool::abort_chunk(std::string const &peer, chunk chk)
{
  auto chk_byte_off = (chk.offset - segment_offset(cur_seg_))/chunk_pool::chunk_size();
  chk_acquired_[chk_byte_off] = false;
  release_peer(peer);
}

std::ostream &operator << (std::ostream &os, chunk_pool const &chkp)
{
  os << "Segment status: \n";
  os << "====\n";
  os << "sto " << chkp.seg_stored_ << "\n";
  os << "map " << chkp.seg_mapped_ << "\n\n";
  os << "Chunk status of mapped segments:\n";
  os << "====\n";
  os << "acq " << chkp.chk_acquired_ << "\n";
  os << "com " << chkp.chk_committed_ << "\n";

  return os;
}

#ifndef NDEBUG
#include <iostream>
void chunk_pool::dump() const
{
  std::cout << *this;
}
#endif 

size_t chunk_pool::running_agent() const
{
  return running_agent_;
}

size_t chunk_pool::estimate_concurrency() const 
{
  return std::min(
    cmbof(obj_desc_)["sources"].object().size() * max_connection_per_peer_,
    max_connection_)
    ;
}
  
void chunk_pool::set_connection_limit(
  size_t max_conn, 
  size_t max_conn_per_peer)
{
  max_connection_ = max_conn;
  max_connection_per_peer_ = max_conn_per_peer;
}

bool chunk_pool::is_complete() const
{
  return bitset_t::npos == chk_committed_.find_first(false);
}

bool chunk_pool::flush_then_next() 
{
  if(!mr_.flush(0, mr_.get_size(), false))
    return false;

  seg_stored_[cur_seg_] = true;
  seg_mapped_[cur_seg_] = false;
  boost::to_string(seg_stored_, mbof(obj_desc_)["localhost"].string());
  cur_seg_ = seg_stored_.find_next(cur_seg_, false);
  return  cur_seg_ != bitset_t::npos;
}

size_t chunk_pool::chunk_size()
{
  return ipc::mapped_region::get_page_size() << 9;
}
