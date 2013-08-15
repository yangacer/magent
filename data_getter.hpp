#ifndef MAGENT_DATA_GETTER_HPP_
#define MAGENT_DATA_GETTER_HPP_

#include <agent/quality_config.hpp>
#include "json/variant.hpp"
#include "chunk_pool.hpp"

namespace json = yangacer::json;

class data_getter 
{
public:
  typedef boost::function<
    void(boost::system::error_code const&, 
         std::string const &peer,
         chunk_pool::chunk)
    > data_handler_type;

  virtual void async_get(json::var_t const  &desc,
                         std::string const  &peer,
                         chunk_pool::chunk  chk,
                         data_handler_type  hdl) = 0;
  virtual ~data_getter(){}
};

#endif
