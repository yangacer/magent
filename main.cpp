#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/program_options.hpp>
#include <json/json.hpp>
#include <agent/agent_v2.hpp>
#include <agent/log.hpp>
#include "extloader.hpp"
#include "chunk_pool.hpp"

namespace json = yangacer::json;

class magent
{
  typedef boost::shared_ptr<head_getter> head_getter_ptr_type;
  typedef boost::shared_ptr<data_getter> data_getter_ptr_type;
  typedef boost::shared_ptr<chunk_pool> chunk_pool_ptr_type;
  typedef std::vector<boost::shared_ptr<data_getter> > data_getter_array_type;
  typedef chunk_pool::chunk chunk;
public:
  magent(int argc, char **argv);
  void run();
protected:
  void heading();
  void handle_heading(boost::system::error_code const &ec, json::var_t const &head_info);
  int  estimate_concurrency() const;
  void generate_segment_map(json::var_t const &head_info);
  void check_and_init_origin();
  void prepare_subagent();
  void dispatch_agent(std::string peer = "");
  void handle_data(boost::system::error_code const &ec, 
                   std::string const &peer, 
                   chunk_pool::chunk chk);
private:
  boost::asio::io_service ios_;
  boost::program_options::variables_map vm_;
  extloader               extloader_;
  //io_service_pool service_pool_;
  json::var_t             obj_desc_;
  chunk_pool_ptr_type     chunk_pool_ptr_;
  //data_getter_array_type  data_getter_array_;
};

magent::magent(int argc, char**argv)
{
  using namespace std;
  namespace po = boost::program_options;

  string obj_desc_str, gri_addr;
  vector<string> extensions;
  po::options_description opts("magent options");

  opts.add_options()
    ("help,h", "Print help message")
    ("version,v", "Version information")
    ("obj-desc", po::value<string>(&obj_desc_str), "Object description")
    ("gri", po::value<string>(&gri_addr), "gri address")
    ("extendsion,x", po::value<vector<string> >(&extensions), "Extension modules")
    ("max-connection,c", po::value<size_t>()->composing(), "Maximum number of connections")
    ("max-conn-per-peer,C", po::value<size_t>()->composing(), "Maxiumum number of connections per peer")
    ;
  po::store(po::parse_command_line(argc, argv, opts), vm_); 
  po::notify(vm_);
  
  if(vm_.count("help")) { cout << opts << "\n"; return; }

  if(obj_desc_str.empty())
    throw invalid_argument("Miss obj-desc argument");
  if(gri_addr.empty())
    throw invalid_argument("Miss gri argument");

  // parse obj_desc
  auto beg(obj_desc_str.begin()), end(obj_desc_str.end());
  if(!json::phrase_parse(beg, end, obj_desc_)) 
    throw invalid_argument("Malformed obj-desc");
  
  if(extensions.size())
    extloader_.load(extensions);

  heading();
}

void magent::heading()
{
  //auto content_length = 
  //  mbof(obj_desc_)["content_length"].test(boost::intmax_t(0));
  auto &segment_map =
    mbof(obj_desc_)["segment_map"].test(json::array_t());

  if(segment_map.empty()) {
    assert(cmbof(obj_desc_)["sources"].object().size() == 1 
           && "multiple sources but no content length");
    
    std::string const &content_type = cmbof(obj_desc_)["content_type"].string();
    std::string const &origin = cmbof(obj_desc_)["sources"].object().begin()->first;
    
    head_getter_ptr_type hg = extloader_.create_head_getter(ios_, content_type, origin);
    if(hg)
      hg->async_head(obj_desc_, boost::bind(&magent::handle_heading, this, _1, _2));
  } else {
    prepare_subagent();  
  }
}

void magent::handle_heading(boost::system::error_code const &ec,
                            json::var_t const &head_info)
{
  if(!ec) {
    generate_segment_map(head_info);
    check_and_init_origin();
    prepare_subagent();
  } else {
    logger::instance().async_log("system_error", false, ec.message());
  }
}

#define ALIGN_(X, Base) \
  ((X + ((Base)-1)) & ~((Base)-1))

void magent::generate_segment_map(json::var_t const &head_info)
{
  auto &segment_map = mbof(obj_desc_)["segment_map"].array();
  auto content_length = cmbof(head_info)["content_length"].intmax();

  assert(content_length != 0 && "Unkonwn size");
  assert(segment_map.empty());

  mbof(obj_desc_)["content_length"] = content_length;
  auto first_seg_size = cmbof(head_info)["header_size"].intmax();
  boost::intmax_t regular_seg_size = 1u << 23; // 8Mb
  regular_seg_size = (regular_seg_size > content_length ) ?  
    content_length : regular_seg_size ;
  first_seg_size = (first_seg_size) ? ALIGN_(first_seg_size, chunk_pool::chunk_size()) : regular_seg_size;
  
  assert(regular_seg_size >= first_seg_size && regular_seg_size <= content_length);

  json::var_t seg_desc;
  if(first_seg_size != regular_seg_size) {
    mbof(seg_desc)["size"] = first_seg_size;
    mbof(seg_desc)["count"] = (boost::intmax_t)1;
    content_length -= first_seg_size;
    segment_map.push_back(seg_desc);
  }
  auto cnt = content_length / regular_seg_size;
  if(cnt) {
    mbof(seg_desc)["size"] = regular_seg_size;
    mbof(seg_desc)["count"] = cnt;
    segment_map.push_back(seg_desc);
  }
  auto last_seg_size = content_length % regular_seg_size;
  if(last_seg_size) {
    mbof(seg_desc)["size"] = last_seg_size;
    mbof(seg_desc)["count"] = (boost::intmax_t)1;
    segment_map.push_back(seg_desc);
  }
}

void magent::check_and_init_origin()
{
  auto &origin_segmap = mbof(obj_desc_)["sources"].object().begin()->second;
  if(cmbof(origin_segmap).is_null()) {
    auto &segmap = cmbof(obj_desc_)["segment_map"].array();
    assert(segmap.size());
    boost::intmax_t cnt = 0;
    for(auto i=segmap.begin(); i!=segmap.end(); ++i)
      cnt += cmbof(*i)["count"].intmax();
    mbof(origin_segmap).test(std::string()).assign(cnt, '1');
  }
}

void magent::prepare_subagent()
{
  // The chunk_pool compute what data to be fetched in terms of chunk.
  // Consecutive requests to a chunk_pool are reponsed with chunks of offsets
  // ordered from small to large but may not be sequentially.
  chunk_pool_ptr_.reset(
    new chunk_pool(obj_desc_, 
                   vm_["max-connection"].as<size_t>(),
                   vm_["max-conn-per-peer"].as<size_t>()))
    ;
  auto concurrency = chunk_pool_ptr_->estimate_concurrency();
  mbof(obj_desc_)["debug"]["concurrency"] = (boost::intmax_t)concurrency;
  dispatch_agent();

}

void magent::dispatch_agent(std::string peer)
{
  auto sources = cmbof(obj_desc_)["sources"].object();
  while(sources.size() && 
        chunk_pool_ptr_->running_agent() < 
        chunk_pool_ptr_->estimate_concurrency()) 
  {
    std::string const &content_type = cmbof(obj_desc_)["content_type"].string();
    chunk chk = chunk_pool_ptr_->get_chunk(peer);
    if(!chk) break;
    data_getter_ptr_type dg =
      extloader_.create_data_getter(ios_, content_type.c_str(), peer);
    if(dg) {
      dg->async_get(obj_desc_, peer, chk,
                    boost::bind(&magent::handle_data, this, _1, _2, _3));
    } else {
      chunk_pool_ptr_->abort_chunk(peer, chk);
    }
  }
}

void magent::handle_data(boost::system::error_code const &ec, 
                         std::string const &peer, 
                         chunk_pool::chunk chk)
{
  //std::cout << *chunk_pool_ptr_ << "\n";
  if(!ec) {
    chunk_pool_ptr_->put_chunk(peer, chk);
  } else {
    chunk_pool_ptr_->abort_chunk(peer, chk);
  }
  if(chunk_pool_ptr_->is_complete()) {
    if(chunk_pool_ptr_->flush_then_next())
      dispatch_agent(peer);
    // std::cout << *chunk_pool_ptr_ << "\n";
    // TODO report
    json::pretty_print(std::cout, obj_desc_);
  } else {
    dispatch_agent(peer);
  }
}

void magent::run()
{
  ios_.run();
}

int main(int argc, char** argv)
{
  try {
    logger::instance().use_file("magent.log");
    magent ma(argc, argv);
    ma.run();
  } catch( std::exception &e) {
    std::cerr << "terminate with exception: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
