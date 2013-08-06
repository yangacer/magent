#include <iostream>
#include <boost/bind.hpp>
#include "agent/log.hpp"
#include "json/json.hpp"
#include "extloader.hpp"
#include "head_getter.hpp"

namespace json = yangacer::json;

void handler(boost::system::error_code const &ec,
             json::var_t const &info)
{
  if(!ec) {
    json::pretty_print(std::cout, info);
  } else {
    std::cerr << "Error: " << ec.message() << "\n";
  }
}

int main(int argc, char **argv)
{
  if(argc < 3 ) {
    std::cout << "Usage: extloader <extension> <obj_desc>\n";
    return 0;
  }
  
  logger::instance().use_file("extloader.log");

  extloader loader;
  loader.load(argv[1]);

  boost::asio::io_service ios;
  json::var_t desc;

  std::string input(argv[2]);
  auto beg(input.begin()), end(input.end());
 
  if(!json::phrase_parse(beg, end, desc))
    return 1;

  auto hg = loader.create_head_getter(ios, "video/mp4", "www.youtube.com");
  if(!hg) {
    std::cerr << "Unsupported getter\n";
    return 1;
  }

  hg->async_head(desc, boost::bind(&handler, _1, _2));

  ios.run();

  return 0;
}
