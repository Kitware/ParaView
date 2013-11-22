/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#include "RefCountedPointer.h"

#ifdef RefCountedPointerDEBUG
//-----------------------------------------------------------------------------
virtual void RefCountedPointer::Delete()
{
  if ((--this->N)==0)
    {
    std::cerr << "Deleted : "; this->Print(std::cerr); std::cerr << std::endl;
    delete this;
    }
  else
    {
    this->PrintRefCount(std::cerr); std::cerr << " "; this->Print(std::cerr); std::cerr << std::endl;
    }
}
#endif

//*****************************************************************************
std::ostream &operator<<(std::ostream &os, RefCountedPointer &rcp)
{
  rcp.Print(os);
  return os;
}
