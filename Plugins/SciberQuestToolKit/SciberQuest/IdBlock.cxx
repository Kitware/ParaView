/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/

#include "IdBlock.h"

std::ostream &operator<<(std::ostream &os, const IdBlock &b)
{
  os << "(" << b.m_data[0] << ", " << b.m_data[1] << ")";
  return os;
}
