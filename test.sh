#!/usr/bin/bash

rm *.log*
./mage --obj-desc '{ 
  "oid" : "Y1pS_6hErDA", 
  "content_type" : "video/mp4", 
  "attribute" : { 
    "resolution" : 720, 
    "dimension": "2d", 
    "type" : "mp4" 
  }, 
  "sources": { 
    "http://www.youtube.com/watch?v=Y1pS_6hErDA": null}  
}' --gri http://10.0.0.185:7878/hb -x ./libtube.so -c 10 -C 2
