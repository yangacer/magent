#include "extloader.hpp"
#include <stdexcept>
#include <dlfcn.h>
#include <cstdio>

// --- extloader_error impl ----

extloader_error::extloader_error(std::string const &extname)
  : msg_("Extension error: " + extname)
{}

char const *extloader_error::what() const throw()
{ return msg_.c_str(); }

// ---- extloader impl ----

#define HEAD_CTOR_ "magent_ext_create_head_getter"
#define DATA_CTOR_ "magent_ext_create_data_getter"

//! Load symbol from extension handle 

template<typename T>
T* ld_chk(void *exthandle, char const* symbol, T*&out)
{
  dlerror();
  out = (T*)dlsym(exthandle, symbol);
  if(dlerror()) out = NULL;
  return out;
}

extloader::extloader() {}

void extloader::set_generic(std::string const &extension)
{
  if(generic_.ext_handle) {
    dlclose(generic_.ext_handle);
    std::memset(&generic_, 0, sizeof(ext_symbols)); 
  }
  load(extension);
  generic_ = extension_v2_[extension];
  extension_v2_.erase(extension);
}

void extloader::load(std::string const &ext)
{
  if(extension_v2_.count(ext) != 0 ) return;
  ext_symbols es;
  void *hdl = dlopen(ext.c_str(), RTLD_LAZY | RTLD_GLOBAL);
  if(!hdl) throw extloader_error(ext);
  es.ext_handle = hdl;
  if( NULL == ld_chk(hdl, HEAD_CTOR_, es.create_head_getter ))
    throw extloader_error(ext + " did not define " HEAD_CTOR_);

  if( NULL == ld_chk(hdl, DATA_CTOR_, es.create_data_getter))
    throw extloader_error(ext + " did not define " DATA_CTOR_);
  extension_v2_[ext] = es;
}

void extloader::load(std::vector<std::string> const &ext_list)
{
  for(auto i=ext_list.begin(); i != ext_list.end(); ++i)
    load(*i);   
}

extloader::~extloader()
{
  for(auto i = extension_v2_.begin(); i != extension_v2_.end(); ++i)
    dlclose(i->second.ext_handle);
}

extloader::head_getter_ptr_type 
extloader::create_head_getter(boost::asio::io_service &ios, 
                              std::string const &content_type, 
                              std::string const &uri)
{
  head_getter_ptr_type rt;
  for(auto i = extension_v2_.begin(); i != extension_v2_.end(); ++i) {
    auto &es = i->second;
    rt = es.create_head_getter(&ios, content_type.c_str(), uri.c_str());
    if( rt ) break;
  }
  if(!rt && generic_.ext_handle) 
    rt = generic_.create_head_getter(&ios, content_type.c_str(), uri.c_str());
  return rt;
}

extloader::data_getter_ptr_type 
extloader::create_data_getter(boost::asio::io_service &ios, 
                              std::string const &content_type, 
                              std::string const &uri)
{
  data_getter_ptr_type rt;
  for(auto i = extension_v2_.begin(); i != extension_v2_.end(); ++i) {
    auto &es = i->second;
    rt = es.create_data_getter(&ios, content_type.c_str(), uri.c_str());
    if(rt)  break;
  }
  if(!rt && generic_.ext_handle) 
    rt = generic_.create_data_getter(&ios, content_type.c_str(), uri.c_str());
  return rt;
}
