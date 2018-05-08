# uncomment the one that's your C++ compiler
#CXX	= c++
#CXX	= g++
# for debug
#CXXFLAGS += -g -Wall -O0
CXXFLAGS += -Wall

all:	sf2-split sf2-test

iffdigest.o:	iffdigest.cc iffdigest.h
	$(CXX) $(CXXFLAGS) -c iffdigest.cc

ifflist:	ifflist.o iffdigest.o
	$(CXX) -o ifflist ifflist.o iffdigest.o

ifflist.o:	ifflist.cc iffdigest.h
	$(CXX) $(CXXFLAGS) -c ifflist.cc

# Wave
wavtest:	wavtest.o iffdigest.o
	$(CXX) -o wavtest wavtest.o iffdigest.o

wavtest.o:	wavtest.cc iffdigest.h
	$(CXX) $(CXXFLAGS) -c wavtest.cc

# Soundfonts
sf2.o:	sf2.cc iffdigest.h sf2.h
	$(CXX) $(CXXFLAGS) -c sf2.cc

sf2filesplitter.o: sf2filesplitter.cc iffdigest.h sf2.h
	$(CXX) $(CXXFLAGS) -c sf2filesplitter.cc

sf2-test.o:	sf2-test.cc iffdigest.h sf2.h
	$(CXX) $(CXXFLAGS) -c sf2-test.cc

sf2-split.o: sf2-split.cc iffdigest.h sf2.h
	$(CXX) $(CXXFLAGS) -c sf2-split.cc


sf2-split: sf2.o sf2filesplitter.o sf2-split.o iffdigest.o
	$(CXX) $(LDFLAGS) -o sf2-split sf2.o sf2filesplitter.o sf2-split.o iffdigest.o

sf2-test: sf2.o sf2-test.o iffdigest.o
	$(CXX) $(LDFLAGS) -o sf2-test sf2.o sf2-test.o iffdigest.o


clean:
	/bin/rm -f iffdigest.o ifflist.o ifflist wavtest.o wavtest sf2.o sf2-split.o sf2-split sf2-test.o sf2-test
