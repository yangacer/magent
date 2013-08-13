#include "extloader.hpp"
#include <stdexcept>
#include <dlfcn.h>

extloader_error::extloader_error(std::string const &extname)
  : msg_("Extension error: " + extname)
{}

char const *extloader_error::what() const throw()
{ return msg_.c_str(); }

extloader::extloader()
{}

void extloader::load(std::string const &ext)
{
  ext_handle_type hdl = dlopen(ext.c_str(), RTLD_LAZY | RTLD_GLOBAL);

  if(!hdl) throw extloader_error(ext);
  if(extension_.count(ext) == 0 ) 
    extension_[ext] = hdl;
}

void extloader::load(std::vector<std::string> const &ext_list)
{
  for(auto i=ext_list.begin(); i != ext_list.end(); ++i)
    load(*i);   
}

extloader::~extloader()
{
  for(auto i = extension_.begin(); i != extension_.end(); ++i)
    dlclose(i->second);
}

#define SUP_HEAD_  "magent_ext_support_head"
#define SUP_DATA_  "magent_ext_support_data"
#define HEAD_CTOR_ "magent_ext_create_head_getter"
#define DATA_CTOR_ "magent_ext_create_data_getter"

/** Load symbol from extension handle and throw
 *  if any error occurs.
 *  @return Dereferenced symbol.
 */
template<typename T>
T* ld_chk(void *exthandle, char const *symbol)
{
  dlerror();
  T* rt = (T*)dlsym(exthandle, symbol);
  if(dlerror()) return NULL;
  return rt;
}

extloader::head_getter_ptr_type 
extloader::create_head_getter(boost::asio::io_service &ios, 
                              std::string const &content_type, 
                              std::string const &uri)
{
  // TODO Preload has_support function to reduce loading time.
  head_getter_ptr_type rt;
  for(auto i = extension_.begin(); i != extension_.end(); ++i) {
    auto has_support = ld_chk<has_getter_t>(i->second, SUP_HEAD_);
    if( has_support(content_type.c_str(), uri.c_str()) ) {
      auto creator = ld_chk<head_getter_create_t>(i->second, HEAD_CTOR_);
      rt = creator(&ios); 
      break;
    }
  }
  return rt;
}

extloader::data_getter_ptr_type 
extloader::create_data_getter(boost::asio::io_service &ios, 
                              std::string const &content_type, 
                              std::string const &uri)
{
  // TODO Preload has_support function to reduce loading time.
  data_getter_ptr_type rt;
  for(auto i = extension_.begin(); i != extension_.end(); ++i) {
    auto has_support = ld_chk<has_getter_t>(i->second, SUP_DATA_);
    if( has_support(content_type.c_str(), uri.c_str()) ) {
      auto creator = ld_chk<data_getter_create_t>(i->second, DATA_CTOR_);
      rt = creator(&ios); 
      break;
    }
  }
  return rt;
}
