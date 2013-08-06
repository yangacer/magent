#include <iostream>
#include <boost/bind.hpp>
#include "tube_head_getter.hpp"
#include "json/json.hpp"

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
  boost::asio::io_service ios;
  json::var_t desc;

  std::string input(argv[1]);
  auto beg(input.begin()), end(input.end());
 
  if(!json::phrase_parse(beg, end, desc))
    return 1;

  boost::shared_ptr<head_getter> hg(new tube_head_getter(ios));
  hg->async_head(desc, boost::bind(&handler, _1, _2));

  ios.run();

  return 0;
}
