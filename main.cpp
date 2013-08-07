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

class peer_pool
{
public:
  peer_pool(json::var_t const& obj_desc);
  //http::entity::url register_peer(chunk chk);
  //void unregister_peer(http::entity::url const &url);
  //void invalid_peer(http::entity::url const &url);
};

class data_getter : agent_v2
{
public:
  data_getter(boost::asio::io_service &ios);
  void async_get(/**/);
protected:
  void handle_request(/**/);
};

class magent
{
  typedef boost::shared_ptr<head_getter> head_getter_ptr_type;
  typedef boost::shared_ptr<peer_pool>  peer_pool_ptr_type;
  typedef boost::shared_ptr<chunk_pool> chunk_pool_ptr_type;
  typedef std::vector<boost::shared_ptr<data_getter> > data_getter_array_type;
public:
  magent(int argc, char **argv);
  void run();
protected:
  void heading();
  void handle_heading(boost::system::error_code const &ec, json::var_t const &head_info);
  int  estimate_concurrency() const;
  void start_subagent();
  void configure_peer_pool();
  
private:
  boost::asio::io_service ios_;
  extloader               extloader_;
  //io_service_pool service_pool_;
  json::var_t             obj_desc_;
  peer_pool_ptr_type      peer_pool_ptr_;
  chunk_pool_ptr_type     chunk_pool_ptr_;
  data_getter_array_type  data_getter_array_;
  int                     max_connection_;
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
    ("max-connection,c", po::value<int>(&max_connection_)->default_value(5), "Maximum number of connections")
    ;
  po::variables_map vm; po::store(po::parse_command_line(argc, argv, opts),
                                  vm); po::notify(vm);
  
  if(vm.count("help")) { cout << opts << "\n"; return; }

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
  // input: obj_desc(for extra info such as resolution, ..., etc.)
  
  /*
  int concurrency = estimate_concurrency(obj_desc);

    */
}

void magent::heading()
{
  //auto content_length = 
  //  mbof(obj_desc_)["content_length"].test(boost::intmax_t(0));
  auto segment_map =
    mbof(obj_desc_)["segment_map"].test(json::array_t());

  if(segment_map.empty()) {
    assert(cmbof(obj_desc_)["sources"].object().size() == 1 && "multiple sources but no content length");
    
    std::string const &content_type = cmbof(obj_desc_)["content_type"].string();
    std::string const &origin = cmbof(obj_desc_)["sources"].object().begin()->first;
    
    head_getter_ptr_type hg = extloader_.create_head_getter(ios_, content_type, origin);
    if(hg)
      hg->async_head(obj_desc_, boost::bind(&magent::handle_heading, this, _1, _2));
  } else {
    start_subagent();  
  }
}

void magent::handle_heading(boost::system::error_code const &ec,
                            json::var_t const &head_info)
{
  if(!ec) {
    mbof(obj_desc_).object().erase("content_length");
    mbof(obj_desc_).object().insert(
      cmbof(head_info).object().begin(), 
      cmbof(head_info).object().end());
    json::pretty_print(std::cout, obj_desc_);
    start_subagent();
  } else {
    logger::instance().async_log("system_error", false, ec.message());
  }
}

int magent::estimate_concurrency() const
{
  return std::min(
    cmbof(obj_desc_)["sources"].cast<int>(),
    max_connection_)
    ;
}

void magent::start_subagent()
{
  // The chunk_pool compute what data to be fetched in terms of chunk.
  // Consecutive requests to a chunk_pool are reponsed with chunks of offsets
  // ordered from small to large but may not be sequentially.
  chunk_pool_ptr_.reset(new chunk_pool(obj_desc_));
  // The peer_pool keeps record of which peers is being connected and offer
  // interfaces to register/unregister/invalid peers.
  // peer_pool_ptr_.reset(new peer_pool(obj_desc_));
  data_getter_array_.resize(estimate_concurrency());
  //for(auto i=data_getter_array_.begin(); i != data_getter_array_.end(); ++i)
  //  i->reset(new data_getter(ios_));
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
