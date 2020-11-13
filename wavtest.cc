#include "iffdigest.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

int main(int argc, char* argv[])
{
  if(argc<2) {
    std::cerr<<"usage: "<<argv[0]<<" filename\n";
    return 2;
  }
  int fd = open(argv[1], O_RDONLY);
  struct stat stbuf;
  fstat(fd, &stbuf);
  char* data = (char*)mmap(0, stbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
  IFFDigest iff(data, stbuf.st_size);
  IFFChunkIterator i=iff.ck_find(iff_ckid("fmt "));
  if(i==iff.ck_end()) {
    std::cerr<<argv[1]<<" is not a valid WAV file.\n";
    return 1;
  } else {
    const char* fmt = (*i).dataPtr();
    unsigned short wformat = iff_u16_le(fmt);
    unsigned short wchannels = iff_u16_le(fmt+2);
    unsigned int smplrate = iff_u32_le(fmt+4);
    std::cout<<argv[1]<<": format "<<wformat<<", "<<wchannels<<" channels, "<<smplrate<<" Hz";
    if(wformat == 1) { // PCM file
      std::cout<<", PCM, "<<(iff_u16_le(fmt+14))<<" bits/sample";
    }
    std::cout<<'\n';
  }
  munmap(data, stbuf.st_size);
  close(fd);
  return 0;
}

