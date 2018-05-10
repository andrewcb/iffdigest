# uncomment the one that's your C++ compiler
#CXX	= c++
#CXX	= g++
# for debug
#CXXFLAGS += -g -Wall -O0
CXXFLAGS += -Wall

all:	iffdigest.o

iffdigest.o:	iffdigest.cc iffdigest.h
	$(CXX) $(CXXFLAGS) -c iffdigest.cc

ifflist:	ifflist.o iffdigest.o
	$(CXX) -o ifflist ifflist.o iffdigest.o

ifflist.o:	ifflist.cc iffdigest.h
	$(CXX) $(CXXFLAGS) -c ifflist.cc

wavtest:	wavtest.o iffdigest.o
	$(CXX) -o wavtest wavtest.o iffdigest.o

wavtest.o:	wavtest.cc iffdigest.h
	$(CXX) $(CXXFLAGS) -c wavtest.cc

clean:
	/bin/rm -f iffdigest.o ifflist.o ifflist wavtest.o wavtest
