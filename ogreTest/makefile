#change this to where your ogre SDK is installed, or point it to /usr/local if you used the default stuffs
OGRESDK=/home/ybalrid/femtogram/ogre/build/sdk
LDFLAGS= -L$(OGRESDK)/lib -lOgreMain -lOgreHlmsPbs -lOgreHlmsUnlit 
IFLAGS = -I$(OGRESDK)/include -I$(OGRESDK)/include/OGRE

all: test

test: sources/main.cpp
	g++ -g --std=c++14 sources/main.cpp $(IFLAGS) $(LDFLAGS) -o test
