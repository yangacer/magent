#ifndef MAGENT_EXTENSION_HPP_
#define MAGENT_EXTENSION_HPP_

#include <boost/shared_ptr.hpp>
#include <boost/asio/io_service.hpp>

extern "C" {

struct head_getter;
struct data_getter;

//! Retern non zero if extension module supports the NULL terminated uri.
int magent_ext_support_head(char const* content_type, 
                            char const *uri);
int magent_ext_support_data(char const *content_type,
                            char const *uri);

boost::shared_ptr<head_getter> 
magent_ext_create_head_getter(boost::asio::io_service &ios);

typedef int has_getter_t(char const *, char const *);
typedef boost::shared_ptr<head_getter> head_getter_create_t(void*);
typedef boost::shared_ptr<data_getter> data_getter_create_t(void*);

#define MAGENT_EXT_HEAD_GETTER_IMPL(Ext) \
extern "C" boost::shared_ptr<head_getter> \
  magent_ext_create_head_getter(boost::asio::io_service &ios) \
{ return boost::shared_ptr<head_getter>(new Ext(ios)); } 

#define MAGENT_EXT_DATA_GETTER_IMPL(Ext) \
extern "C" boost::shared_ptr<data_getter> \
  magent_ext_create_data_getter(boost::asio::io_service &ios) \
{ return boost::shared_ptr<data_getter>(new Ext(ios)); } 

}

#endif
