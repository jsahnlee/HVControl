#ifndef HVChannel_hh
#define HVChannel_hh

#include <ncurses.h>
#include <string>

class HVChannel {
public:
  HVChannel(const char * name, int slot, int channel, const char * group, float vset, float ohm) {
    fName = name;
    fSlot = slot;
    fChannel = channel;
    fGroup = group;
    fVSet = vset;
    fOhm = ohm;
  }
  ~HVChannel(){}

  void SetCurrentValue(float v, float i) {
    fVCur = v;
    fICur = i;
  }

  const char * GetName() const {
    return fName.c_str();
  }

  int GetSlot() const {
    return fSlot;
  }

  int GetChannel() const {
    return fChannel;
  }

  const char * GetGroup() const {
    return fGroup.data();
  }

  float GetVSet() const  {
    return fVSet;
  }
  float GetMaxI() const {
    return fVSet/fOhm*1.05;
  }

  void Print() const {
    printw("%6s %2d %2d %7.1f %7.1f %7.1f %7.1f %6s\n",
           fName.c_str(), fSlot, fChannel, fVSet, fVSet/fOhm*1.05, fVCur, fICur, fGroup.c_str());
  }

  void print() const {
    printf("%6s %2d %2d %7.1f %7.1f %7.1f %7.1f %6s\n",
           fName.c_str(), fSlot, fChannel, fVSet, fVSet/fOhm*1.05, fVCur, fICur, fGroup.c_str());
  }

private:
  std::string fName;
  std::string fGroup;
  int fSlot;
  int fChannel;
  float fVSet;
  float fOhm;
  float fVCur;
  float fICur;

};

#endif
