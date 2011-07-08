/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "CartesianDataBlockIODescriptorIterator.h"

ostream &operator<<(ostream &os, const CartesianDataBlockIODescriptorIterator &it)
{
  os << it.GetFileView() << " -> " << it.GetMemView();
  return os;
}
