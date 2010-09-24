#include "iffdigest.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <iostream.h>
#include <stdio.h>

void
dump_chunk_list(IFFChunkIterator from, IFFChunkIterator to, int level=0)
{
  for( ; from != to; from++ ) {
    for(int i=0; i<level; i++) putchar(' ');
    unsigned int len=(*from).len();
    const char* data=(*from).dataPtr();
    printf("%s (%d bytes) ", (*from).id_str(), len);
    if(len>0) {
      for(int i=0; i<len && i<12; i++) {
        if(data[i]>31 && data[i]<127)
          printf("'%c' ", data[i]);
        else printf("%02x  ", data[i]&0xff);
      }
    }
    putchar('\n');
    dump_chunk_list((*from).ck_begin(), (*from).ck_end(), level+1);
  }
}

main(int argc, char* argv[])
{
  if(argc<2) {
    cerr<<"usage: "<<argv[0]<<" filename\n";
    return 2;
  }
  int fd = open(argv[1], O_RDONLY);
  struct stat stbuf;
  fstat(fd, &stbuf);
  char* data = (char*)mmap(0, stbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
  IFFDigest iff(data, stbuf.st_size);
  if(iff.valid()) {
    dump_chunk_list(iff.ck_begin(), iff.ck_end(), 0);
    return 0;
  } else {
    cerr<<argv[0]<<": "<<argv[1]<<" is not a legal EA-IFF-85 or RIFF file.\n";
    return 1;
  }
}
