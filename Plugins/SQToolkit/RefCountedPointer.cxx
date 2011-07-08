/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#include "RefCountedPointer.h"

#ifdef RefCountedPointerDEBUG
//-----------------------------------------------------------------------------
virtual void RefCountedPointer::Delete()
{
  if ((--this->N)==0)
    {
    cerr << "Deleted : "; this->Print(cerr); cerr << endl;
    delete this;
    }
  else
    {
    this->PrintRefCount(cerr); cerr << " "; this->Print(cerr); cerr << endl;
    }
}
#endif

//*****************************************************************************
ostream &operator<<(ostream &os, RefCountedPointer &rcp)
{
  rcp.Print(os);
  return os;
}

