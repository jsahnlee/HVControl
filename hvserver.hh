#ifndef hvserver_hh
#define hvserver_hh

#include <vector>
#include <mutex>

#include "CAENHVWrapper.h"
#include "HVChannel.hh"

// constants
const int kPORT = 7820;
const int kUNITSIZE = 4;
const int kMESSLEN = 100*kUNITSIZE;
const int kNCH = 48;

// command
const int kHVOFF = 0;
const int kHVON = 1;
const int kHVMON = 2;
const int kTBLCUR = 3;
const int kTBLNEW = 4;

typedef enum { INFO, WARNING, ERROR } LEVEL_t;

// global variable
int fSystemHandle;
std::vector<HVChannel*> fChannels;
std::mutex fCAENMutex;
std::mutex fLogMutex;
TString fHVTable;
int fPowerState;

// functions
bool readhvtable(const char * tname);
bool hvconnect(const char * addr, const char * user, const char * pass,
               CAENHV_SYSTEM_TYPE_t stype = SY4527);
void hvdisconnect();
bool hvloadsetting();
bool hvpower(ulong power, int n, int * chid);



void tf_monitoring();
void tf_monitoring_toy();
void tf_msgserver();

void Log(LEVEL_t level, const char * where, const char * log, ...);

#endif