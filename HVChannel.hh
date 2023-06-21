#ifndef HVChannel_hh
#define HVChannel_hh

#include <string>
#include <ncurses.h>

class HVChannel {
public:
  HVChannel(const char * name, int slot, int channel, const char * group,
            float vset, float ohm, int id)
  {
    fName = name;
    fGroup = group;
    fID = id;
    fSlot = slot;
    fChannel = channel;
    fVSet = vset;
    fOhm = ohm;
    fVCur = 0.;
    fICur = 0.;
    fPower = 0;
  }
  ~HVChannel() {}

  void SetCurrentValue(float v, float i)
  {
    fVCur = v;
    fICur = i;
  }

  void SetPower(int p)
  {
    fPower = p;
  }
  int GetPower() const { return fPower; }

  void RampUp()
  {
    fVCur += fVSet/20.;
    if (fVCur > fVSet)
      fVCur = fVSet;
    fICur = fVCur/fOhm;
  }

  void RampDown()
  {
    fVCur -= fVSet/20;
    if (fVCur < 0)
      fVCur = 0;
    fICur = fVCur/fOhm;    
  }

  int GetID() const { return fID; }
  int GetSlot() const { return fSlot; }
  int GetChannel() const { return fChannel; }
  float GetVSet() const { return fVSet; }
  float GetIMax() const { return fVSet / fOhm * 1.05; }
  float GetV() const { return fVCur; }
  float GetI() const { return fICur; }
  float GetOhm() const { return fOhm; }
  const char * GetName() const { return fName.c_str(); }
  const char * GetGroup() const { return fGroup.data(); }

  void Print() const
  {
    printw("%6s %2d %2d %7.1f %7.1f %7.1f %7.1f %6s\n", fName.c_str(), fSlot,
           fChannel, fVSet, fVSet / fOhm * 1.05, fVCur, fICur, fGroup.c_str());
  }

  void print() const
  {
    printf("%6s %2d %2d %7.1f %7.1f %7.1f %7.1f %6s\n", fName.c_str(), fSlot,
           fChannel, fVSet, fVSet / fOhm * 1.05, fVCur, fICur, fGroup.c_str());
  }

private:
  int fID;
  int fSlot;
  int fChannel;
  int fPower;
  float fVSet;
  float fOhm;
  float fVCur;
  float fICur;
  std::string fName;
  std::string fGroup;
};

#endif
