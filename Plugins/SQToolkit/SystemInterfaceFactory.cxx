/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/

#include "SystemInterfaceFactory.h"

#include "SystemType.h"
#include "LinuxSystemInterface.h"
#include "WindowsSystemInterface.h"
#include "OSXSystemInterface.h"

#include "SQMacros.h"
#include <iostream>
using std::cerr;
using std::endl;

//-----------------------------------------------------------------------------
SystemInterface *SystemInterfaceFactory::NewSystemInterface()
{
  // This typdef is declared in SystemType.h according to the 
  // current system type
  return new CurrentSystemInterface;
}
