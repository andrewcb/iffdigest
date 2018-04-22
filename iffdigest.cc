#include "iffdigest.h"
#include <algorithm>

static IFFChunkList
parseChunks(const char* mem, enum IFFFormat fmt, unsigned int len);

// compare a chunk ID (unsigned int) to a (4-letter) string.

static inline bool
ckid_cmp(iff_ckid_t id, const char* tag)
{
  return (id == iff_ckid(tag));
}

//  ----- IFFChunkList methods 

IFFChunkIterator
IFFChunkList::findNextChunk(IFFChunkIterator from, iff_ckid_t ckid)
{
  return find(++from, end(), ckid);
}

IFFChunkIterator
IFFChunkList::findChunk(iff_ckid_t ckid)
{
  return find(begin(), end(), ckid);
}

//  ----- IFFChunk methods 
//  ----- IFFChunk methods 

IFFChunk::IFFChunk(const IFFChunk& ck)
 : ckid(ck.ckid), tnull('\0'), ctype(ck.ctype), data(0), length(0) {
  if(ctype == IFF_CHUNK_DATA) {
    length=ck.length; data=ck.data;
  } else {
    subchunks = ck.subchunks;
  } 
}


void
IFFChunk::operator=(const IFFChunk& ck)
{
  ckid = ck.ckid;
  tnull = '\0';
  ctype = ck.ctype;
  switch(ctype) {
  case IFF_CHUNK_DATA:
    data = ck.data; length = ck.length; break;
  case IFF_CHUNK_LIST:
    subchunks = ck.subchunks;  data = 0; length = 0;
  }
}

static IFFChunk
parseChunk(const char* mem, enum IFFFormat fmt, unsigned int *clen)
{
  unsigned int len;
  len = u32(((unsigned int*)mem)[1], fmt);
  *clen = ((len+8)+1)&0xfffffffe;
  if(ckid_cmp(((unsigned int*)mem)[0], "LIST")) {
    IFFChunkList sc = parseChunks(mem+12, fmt, len-4);
    return IFFChunk( ((unsigned int*)mem)[2], sc);
  } else {
    return IFFChunk(*((unsigned int*)mem), mem+8, len);
  }
}

static IFFChunkList
parseChunks(const char* mem, enum IFFFormat fmt, unsigned int len)
{
  IFFChunkList result;

  while(len>0) {
    unsigned int cl;
    IFFChunk c = parseChunk(mem, fmt, &cl);
    mem+=cl; len -= cl;
    result.push_back(c);
  }
  return result;
}

IFFDigest::IFFDigest(const char* data, unsigned int dlen)
 : tnull('\0'), contents(data)
{
  if(ckid_cmp(((unsigned int*)data)[0], "FORM")) {
    ftype = IFF_FMT_IFF85;
  } 
  else if(ckid_cmp(((unsigned int*)data)[0], "RIFF")) {
    ftype = IFF_FMT_RIFF;
  } else {
    // maybe throw an exception here?
    ftype = IFF_FMT_ERROR;
    return;
  }
  unsigned int len = u32(((unsigned int*)data)[1], ftype)-4;
  if(len>dlen-12) {
    // illegal length in top level; maybe throw an exception here?
    ftype = IFF_FMT_ERROR;
    return;
  }
  fid = ((unsigned int*)data)[2];
  chunks = parseChunks(data+12, ftype, len);
}
