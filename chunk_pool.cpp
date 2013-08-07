#include "chunk_pool.hpp"
#include <json/accessor.hpp>

chunk_pool::chunk_pool(json::var_t &obj_desc)
: obj_desc_(obj_desc)
{
  auto content_length = cmbof(obj_desc_)["content_length"].intmax();
  auto local_peer = 
    mbof(obj_desc_)["sources"]["localhost"].test(json::array_t());
  
}

void chunk_pool::generate_segment_map()
{
  // There are X cases to be consider when do segmentation
  // 1. "sources" contains only origin peer 
  //  1.1 origin peer is segmented 
  //    Somebody has head and segment it but has not fetch any data
  //  1.2 origin peer is not segmented
  //    We have to segment it on our own.
  // 2. "sources" contains multiple peers
  //  2.1 
}
