#include <fstream>
#include <iostream>
#include <ncurses.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <term.h>
#include <thread>
#include <unistd.h>

#include "CAENHVWrapper.h"
#include "HVChannel.hh"
#include "hvcontrol.hh"

using namespace std;

bool STOP = false;

void tf_stop() {
  char a;
  while (true) {
    cin >> a;
    if (a == 'q') {
      STOP = true;
      break;
    }
  }
}

void PrintHelp() {
  cout << "Usage: hvcontrol [options]" << endl;
  cout << endl;
  cout << "Options:" << endl;
  cout << "  -t     HV table name" << endl;
  cout << "  -c     command (on/off/mon/set)" << endl;
  cout << "  -g     group name" << endl;
  cout << endl;
  cout << "example: hvcontrol -t table -c on -g group" << endl;
  cout << endl;
}

int main(int argc, char **argv) 
{
  if (argc < 2) {
    clear();
    PrintHelp();
    return -1;
  }

  string tableName;
  string command;
  string groupName = "all";

  int opt;
  while ((opt = getopt(argc, argv, "t:c:g:")) != -1) {
    switch (opt) {
    case 't':
      tableName = optarg;
      break;
    case 'c':
      command = optarg;
      break;
    case 'g':
      groupName = optarg;
      break;
    default:
      break;
    }
  }

  readHVTable(tableName.data());

  int sysHndl = -1;

  int sysType = 3; // SY4527
  int linkType = 0;
  const char *ipaddr = "192.168.1.27";
  const char *userName = "admin";
  const char *passwd = "admin";

  CAENHVRESULT ret;
  ret = CAENHV_InitSystem((CAENHV_SYSTEM_TYPE_t)sysType, linkType,
                          (void *)ipaddr, userName, passwd, &sysHndl);
  if (ret != 0) {
    printf("CAENHV_InitSystem: %s (num. %d)\n", CAENHV_GetError(sysHndl), ret);
    return -1;
  }

  if (command == "mon") {
    if (monitoring(sysHndl, groupName.data()) != 0) {
      return -1;
    }
  } else if (command == "off") {
    if (powerOff(sysHndl, groupName.data()) != 0) {
      return -1;
    }
  } else if (command == "on") {
    if (powerOn(sysHndl, groupName.data()) != 0) {
      return -1;
    }
  } else if (command == "set") {
    if (initialize(sysHndl) != 0) {
      return -1;
    }
  } else if (command == "print") {
    if (print(sysHndl) != 0) {
      return -1;
    }
  }

  ret = CAENHV_DeinitSystem(sysHndl);
  if (ret != 0) {
    printf("CAENHV_DeInitSystem: %s (num. %d)\n", CAENHV_GetError(sysHndl), ret);
  }

  return 0;
}

int readHVTable(const char *filename) 
{
  ifstream input;
  input.open(filename);
  if (!input.is_open()) {
    return -1;
  }

  string line;
  while (input.good()) {
    getline(input, line);

    if (line.empty())
      continue;

    istringstream iss(line);

    string name, group;
    int slot, channel, pmt;
    float vset, ohm;
    iss >> name >> slot >> channel >> vset >> ohm >> pmt >> group;

    if (iss.fail() || (name.data())[0] == '#')
      continue;

    HVChannel ch(name.data(), slot, channel, group.data(), vset, ohm);
    fChannels.push_back(ch);
  }

  return 0;
}

int initialize(int handle) 
{
  CAENHVRESULT ret;
  for (HVChannel ch : fChannels) {
    const char *name = ch.GetName();
    ushort slot = ch.GetSlot();
    ushort channel = ch.GetChannel();
    float vset = ch.GetVSet();
    float iset = ch.GetMaxI();
    ret = CAENHV_SetChName(handle, slot, 1, &channel, name);
    if (ret != 0) {
      printf("CAENHV_SetChName: %s (num. %d)\n", CAENHV_GetError(handle), ret);
      return -1;
    }
    ret = CAENHV_SetChParam(handle, slot, "V0Set", 1, &channel, &vset);
    if (ret != 0) {
      printf("CAENHV_SetChParam: %s (num. %d)\n", CAENHV_GetError(handle), ret);
      return -1;
    }
    ret = CAENHV_SetChParam(handle, slot, "I0Set", 1, &channel, &iset);
    if (ret != 0) {
      printf("CAENHV_SetChParam: %s (num. %d)\n", CAENHV_GetError(handle), ret);
      return -1;
    }
  }

  return 0;
}

int monitoring(int handle, const char *group) 
{
  std::thread t(tf_stop);

  string gname = group;
  float vcur, icur;

  CAENHVRESULT ret;

  initscr();
  while (true) {
    clear();

    for (HVChannel ch : fChannels) {
      string gg = ch.GetGroup();
      if (gg == gname || gname == "all") {
        ushort slot = ch.GetSlot();
        ushort channel = ch.GetChannel();
        ret = CAENHV_GetChParam(handle, slot, "VMon", 1, &channel, &vcur);
        if (ret != 0) {
          printf("CAENHV_GetChParam: %s (num. %d)\n", CAENHV_GetError(handle),
                 ret);
          return -1;
        }
        ret = CAENHV_GetChParam(handle, slot, "IMon", 1, &channel, &icur);
        if (ret != 0) {
          printf("CAENHV_GetChParam: %s (num. %d)\n", CAENHV_GetError(handle),
                 ret);
          return -1;
        }
        ch.SetCurrentValue(vcur, icur);
        ch.Print();
      }
    }

    refresh();

    if (STOP)
      break;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  endwin();
  t.join();

  return 0;
}

int print(int handle) 
{
  float vcur, icur;
  CAENHVRESULT ret;

  for (HVChannel ch : fChannels) {
    ushort slot = ch.GetSlot();
    ushort channel = ch.GetChannel();
    ret = CAENHV_GetChParam(handle, slot, "VMon", 1, &channel, &vcur);
    if (ret != 0) {
      printf("CAENHV_GetChParam: %s (num. %d)\n", CAENHV_GetError(handle), ret);
      return -1;
    }
    ret = CAENHV_GetChParam(handle, slot, "IMon", 1, &channel, &icur);
    if (ret != 0) {
      printf("CAENHV_GetChParam: %s (num. %d)\n", CAENHV_GetError(handle), ret);
      return -1;
    }
    ch.SetCurrentValue(vcur, icur);
    ch.print();
  }

  return 0;
}

int powerOn(int handle, const char *group) 
{
  string gname = group;

  CAENHVRESULT ret;
  for (HVChannel ch : fChannels) {
    string gg = ch.GetGroup();

    if (gg == gname || gname == "all") {
      ushort slot = ch.GetSlot();
      ushort channel = ch.GetChannel();
      ulong val = 1;
      ret = CAENHV_SetChParam(handle, slot, "Pw", 1, &channel, &val);
      if (ret != 0) {
        printf("CAENHV_SetChParam: %s (num. %d)\n", CAENHV_GetError(handle),
               ret);
        return -1;
      }
    }
  }

  return 0;
}

int powerOff(int handle, const char *group) 
{
  string gname = group;

  CAENHVRESULT ret;
  for (HVChannel ch : fChannels) {
    string gg = ch.GetGroup();

    if (gg == gname || gname == "all") {
      ushort slot = ch.GetSlot();
      ushort channel = ch.GetChannel();
      ulong val = 0;
      ret = CAENHV_SetChParam(handle, slot, "Pw", 1, &channel, &val);
      if (ret != 0) {
        printf("CAENHV_SetChParam: %s (num. %d)\n", CAENHV_GetError(handle),
               ret);
        return -1;
      }
    }
  }

  return 0;
}

