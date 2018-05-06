#include "iffdigest.h"
#include "sf2.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <string.h>

int main(int argc, char* argv[])
{
  // Check cmdline
  if(argc!=2) {
    std::cerr<<"usage: "<<argv[0]<<" <filename>\n";
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

  std::cout << "---- Split "<<argv[1] << " ----\n";
  SF2File sfFile;
  // Analyse sf2
  if(sfFile.Analyse(&iff)) {

    // Extract bare filename from input file - stolen from
    // https://stackoverflow.com/questions/8520560/get-a-file-name-from-a-path
    std::string origFilename = argv[1];
    // Remove directory if present.
    // Do this before extension removal incase directory has a period character.
    const size_t last_slash_idx = origFilename.find_last_of("\\/");
    if (std::string::npos != last_slash_idx) {
      origFilename.erase(0, last_slash_idx + 1);
    }
    // Remove extension if present.
    const size_t period_idx = origFilename.rfind('.');
    if (std::string::npos != period_idx) {
        origFilename.erase(period_idx);
    }

    // Wrap splitter around sf
    SF2FileSplitter splitter(&sfFile);

    // Write filename will be
    // <orig-sf2-name><bankNo(3)>-<presetNo(3)-<preset-name>.sf2<0>
    char* wrFileName = new char[strlen(argv[0])+1 + 3+1 + 3+1 + SF_NAME_LEN+1 +3 +1];

    // loop all presets
    SF2Hydra& hydra = sfFile.getHydra();
    for(bankPresetMapIter_t bankIter=hydra.bank_begin();
        bankIter!=hydra.bank_end();bankIter++) {
      for(presetMapIter_t presIter=hydra.preset_begin(bankIter);
          presIter!=hydra.preset_end(bankIter);presIter++) {
        // One preset per file here
        splitter.clearPresets();
        splitter.addPreset(presIter);
        // Create data for the preset
        uint32_t dataLen;
        char *dataSplit = splitter.createSplit(dataLen);
        // Build output filename
        const sfPresetHeader_t& presetHeader = hydra.getPresetHeader(presIter);
        std::string presetName;
        sf2NameToStr(presetName, presetHeader.achPresetName);
        std::replace(presetName.begin(), presetName.end(), ' ', '_');
        sprintf(wrFileName, "%03d-%03d-%s.sf2",
                //origFilename.c_str(),
                bankIter->first,
                presIter->first,
                presetName.c_str());
        // Do write to file
        int fdWrite = open(wrFileName,
                            O_CREAT | O_WRONLY | O_TRUNC,
                            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if(fdWrite >= 0) {
          write(fdWrite, dataSplit, dataLen);
          close(fdWrite);
          // we are responsible
          delete dataSplit;
        }
      }
    }
    delete wrFileName;

    retval = 0;
  }

  munmap(data, stbuf.st_size);
  close(fd);
  return retval;
}

