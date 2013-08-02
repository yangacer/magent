#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/program_options.hpp>
#include <json/json.hpp>
#include <agent/agent_v2.hpp>

namespace json = yangacer::json;

class chunk_pool
{
public:
  struct chunk
  {
    boost::asio::mutable_buffer buffer;
    boost::intmax_t offset;
  };

  chunk_pool(json::var_t const &obj_desc);
  chunk get_chunk();
  void put_chunk(chunk chk);
  void abort_chunk(chunk chk);
private:
  //odb_handle odb_handle_;
};

class getter
{
public:
  getter(boost::asio::io_service &ios, chunk_pool &cpool);
private:
  agent_v2 agent_;
};

class magent
{
  typedef boost::shared_ptr<chunk_pool> chunk_pool_ptr_type;
  typedef std::vector<boost::shared_ptr<getter> > getter_array_type;
public:
  magent(int argc, char **argv);
  static int estimate_concurrency(json::var_t const &obj_desc);
  void run();
protected:
private:
  boost::asio::io_service ios_;
  //io_service_pool service_pool_;
  chunk_pool_ptr_type chunk_pool_ptr_;
  getter_array_type getter_array_;
};

magent::magent(int argc, char**argv)
{
  namespace po = boost::program_options;

  std::string obj_desc_str, gri_addr;
  
  po::options_description opts("magent options");
  opts.add_options
    ("help,h", "Print help message")
    ("version,v", "Version information")
    ("obj-desc", po::value<std::string>(&obj_desc_str), "Object description")
    ("gri", po::value<std::string>(&gri_addr), "gri address")
    ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, opts));
  po::notify(vm);
  
  if(obj_desc_str.empty())
    throw std::invalid_argument("Miss obj-desc argument");
  if(gri_addr.empty())
    throw std::invalid_argument("Miss gri argument");

  // parse obj_desc
  json::var_t obj_desc;
  auto beg(obj_desc_str.begin()), end(obj_desc.end());
  if(!json::phrase_parse(beg, end, obj_desc)) 
    throw std::invalid_argument("Malformed obj-desc");
  
  // TODO MOVE TO chunk_pool init
  // auto content_length = mbof(obj_desc)["content_length"].test(0);

  int concurrency = estimate_concurrency(obj_desc);

  chunk_pool_ptr_.reset(new chunk_pool(obj_desc));
  getter_array_.resize(concurrency);
  for(auto i=getter_array_.begin(); i != getter_array_.end(); ++i)
    i->reset(new getter(ios_, chunk_pool));
}

void magent::run()
{
  ios_.run();
}

int main(int argc, char** argv)
{
  try {
    magent ma(argc, argv);
    ma.run();
  } catch( std::exception &e) {
    std::cerr << "terminate with exception: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
