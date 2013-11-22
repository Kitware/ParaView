/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#ifndef RefCountedPointer_h
#define RefCountedPointer_h

#include <iostream> // for ostream

//=============================================================================
class RefCountedPointer
{
public:
  RefCountedPointer() : N(1) {}
  virtual ~RefCountedPointer(){}

  #if defined RefCountedPointerDEBUG
  virtual void Delete();
  #else
  virtual void Delete(){ if ((--this->N)==0){ delete this; } }
  #endif
  virtual void Register(){ ++this->N; }

  virtual int GetRefCount(){ return this->N; }
  virtual void SetRefCount(int rc){ this->N=rc; }

  virtual void Print(std::ostream &){}
  virtual void PrintRefCount(std::ostream &os){ os << "RefCount=" << this->N; }

private:
  int N;
};

//*****************************************************************************
std::ostream &operator<<(std::ostream &os, RefCountedPointer &rcp);

#define SetRefCountedPointer(NAME,TYPE)\
  virtual void Set##NAME(TYPE *v);\
  \
  virtual void SetNew##NAME(TYPE *v);\

#define SetRefCountedPointerImpl(CLASS,NAME,TYPE)\
void CLASS::Set##NAME(TYPE *v)\
{\
  if (v==this->NAME){ return; }\
  if (this->NAME){ this->NAME->Delete(); }\
  this->NAME=v;\
  if (this->NAME){ this->NAME->Register(); }\
}\
\
void CLASS::SetNew##NAME(TYPE *v)\
{\
  if (v==this->NAME){ return; }\
  if (this->NAME){ this->NAME->Delete(); }\
  this->NAME=v;\
}

#endif

// VTK-HeaderTest-Exclude: RefCountedPointer.h
