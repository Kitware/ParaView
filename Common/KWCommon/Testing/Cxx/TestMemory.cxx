/*=========================================================================

  Program:   ParaView
  Module:    TestMemory.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWProcessStatistics.h"

int main()
{
  vtkKWProcessStatistics *pr = vtkKWProcessStatistics::New();

  float dev = 1024.0; // Let's display in MB.  
  cout 
    << "Total physical: " << pr->GetTotalPhysicalMemory() / dev << endl
    << "     Available: " << pr->GetAvailablePhysicalMemory() / dev << endl
    << " Total virtual: " << pr->GetTotalVirtualMemory() / dev << endl
    << "     Available: " << pr->GetAvailableVirtualMemory() / dev << endl;
  
  pr->Delete();
  return 0;
}
