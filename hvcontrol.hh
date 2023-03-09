#ifndef hvcontrol_hh
#define hvcontrol_hh

#include "HVChannel.hh"
#include <vector>

std::vector<HVChannel> fChannels;

int readHVTable(const char * filename);
int initialize(int handle);
int monitoring(int handle, const char * group = "all");
int powerOn(int handle, const char * group);
int powerOff(int handle, const char * group = "all");
int print(int handle);

#endif
