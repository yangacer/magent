#ifndef MAGENT_EXTENSION_HPP_
#define MAGENT_EXTENSION_HPP_

#include <boost/shared_ptr.hpp>
#include <boost/asio/io_service.hpp>

extern "C" {

struct head_getter;
struct data_getter;

typedef boost::shared_ptr<head_getter> head_getter_create_t(void*, char const*, char const*);
typedef boost::shared_ptr<data_getter> data_getter_create_t(void*, char const*, char const*);

#define MAGENT_EXT_HEAD_GETTER_IMPL(Ext, HasFunction) \
extern "C" boost::shared_ptr<head_getter> \
  magent_ext_create_head_getter(boost::asio::io_service &ios, char const *ctype, char const *uri) \
{ \
  return (HasFunction(ctype, uri)) ? \
    boost::shared_ptr<head_getter>(new Ext(ios)) \
    : boost::shared_ptr<head_getter>(); \
} 

#define MAGENT_EXT_DATA_GETTER_IMPL(Ext, HasFunction) \
extern "C" boost::shared_ptr<data_getter> \
  magent_ext_create_data_getter(boost::asio::io_service &ios, char const *ctype, char const *uri) \
{ \
  return (HasFunction(ctype, uri)) ? \
    boost::shared_ptr<data_getter>(new Ext(ios)) \
    : boost::shared_ptr<data_getter>(); \
} 

}

#endif
