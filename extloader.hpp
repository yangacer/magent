#ifndef MAGENT_EXTLOADER_HPP_
#define MAGENT_EXTLOADER_HPP_

#include <vector>
#include <map>
#include <string>
#include <exception>
#include <boost/shared_ptr.hpp>
#include <boost/asio/io_service.hpp>
#include "extension.hpp"
#include "head_getter.hpp"

struct extloader_error : std::exception
{
  extloader_error(std::string const &extname);
  ~extloader_error() throw() {}
  virtual char const *what() const throw();
private:
  std::string msg_;
};

class extloader
{
  typedef void* ext_handle_type;
public:
  typedef boost::shared_ptr<head_getter> head_getter_ptr_type;
  extloader();
  void load(std::string const &extension);
  void load(std::vector<std::string> const &ext_list);
  ~extloader();
  head_getter_ptr_type create_head_getter(
    boost::asio::io_service &ios,
    std::string const &content_type, 
    std::string const &uri);
private:
  std::map<std::string, ext_handle_type> extension_;
};

#endif
