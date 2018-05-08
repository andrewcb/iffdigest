#include "iffdigest.h"
#include "sf2.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <string.h>

void dataOut(SF2Hydra& hydra)
{
  std::string strName;
  std::cout << "\n---- Presets found ----\n";
  // Bank loop
  for(bankPresetMapIter_t b_i=hydra.bank_begin(); b_i!=hydra.bank_end();b_i++) {
    std::cout << "Bank " << b_i->first << ":\n";
    // Preset loop
    for(presetMapIter_t p_i=hydra.preset_begin(b_i);
        p_i!=hydra.preset_end(b_i);p_i++) {
      // Display data
      const sfPresetHeader_t& ph = hydra.getPresetHeader(p_i);
      const presInfo_t& pi = hydra.getPresetInfo(p_i);
      sf2NameToStr(strName, ph.achPresetName);
      std::cout << "  Preset " << p_i->first << ": \"" << strName << "\"" <<
        " / PBAGs: " << pi.pbagInfoVec.size()<<"\n";
      std::cout << "    Mod/Gen/Inst:\n";
      for(uint32_t bagIDx=0; bagIDx<pi.pbagInfoVec.size(); bagIDx++) {
        std::cout<<"    "<<pi.pbagInfoVec[bagIDx].modIdxs.size()<<"/"<<
          pi.pbagInfoVec[bagIDx].genIdxs.size()<<"/";
        if(!pi.pbagInfoVec[bagIDx].instOrSample) {
          std::cout<<"global";
        }
        else {
          std::cout<<"\"";
          const sfInst_t& sfi =
            hydra.getInstrument(pi.pbagInfoVec[bagIDx].instOrSampleIdx);
          sf2NameToStr(strName, sfi.achInstName);
          std::cout<<strName;
          std::cout<<"\"";
        }
        std::cout<<"\n";
      }
    }
  }

  // Instrument loop
  std::cout << "\n---- Instruments found ----\n";
  for(uint16_t i=0; i<hydra.instrument_count();i++) {
    const sfInst_t& sfi = hydra.getInstrument(i);
    const instInfo_t& ii = hydra.getInstrumentInfo(i);
    sf2NameToStr(strName, sfi.achInstName);
    std::cout << "\"" << strName << "\"" <<
      " / IBAGS: " << ii.ibagInfoVec.size()<<"\n";
    std::cout << "  Mod/Gen/Samp:\n";
    for(uint32_t bagIDx=0; bagIDx<ii.ibagInfoVec.size(); bagIDx++) {
      std::cout<<"  "<<ii.ibagInfoVec[bagIDx].modIdxs.size()<<"/"<<
        ii.ibagInfoVec[bagIDx].genIdxs.size()<<"/";
        if(!ii.ibagInfoVec[bagIDx].instOrSample) {
          std::cout<<"global";
        }
        else {
          std::cout<<"\"";
          const sfSample_t& sfs =
            hydra.getSample(ii.ibagInfoVec[bagIDx].instOrSampleIdx);
          sf2NameToStr(strName, sfs.achSampleName);
          std::cout<<strName;
          std::cout<<"\"";
        }
        std::cout<<"\n";
    }
    //std::cout << "\n";
  }
}

int main(int argc, char* argv[])
{
  // Check cmdline
  if(argc<2) {
    std::cerr<<"usage: "<<argv[0]<<" filename\n";
    return -1;
  }

  // Open soundfont
  int fd = open(argv[1], O_RDONLY);
  if(fd < 0) {
    std::cerr<<argv[1]<<" could not be opened!\n";
    return -1;
  }

  // Parse soundfont
  struct stat stbuf;
  fstat(fd, &stbuf);
  char* data = (char*)mmap(0, stbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
  IFFDigest iff(data, stbuf.st_size);

  int retval = -1;

  // wrap to sf2
  std::cout << "---- Analyse sound for font in "<<argv[1] << " ----\n";
  SF2File sfFile;
  if(sfFile.Analyse(&iff)) {
    SF2Hydra& hydra = sfFile.getHydra();
    dataOut(hydra);
    retval = 0;
  }

  munmap(data, stbuf.st_size);
  close(fd);
  return retval;
}

