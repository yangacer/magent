# Agent of Multiple Source Downloading

## Requirements

- Fetch an object from multiple source(URIs)
- Stream-Oriented (sequential)
- Response for offering data according to given name and attributes. 

## Interface

- Command line: 
```magent --gri address --desc object_description``` 

## Issues

### Query
 
The agent queries GRI no mater source exists in local or not. An agent only know a pre-configured GRI (a local GRI) and does query to the GRI only. Consequently, the GRI has its own decision to forward a query to remote/neighbor/parent GRI or not. Query to a GRI can be repeated several times to keep freshness and boost availability.

### Segment order
    
Data is downloaded from different sources in sequential order of segments. The agent does **best effort** to fetch data sequentially. Though, the final words is sadly affected by peer status.

### Peer order

The agent **MUST NOT** connects to the same peer over a session. Peer information such as speed and availability SHOULD be refreshed periodically or when no peer information is available (Periodic refresh can be disabled if size of an object is too small). With sufficient information, the agent arrange priority of peer to be fetched from. Otherwise, the agent generate a randomize sequence of order. Rearrangement take place whenever peer information is refreshed. The agent can estimate can report peer information to GRI.

### Stat

The agent reports its status, i.e. data fetched, to GRI via HTTP POST. The agent **MUST** reports either complete segments or empty segments but **NOT** semi-segments. Format of status is the same with [object description](#Object.Description)

### QoS

When the agent reports its status, a GRI can feedback a QoS message of following format:

```
{
  QoS_description : "description" // foreground | background
}
```
    
### Timeout handling
 
Agent always retries to complete an object even timeout occurs. Retrying includes to reconnecting available peers and re-querying GRI(local GRI). The agent only aborts when retrying count exceeds given limit. On the other hand, clients wait for streaming agent's result can timeout whenever they want.

### Source resolution 
 
Source URIs **MUST** confirms to [RFC3986](http://tools.ietf.org/html/rfc3986). The agent can internally supports specific URI patterns and performs different source resolving.
    
### Segment size

Segment size **MUST** be greater or equal to system's page size. It **SHOULD** not be too small and introduces too many segments when large objects dominate object space.

Current policy:

```
assert( content_length > 0 );
if( content_length < 16mb ) 
  segment_size = 1mb;
else if(content_length < 128mb) 
  segment_size = 2mb;
else 
  segment_size = 4mb;
```

Note: Tail segment may have size less than segment_size. To determine whether it is complete or not, content_length is required.
 
### Object Description

```
{
  oid : object_id,
  content_type : MIMETYPE,
  content_length : object_size_in_bytes (optional),
  localhost : segment_mask (or null) 
  segment_map : [ 
    { // Run length encoded concept
      size : segement_size_in_bytes,
      count : count_of_segement_of_the_same_size,
      digest : SHA1_digets_of_a_segment
    }, ...
  ],
  sources: sources_description // see below
}
```

### Sources Description

```
{ 
  peer_URI : segment_mask
  peer_URI : segment_mask,
  ...
}
```

### Segment Mask

The agent (and GRI) uses bit mask string to indicate which segment is complete. The right most 0 or 1 is the **first** segment, vice versa. e.g. "0001" indicates the **first** segment is complete and there are four segments in total. 

### Chunk size

A chunk is defined as a partition of a segment. The agent distribute peer connections in granularity of chunk. Chunk size is fixed as 16 KB.

### Estimation of concurrency degree

```
min{ #(peer), #(chunk) } = degree(concurrency) = #(connection)
``` 
