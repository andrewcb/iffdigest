#include "iffdigest.h"
#include "sf2.h"
#include <string.h>
#include <iostream>

////////////////////////////////////////////////////////////////////////////////
// SF2FileSplitter members
void SF2FileSplitter::clearPresets()
{
  presetList.clear();
}

void SF2FileSplitter::addPreset(presetMapIter_t preset)
{
  presetList.push_back(preset);
}

char *SF2FileSplitter::createSplit(uint32_t &len)
{
  // Clear working variables
  pbagIdxForPHDRList.clear();
  pbagList.clear();
  pmodIdxList.clear();
  pgenIdxList.clear();
  instIdxList.clear();
  instIdxMapper.clear();
  ibagIdxForINSTList.clear();
  ibagList.clear();
  imodIdxList.clear();
  igenIdxList.clear();
  shdrIdxList.clear();
  shdrIdxMapper.clear();
  sampleAreas.clear();
  sampleCountTotal = 0;

  SF2Hydra& hydra = SF2SourceFile->getHydra();
  std::cout << "\nCreate soundfont for the following presets:\n";
  for(std::list<presetMapIter_t>::iterator presetListIter=presetList.begin();
      presetListIter!=presetList.end(); ++presetListIter) {
    const sfPresetHeader_t& presetHeader = hydra.getPresetHeader(*presetListIter);
    std::string presetName;
    sf2NameToStr(presetName, presetHeader.achPresetName);
    std::cout << presetName << "\n";
  }
  std::cout << "\n";

  // 1. Collect required hydra chunks and align indexes
  prepareSplit();
  // 2. Dry run - just calc length / no data write
  len =  getLenOrWriteData(0);
  char *data = new char[len];
  memset(data, 0, len);
  // 3. Actally write data to buffer
  getLenOrWriteData(data);

  return data;
}

void SF2FileSplitter::addSampleAreaForSHDR(uint32_t shdrIdx)
{
  // Maybe this can be enhanced later to avoid redundancies e.g by handling
  // overlapping sample areas. Currently a sample area is detected as already
  // inserted if it has the same start/end indexes.
  SF2Hydra& hydra = SF2SourceFile->getHydra();
  // sample area not yet inserted?
  bool bInsertNow = false;
  const sfSample_t& sample = hydra.getSample(shdrIdx);
  std::map<uint32_t, std::map<uint32_t, uint32_t>>::iterator startIter;
  startIter = sampleAreas.find(sample.dwStart);
  // start found: Check if end not yet inserted
  if(startIter != sampleAreas.end()) {
    std::map<uint32_t, uint32_t> &endMap = startIter->second;
    if(endMap.find(sample.dwEnd) == endMap.end()) {
      bInsertNow = true;
    }
  }
  // start not found: insert
  else {
    bInsertNow = true;
  }
  if(bInsertNow) {
    // keep start/end key with offset to align SHDR created later
    sampleAreas[sample.dwStart][sample.dwEnd] = sample.dwStart-sampleCountTotal;
    uint32_t sampleCount = sample.dwEnd - sample.dwStart;
    sampleCountTotal += (sampleCount + 46);
  }
}

void SF2FileSplitter::prepareSplit()
{
  SF2Hydra& hydra = SF2SourceFile->getHydra();
  // Iterate all presets added..
  for(std::list<presetMapIter_t>::iterator presetListIter=presetList.begin();
      presetListIter!=presetList.end(); ++presetListIter) {

    // Keep new PBAG index for PHDR
    pbagIdxForPHDRList.push_back(pbagList.size());

    // Get preset info for pbags
    const presInfo_t& presInfo = hydra.getPresetInfo(*presetListIter);
    uint32_t pbagCount = presInfo.pbagInfoVec.size();
    // Loop preset's PBAG-zone
    for(uint32_t pbagIDx=0; pbagIDx<pbagCount; pbagIDx++) {
      sfPresetBag_t PBAG;
      // set new indexes and keep PBAG
      PBAG.wModNdx = pmodIdxList.size();
      PBAG.wGenNdx = pgenIdxList.size();
      pbagList.push_back(PBAG);

      const bagInfo_t &pbagInfo = presInfo.pbagInfoVec[pbagIDx];
      // Keep PMOD-zone indexes
      for(std::list<uint16_t>::const_iterator pmodIter = pbagInfo.modIdxs.begin();
          pmodIter!=pbagInfo.modIdxs.end(); ++pmodIter) {
        pmodIdxList.push_back(*pmodIter);
      }
      // Keep PGEN-zone indexes
      for(std::list<uint16_t>::const_iterator pgenIter = pbagInfo.genIdxs.begin();
          pgenIter!=pbagInfo.genIdxs.end(); ++pgenIter) {
        pgenIdxList.push_back(*pgenIter);
      }

      // Instrument?
      if(pbagInfo.instOrSample) {
        uint16_t instIdx = pbagInfo.instOrSampleIdx;
        // instrument not yet used?
        if(instIdxMapper.find(instIdx) == instIdxMapper.end()) {
          // Keep instrument indexes old/new
          instIdxMapper[instIdx] = instIdxList.size();
          instIdxList.push_back(instIdx);
          // Keep new IBAG index for INST
          ibagIdxForINSTList.push_back(ibagList.size());
          // Get instrument info for ibags
          const instInfo_t& instInfo = hydra.getInstrumentInfo(instIdx);
          uint32_t ibagCount = instInfo.ibagInfoVec.size();
          // Loop instruments's IBAG-zone
          for(uint32_t ibagIDx=0; ibagIDx<ibagCount; ibagIDx++) {
            sfInstBag_t IBAG;
            // set new indexes and keep IBAG
            IBAG.wInstModNdx = imodIdxList.size();
            IBAG.wInstGenNdx = igenIdxList.size();
            ibagList.push_back(IBAG);

            const bagInfo_t &ibagInfo = instInfo.ibagInfoVec[ibagIDx];
            // Keep IMOD-zone indexes
            for(std::list<uint16_t>::const_iterator imodIter = ibagInfo.modIdxs.begin();
                imodIter!=ibagInfo.modIdxs.end(); ++imodIter) {
              imodIdxList.push_back(*imodIter);
            }
            // Keep IGEN-zone indexes
            for(std::list<uint16_t>::const_iterator igenIter = ibagInfo.genIdxs.begin();
                igenIter!=ibagInfo.genIdxs.end(); ++igenIter) {
              igenIdxList.push_back(*igenIter);
            }

            // Sample?
            if(ibagInfo.instOrSample) {
              uint16_t shdrIdx = ibagInfo.instOrSampleIdx;
              // sample not yet used?
              if(shdrIdxMapper.find(shdrIdx) == shdrIdxMapper.end()) {
                // Keep sample header index old/new
                shdrIdxMapper[shdrIdx] = shdrIdxList.size();
                shdrIdxList.push_back(shdrIdx);
                // Keep sample area
                addSampleAreaForSHDR(shdrIdx);

                // Ignore wSampleLink - in fluid3gm.sf2 Sample 'P200 Piano C10(L)'
                // wSampleLink = 0 which links to 'Gun'??. Have no idea what magic
                // polyphone uses to display correct link...

                // All samples except mono types are linked
                /*if(hydra.getSample(shdrIdx).sfSampleType > 1) {
                  // Add linked samples too
                  uint16_t shdrIdxLinked = hydra.getSample(shdrIdx).wSampleLink;
                  if(shdrIdxMapper.find(shdrIdxLinked) == shdrIdxMapper.end()) {
                    // Keep sample header index old/new
                    shdrIdxMapper[shdrIdxLinked] = shdrIdxList.size();
                    shdrIdxList.push_back(shdrIdxLinked);
                    // Keep sample area
                    addSampleAreaForSHDR(shdrIdxLinked);
                  }
                }*/
              }
            }
          }
        }
      }
    }
  }
}

uint32_t SF2FileSplitter::getLenOrWriteData(char* data)
{
  SF2Hydra& hydra = SF2SourceFile->getHydra();
  IFFChunk *infoChunk = SF2SourceFile->getInfo();
  SF2Samples &samples = SF2SourceFile->getSamples();
  uint32_t len = 0;

  // RIFF header
  if(data) {
    memcpy(data+len, "RIFF", 4);
  }
  len += 4;
  // total len set below
  len += 4;
  // sfbk id
  if(data) {
    memcpy(data+len, "sfbk", 4);
  }
  len += 4;

  // INFO chunk: 1:1 copy
  infoChunk->writeData(len, data, IFF_FMT_RIFF);

  // Fill sdta/SHDR data (with aligened indexes)
  // Note: We deny ROM samples -> smpl is mandatory (sm24 optional)
  uint32_t lenSHDR = (shdrIdxList.size()+1/*terminal*/);
  sfSample_t* dataSHDR = 0;
  int16_t* dataSmpl = 0;
  int8_t* dataSm24 = 0;
  if(data) {
    dataSHDR = new sfSample_t[lenSHDR];
    dataSmpl = new int16_t[sampleCountTotal];
    memset(dataSmpl, 0, sampleCountTotal*sizeof(int16_t));
    if(samples.sm24()) {
      dataSm24 = new int8_t[sampleCountTotal];
      memset(dataSm24, 0, sampleCountTotal);
    }
    typedef std::list<uint16_t>::iterator idx16Iterator;
    // Loop all SHDRs to be exported
    for(idx16Iterator shdrIter=shdrIdxList.begin();
        shdrIter!=shdrIdxList.end();
        shdrIter++) {
      // Get copy of source SHDR
      uint16_t sourceSHDRIdx = *shdrIter;
      const sfSample_t &sourceSHDREntry = hydra.getSample(sourceSHDRIdx);
      // Reference to destination SHDR
      assert(shdrIdxMapper.find(sourceSHDRIdx) != shdrIdxMapper.end());
      uint16_t destSHDRIdx = shdrIdxMapper[sourceSHDRIdx];
      assert(destSHDRIdx < lenSHDR + 1 /*terminal*/);
      sfSample_t &destSHDREntry = dataSHDR[destSHDRIdx];
      // copy
      destSHDREntry = sourceSHDREntry;
      // Align sample indexes
      uint32_t sampleOffset =
        sampleAreas[destSHDREntry.dwStart][destSHDREntry.dwEnd]; // assertion?
      destSHDREntry.dwStart -= sampleOffset;
      destSHDREntry.dwEnd -= sampleOffset;
      destSHDREntry.dwStartloop -= sampleOffset;
      destSHDREntry.dwEndloop -= sampleOffset;
      // copy samples
      memcpy(dataSmpl+destSHDREntry.dwStart,
              samples.smpl()->dataPtr() + sourceSHDREntry.dwStart*sizeof(int16_t),
              (destSHDREntry.dwEnd - destSHDREntry.dwStart)*sizeof(int16_t));
      if(samples.sm24()) {
        memcpy(dataSm24+destSHDREntry.dwStart,
                samples.sm24()->dataPtr() + sourceSHDREntry.dwStart,
                destSHDREntry.dwEnd - destSHDREntry.dwStart);
      }
    }
    // SHDR terminal sample
    sfSample_t &destTerminalSHDREntry = dataSHDR[lenSHDR-1];
    memcpy(&destTerminalSHDREntry.achSampleName, "EOS", 3);
  }
  // Setup sdta chunk with smpl/sm24 sub chunks
  IFFChunkList sdtaSubChunks;
  sdtaSubChunks.push_back(IFFChunk(iff_ckid("smpl"),
    (char*)dataSmpl,
    sampleCountTotal*2));
  if(samples.sm24()) {
    sdtaSubChunks.push_back(IFFChunk(iff_ckid("sm24"),
      (char*)dataSm24,
      sampleCountTotal));
  }
  // make sdta chunk and write data
  IFFChunk sdta(iff_ckid("sdta"), sdtaSubChunks);
  sdta.writeData(len, data, IFF_FMT_RIFF);

  // Create pdta (hydra) chunks

  // PHDR data
  sfPresetHeader_t *dataPHDR = 0;
  uint32_t lenPHDR = (presetList.size()+1/*terminal*/);
  if(data) {
    dataPHDR = new sfPresetHeader_t[lenPHDR];
    assert(presetList.size() == pbagIdxForPHDRList.size());
    std::list<presetMapIter_t>::iterator presetListIter;
    std::list<uint16_t>::iterator bagIdxIter;
    uint16_t headerCount;
    for(presetListIter=presetList.begin(), bagIdxIter=pbagIdxForPHDRList.begin(), headerCount=0;
        presetListIter!=presetList.end();
        presetListIter++, bagIdxIter++, headerCount++) {
      const sfPresetHeader_t& presetHeader = hydra.getPresetHeader(*presetListIter);
      // copy and align pbag zone pointer
      dataPHDR[headerCount] = presetHeader;
      dataPHDR[headerCount].wPresetBagNdx = *bagIdxIter;
    }
    // PHDR terminal sample
    sfPresetHeader_t &destTerminalPHDREntry = dataPHDR[lenPHDR-1];
    // phdr->pbag
    destTerminalPHDREntry.wPresetBagNdx = pbagList.size();
    memcpy(&destTerminalPHDREntry.achPresetName, "EOP", 3);
  }
  // PHDR chunk
  IFFChunk PHDR(iff_ckid("phdr"),
    (char*)dataPHDR,
    lenPHDR * sizeof(sfPresetHeader_t));

  // PBAG data
  sfPresetBag_t *dataPBAG = 0;
  uint32_t lenPBAG = (pbagList.size()+1/*terminal*/);
  if(data) {
    dataPBAG = new sfPresetBag_t[lenPBAG];
    std::list<sfPresetBag_t>::iterator pbagIter;
    uint32_t pbagCount;
    for(pbagIter=pbagList.begin(), pbagCount=0;
        pbagIter!=pbagList.end();
        pbagIter++, pbagCount++) {
      dataPBAG[pbagCount].wGenNdx = pbagIter->wGenNdx;
      dataPBAG[pbagCount].wModNdx = pbagIter->wModNdx;
    }
    // PBAG terminal: point to terminals
    dataPBAG[lenPBAG-1].wGenNdx = pgenIdxList.size();
    dataPBAG[lenPBAG-1].wModNdx = pmodIdxList.size();
  }
  // PBAG chunk
  IFFChunk PBAG(iff_ckid("pbag"),
    (char*)dataPBAG,
    lenPBAG * sizeof(sfPresetBag_t));

  // PMOD data
  sfModList_t *dataPMOD = 0;
  uint32_t lenPMOD = (pmodIdxList.size()+1/*terminal*/);
  if(data) {
    dataPMOD = new sfModList_t[lenPMOD];
    std::list<uint16_t>::iterator pmodIter;
    uint32_t pmodCount;
    for(pmodIter=pmodIdxList.begin(), pmodCount=0;
        pmodIter!=pmodIdxList.end();
        pmodIter++, pmodCount++) {
      const sfModList_t& sourcePMOD = hydra.getPresetModulator(*pmodIter);
      // copy
      dataPMOD[pmodCount] = sourcePMOD;
    }
    // PMOD terminal: all fields 0 -> nothing to do
  }
  // PMOD chunk
  IFFChunk PMOD(iff_ckid("pmod"),
    (char*)dataPMOD,
    lenPMOD * sizeof(sfModList_t));

  // PGEN data
  sfGenList_t *dataPGEN = 0;
  uint32_t lenPGEN = (pgenIdxList.size()+1/*terminal*/);
  if(data) {
    dataPGEN = new sfGenList_t[lenPGEN];
    std::list<uint16_t>::iterator pgenIter;
    uint32_t pgenCount;
    for(pgenIter=pgenIdxList.begin(), pgenCount=0;
        pgenIter!=pgenIdxList.end();
        pgenIter++, pgenCount++) {
      const sfGenList_t& sourcePGEN = hydra.getPresetGenerator(*pgenIter);
      // copy
      dataPGEN[pgenCount] = sourcePGEN;
      // instrument?
      if(dataPGEN[pgenCount].sfGenOper == (SFGenerator)INSTRUMENT) {
        // align instrument index
        assert(instIdxMapper.find(sourcePGEN.genAmount) != instIdxMapper.end());
        dataPGEN[pgenCount].genAmount = instIdxMapper[sourcePGEN.genAmount];
      }
    }
    // PGEN terminal: all fields 0 -> nothing to do
  }
  // PGEN chunk
  IFFChunk PGEN(iff_ckid("pgen"),
    (char*)dataPGEN,
    lenPGEN * sizeof(sfGenList_t));

  // INST data
  sfInst_t *dataINST = 0;
  uint32_t lenINST = (instIdxList.size()+1/*terminal*/);
  if(data) {
    dataINST = new sfInst_t[lenINST];
    assert(instIdxList.size() == ibagIdxForINSTList.size());
    std::list<uint16_t>::iterator instIter;
    std::list<uint16_t>::iterator bagIdxIter;
    uint32_t instCount;
    for(instIter=instIdxList.begin(), bagIdxIter=ibagIdxForINSTList.begin(), instCount=0;
        instIter!=instIdxList.end();
        instIter++, bagIdxIter++, instCount++) {
      const sfInst_t& sourceINST = hydra.getInstrument(*instIter);
      // copy and align ibag zone pointer
      dataINST[instCount] = sourceINST;
      dataINST[instCount].wInstBagNdx = *bagIdxIter;
    }
    // INST terminal sample
    sfInst_t &destTerminalINSTEntry = dataINST[lenINST-1];
    // inst->ibag
    destTerminalINSTEntry.wInstBagNdx = ibagList.size();
    memcpy(&destTerminalINSTEntry.achInstName, "EOI", 3);
  }
  // INST chunk
  IFFChunk INST(iff_ckid("inst"),
    (char*)dataINST,
    lenINST * sizeof(sfInst_t));

  // IBAG data
  sfInstBag_t *dataIBAG = 0;
  uint32_t lenIBAG = (ibagList.size()+1/*terminal*/);
  if(data) {
    dataIBAG = new sfInstBag_t[lenIBAG];
    std::list<sfInstBag_t>::iterator ibagIter;
    uint32_t ibagCount;
    for(ibagIter=ibagList.begin(), ibagCount=0;
        ibagIter!=ibagList.end();
        ibagIter++, ibagCount++) {
      dataIBAG[ibagCount].wInstGenNdx = ibagIter->wInstGenNdx;
      dataIBAG[ibagCount].wInstModNdx = ibagIter->wInstModNdx;
    }
    // IBAG terminal: point to terminals
    dataIBAG[lenIBAG-1].wInstGenNdx = igenIdxList.size();
    dataIBAG[lenIBAG-1].wInstModNdx = imodIdxList.size();
  }
  // IBAG chunk
  IFFChunk IBAG(iff_ckid("ibag"),
    (char*)dataIBAG,
    lenIBAG * sizeof(sfInstBag_t));

  // IMOD data
  sfModList_t *dataIMOD = 0;
  uint32_t lenIMOD = (imodIdxList.size()+1/*terminal*/);
  if(data) {
    dataIMOD = new sfModList_t[lenIMOD];
    std::list<uint16_t>::iterator imodIter;
    uint32_t imodCount;
    for(imodIter=imodIdxList.begin(), imodCount=0;
        imodIter!=imodIdxList.end();
        imodIter++, imodCount++) {
      const sfModList_t& sourceIMOD = hydra.getInstrumentModulator(*imodIter);
      // copy
      dataIMOD[imodCount] = sourceIMOD;
    }
    // IMOD terminal: all fields 0 -> nothing to do
  }
  // IMOD chunk
  IFFChunk IMOD(iff_ckid("imod"),
    (char*)dataIMOD,
    lenIMOD * sizeof(sfModList_t));

  // IGEN data
  sfGenList_t *dataIGEN = 0;
  uint32_t lenIGEN = (igenIdxList.size()+1/*terminal*/);
  if(data) {
    dataIGEN = new sfGenList_t[lenIGEN];
    std::list<uint16_t>::iterator igenIter;
    uint32_t igenCount;
    for(igenIter=igenIdxList.begin(), igenCount=0;
        igenIter!=igenIdxList.end();
        igenIter++, igenCount++) {
      const sfGenList_t& sourceIGEN = hydra.getInstrumentGenerator(*igenIter);
      // copy
      dataIGEN[igenCount] = sourceIGEN;
      // sample?
      if(dataIGEN[igenCount].sfGenOper == (SFGenerator)SAMPLEID) {
        // align sample index
        assert(shdrIdxMapper.find(sourceIGEN.genAmount) != shdrIdxMapper.end());
        dataIGEN[igenCount].genAmount = shdrIdxMapper[sourceIGEN.genAmount];
      }
    }
    // IGEN terminal: all fields 0 -> nothing to do
  }
  // IGEN chunk
  IFFChunk IGEN(iff_ckid("igen"),
    (char*)dataIGEN,
    lenIGEN * sizeof(sfGenList_t));

  // SHDR chunk (data was created with sdta data above)
  IFFChunk SHDR(iff_ckid("shdr"),
    (char*)dataSHDR,
    lenSHDR * sizeof(sfSample_t));

  // We have our hydra complete -> create pdta chunk and write data
  IFFChunkList pdtaSubChunks;
  pdtaSubChunks.push_back(PHDR);
  pdtaSubChunks.push_back(PBAG);
  pdtaSubChunks.push_back(PMOD);
  pdtaSubChunks.push_back(PGEN);
  pdtaSubChunks.push_back(INST);
  pdtaSubChunks.push_back(IBAG);
  pdtaSubChunks.push_back(IMOD);
  pdtaSubChunks.push_back(IGEN);
  pdtaSubChunks.push_back(SHDR);
  // make sdta chunk and write data
  IFFChunk pdta(iff_ckid("pdta"), pdtaSubChunks);
  pdta.writeData(len, data, IFF_FMT_RIFF);

  // output what will be written
  if(data) {
    std::cout << "PHDR:\n";
    for(uint16_t phdrEntry = 0; phdrEntry<lenPHDR; phdrEntry++) {
      std::string presetName;
      sf2NameToStr(presetName, ((sfPresetHeader_t*)PHDR.dataPtr())[phdrEntry].achPresetName);
      std::cout << "Preset: " << presetName << "\n";
    }

    std::cout << "INST:\n";
    for(uint16_t instEntry = 0; instEntry<lenINST; instEntry++) {
      std::string instName;
      sf2NameToStr(instName, ((sfInst_t*)INST.dataPtr())[instEntry].achInstName);
      std::cout << "Instrument: " << instName << "\n";
    }

    std::cout << "SHDR:\n";
    for(uint16_t shdrEntry = 0; shdrEntry<lenSHDR; shdrEntry++) {
      std::string sampleName;
      sf2NameToStr(sampleName, ((sfSample_t*)SHDR.dataPtr())[shdrEntry].achSampleName);
      std::cout << "Sample: " << sampleName << "\n";
    }
  }




  // Now that we know total len - write it right after initial 'RIFF'
  if(data) {
    // RIFF and len itself are not within lenght -> '-8'
    uint32_t currLen = u32(len-8, IFF_FMT_RIFF);
    memcpy(data+4, &currLen, 4);
  }

  // Cleanup
  for(IFFChunkIterator sdtaSubChunksIter = sdta.ck_begin();
      sdtaSubChunksIter != sdta.ck_end();
      sdtaSubChunksIter++) {
    delete (*sdtaSubChunksIter).dataPtr();
  }
  for(IFFChunkIterator pdtaSubChunksIter = pdta.ck_begin();
      pdtaSubChunksIter != pdta.ck_end();
      pdtaSubChunksIter++) {
    delete (*pdtaSubChunksIter).dataPtr();
  }

  return len;
}
