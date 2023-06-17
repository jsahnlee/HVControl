#include <cstdarg>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "TDatime.h"
#include "TMessage.h"
#include "TMonitor.h"
#include "TRandom3.h"
#include "TServerSocket.h"
#include "TString.h"

#include "hvserver.hh"

using namespace std;

int main(int argc, char ** argv)
{
  fPowerState = 0;

  TString ipaddr = "192.168.2.10";
  TString user = "admin";
  TString pass = "admin";
  TString hvtable;

  const char * optstring = "a:u:p:t:";
  int option;
  while ((option = getopt(argc, argv, optstring)) != -1) {
    switch (option) {
      case 'a': ipaddr = optarg; break;
      case 'u': user = optarg; break;
      case 'p': pass = optarg; break;
      case 't': hvtable = optarg; break;
      default: break;
    }
  }

  // read hv table
  if (!readhvtable(hvtable.Data())) return 0;
  // connect to caen system
  // if (!hvconnect(ipaddr.Data(), user.Data(), pass.Data())) return 0;
  // load hv setting
  // hvloadsetting();

  // threads
  std::thread th1(&tf_monitoring_toy);
  std::thread th2(&tf_msgserver);

  th1.join();
  th2.join();

  // hvdisconnect();

  return 0;
}

bool readhvtable(const char * tname)
{
  ifstream input;
  input.open(tname);
  if (!input.is_open()) {
    Log(ERROR, "readhvtable", "failed to read hv table (%s)", tname);
    return false;
  }

  fHVTable = tname;

  fChannels.clear();

  string line;
  while (input.good()) {
    getline(input, line);

    if (line.empty()) continue;

    istringstream iss(line);

    string name, group;
    int slot, channel, pmt;
    float vset, ohm;
    iss >> name >> slot >> channel >> vset >> ohm >> pmt >> group;

    if (iss.fail() || (name.data())[0] == '#') continue;

    HVChannel * ch = new HVChannel(name.data(), slot, channel, group.data(), vset, ohm, pmt);
    fChannels.push_back(ch);
  }

  return true;
}

bool hvconnect(const char * addr, const char * user, const char * pass,
               CAENHV_SYSTEM_TYPE_t stype)
{
  CAENHVRESULT ret;
  ret = CAENHV_InitSystem(stype, 0, (void *)addr, user, pass, &fSystemHandle);
  if (ret != 0) {
    Log(ERROR, "hvconnect", "failed to connect to CAEN system (%s)",
        CAENHV_GetError(fSystemHandle));
    return false;
  }
  return true;
}

void hvdisconnect() { CAENHV_DeinitSystem(fSystemHandle); }

bool hvloadsetting()
{
  std::unique_lock<std::mutex> lock(fCAENMutex);

  CAENHVRESULT ret;
  for (HVChannel * ch : fChannels) {
    const char * name = ch->GetName();
    ushort slot = ch->GetSlot();
    ushort channel = ch->GetChannel();
    float vset = ch->GetVSet();
    float iset = ch->GetIMax();
    ret = CAENHV_SetChName(fSystemHandle, slot, 1, &channel, name);
    if (ret != 0) {
      Log(ERROR, "hvloadsetting", "failed to set channel name (%s)",
          CAENHV_GetError(fSystemHandle));
      return false;
    }
    ret = CAENHV_SetChParam(fSystemHandle, slot, "V0Set", 1, &channel, &vset);
    if (ret != 0) {
      Log(ERROR, "hvloadsetting", "failed to set channel V0 (%s)",
          CAENHV_GetError(fSystemHandle));
      return false;
    }
    ret = CAENHV_SetChParam(fSystemHandle, slot, "I0Set", 1, &channel, &iset);
    if (ret != 0) {
      Log(ERROR, "hvloadsetting", "failed to set channel I0 (%s)",
          CAENHV_GetError(fSystemHandle));
      return false;
    }
  }

  return true;
}

/*
bool hvpower(ulong power, int n, int * chid)
{
  std::unique_lock<std::mutex> lock(fCAENMutex);

  CAENHVRESULT ret;
  for (HVChannel * ch : fChannels) {
    bool pass = false;
    int id = ch->GetID();

    if (n > 0) {
      pass = true;
      for (int i = 0; i < n; i++) {
        if (id == chid[i]) pass = false;
      }
    }
    if (pass) continue;

    ushort slot = ch->GetSlot();
    ushort channel = ch->GetChannel();
    ret = CAENHV_SetChParam(fSystemHandle, slot, "Pw", 1, &channel, &power);
    if (ret != 0) {
      Log(ERROR, "hvloadsetting", "failed to power channel (%s)",
          CAENHV_GetError(fSystemHandle));
      return false;
    }
  }

  return true;
}
*/

bool hvpower(ulong power, int n, int * chid)
{
  std::unique_lock<std::mutex> lock(fCAENMutex);

  CAENHVRESULT ret;
  for (HVChannel * ch : fChannels) {
    bool pass = false;
    int id = ch->GetID();

    if (n > 0) {
      pass = true;
      for (int i = 0; i < n; i++) {
        if (id == chid[i]) pass = false;
      }
    }
    if (pass) continue;
    ch->SetPower(power);
  }
  return true;
}

void tf_monitoring()
{
  Log(INFO, "monitoring", "hv monitoring started");

  float vcur, icur;
  CAENHVRESULT ret;

  while (true) {
    std::unique_lock<std::mutex> lock(fCAENMutex);
    for (HVChannel * ch : fChannels) {
      ushort slot = ch->GetSlot();
      ushort channel = ch->GetChannel();
      vcur = 0;
      ret = CAENHV_GetChParam(fSystemHandle, slot, "VMon", 1, &channel, &vcur);
      if (ret != 0) {
        Log(ERROR, "tf_monitoring", "failed in VMon (%s)",
            CAENHV_GetError(fSystemHandle));
      }
      icur = 0;

      ret = CAENHV_GetChParam(fSystemHandle, slot, "IMon", 1, &channel, &icur);
      if (ret != 0) {
        Log(ERROR, "hvloadsetting", "failed in IMon (%s)",
            CAENHV_GetError(fSystemHandle));
      }
      ch->SetCurrentValue(vcur, icur);
    }
    lock.unlock();
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

void tf_monitoring_toy()
{
  Log(INFO, "monitoring", "hv monitoring started");

  float vcur, icur;
  while (true) {
    std::unique_lock<std::mutex> lock(fCAENMutex);
    for (HVChannel * ch : fChannels) {
      if (ch->GetPower() == 0) { 
        ch->RampDown(); 
      }
      else {
        ch->RampUp();
      }
    }
    lock.unlock();
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

void tf_msgserver()
{
  int port = kPORT;
  char buffer[kMESSLEN];

  Log(INFO, "msgserver", "server started");

  auto * server = new TServerSocket(port, true);
  auto * monitor = new TMonitor();
  monitor->Add(server);

  while (true) {
    TSocket * socket = monitor->Select(1000);
    if (socket == (TSocket *)-1) { continue; }

    TInetAddress addr = socket->GetInetAddress();

    if (socket->IsA() == TServerSocket::Class()) {
      TSocket * s = ((TServerSocket *)socket)->Accept();
      monitor->Add(s);

      addr = s->GetInetAddress();
      Log(INFO, "msgserver", "new client connected [ip=%s, port=%d]",
          addr.GetHostAddress(), addr.GetPort());
      continue;
    }

    int stat = socket->RecvRaw(buffer, kUNITSIZE);
    if (stat <= 0) {
      if (stat == 0 || stat == -5) {
        Log(INFO, "msgserver", "client disconnected [ip=%s, port=%d]",
            addr.GetHostAddress(), addr.GetPort());
      }
      else {
        Log(WARNING, "msgserver", "received error (%d) [ip=%s, port=%d]", stat,
            addr.GetHostAddress(), addr.GetPort());
      }
      socket->Close();
      monitor->Remove(socket);
      delete socket;
    }
    else {
      int cmd;
      memcpy(&cmd, buffer, kUNITSIZE);

      if (cmd == kHVOFF) {
        int nch = 0;
        int * chnum = nullptr;
        socket->RecvRaw(buffer, kUNITSIZE);
        memcpy(&nch, buffer, kUNITSIZE);
        if (nch > 0) {
          chnum = new int[nch];
          socket->RecvRaw(buffer, nch * kUNITSIZE);
          memcpy(chnum, buffer, nch * kUNITSIZE);
        }
        hvpower(0, nch, chnum);
        if (chnum) delete chnum;
      }
      else if (cmd == kHVON) {
        int nch = 0;
        int * chnum = nullptr;
        socket->RecvRaw(buffer, kUNITSIZE);
        memcpy(&nch, buffer, kUNITSIZE);
        if (nch > 0) {
          chnum = new int[nch];
          socket->RecvRaw(buffer, nch * kUNITSIZE);
          memcpy(chnum, buffer, nch * kUNITSIZE);
        }
        hvpower(1, nch, chnum);
        if (chnum) delete chnum;
      }
      else if (cmd == kHVMON) {
        memset(buffer, 0, kMESSLEN);

        float vcur[kNCH];
        float icur[kNCH];
        memset(vcur, 0, kNCH * sizeof(float));
        memset(icur, 0, kNCH * sizeof(float));

        std::unique_lock<std::mutex> lock(fCAENMutex);
        for (HVChannel * ch : fChannels) {
          int id = ch->GetID();
          vcur[id - 1] = ch->GetV();
          icur[id - 1] = ch->GetI();
        }
        lock.unlock();

        memcpy(buffer, vcur, kNCH * kUNITSIZE);
        memcpy(buffer + kNCH * kUNITSIZE, icur, kNCH * kUNITSIZE);
        socket->SendRaw(buffer, 2*kNCH * kUNITSIZE);
      }
      else if (cmd == kTBLNEW) {
        char fname[256];
        socket->Recv(fname, 256);
        if (readhvtable(fname)) { hvloadsetting(); }
      }
      else if (cmd == kTBLCUR) {
        socket->SendRaw(fHVTable.Data(), 256);
      }
      else {
        Log(WARNING, "msgserver", "unknown command (%d) received", cmd);
      }
    }
  }

  monitor->DeActivateAll();
  TList * sockets = monitor->GetListOfDeActives();
  for (int i = 0; i < sockets->GetSize(); i++) {
    ((TSocket *)(sockets->At(i)))->Close();
    delete ((TSocket *)(sockets->At(i)));
  }

  monitor->RemoveAll();
  delete monitor;
}

void Log(LEVEL_t level, const char * where, const char * log, ...)
{
  va_list argList;
  char buffer[1024];

  va_start(argList, log);
  vsnprintf(buffer, 1024, log, argList);
  va_end(argList);

  time_t ctime;
  time(&ctime);

  TString timestr = TDatime(ctime).AsSQLString();
  TString levelstr;
  switch (level) {
    case INFO: levelstr = " [INFO] "; break;
    case WARNING: levelstr = " [WARNING] "; break;
    case ERROR: levelstr = " [ERROR] "; break;
    default: break;
  }
  TString mess = Form("%s: %s", where, buffer);

  TString log = timestr + levelstr + mess;
  cout << log << endl;
}
