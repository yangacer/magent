#!/usr/bin/bash

rm *.log*
./mage --obj-desc '{
  "attribute" : 
  {
    "dimension" : "2d",
    "resolution" : 720,
    "type" : "mp4"
  },
  "content_length" : 48744914,
  "content_type" : "video\/mp4",
  "localhost" : "0000000",
  "oid" : "Y1pS_6hErDA.720.mp4",
  "segment_map" : [

    {
      "count" : 1,
      "size" : 2097152
    },

    {
      "count" : 5,
      "size" : 8388608
    },

    {
      "count" : 1,
      "size" : 4704722
    }
  ],
  "sources" : 
  {
    "http://www.youtube.com/watch?v=Y1pS_6hErDA" : "1111111",
    "http://10.0.0.185/Y1pS_6hErDA.mp4" : "1111111",
    "http://10.0.0.170/Y1pS_6hErDA.mp4" : "1111111",
    "http://10.0.0.185:7878/static" : "1111111"
  }
}' --gri http://10.0.0.185:7878/hb -x ./libtube.so -g ./libgeneric.so -c 12 -C 3
