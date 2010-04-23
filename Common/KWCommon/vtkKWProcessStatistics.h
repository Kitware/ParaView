/*=========================================================================

  Module:    vtkKWProcessStatistics.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWProcessStatistics - get statistics such as cpu and memory usage
// .SECTION Description

#ifndef __vtkKWProcessStatistics_h
#define __vtkKWProcessStatistics_h

#include "vtkObject.h"

class VTK_EXPORT vtkKWProcessStatistics : public vtkObject
{
public:
  // Description:
  // Construct the ProcessStatistics with eight points.
  static vtkKWProcessStatistics *New();
  vtkTypeMacro(vtkKWProcessStatistics,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  int   GetProcessSizeInBytes();
  float GetProcessCPUTimeInMilliseconds();

  // Description:
  // Evaluate the memory information.
  int QueryMemory();

  // Description:
  // Retrieve memory information in kilobytes.
  long GetTotalVirtualMemory();
  long GetAvailableVirtualMemory();
  long GetTotalPhysicalMemory();
  long GetAvailablePhysicalMemory();  

protected:
  vtkKWProcessStatistics();
  ~vtkKWProcessStatistics() {};

  
private:
  vtkKWProcessStatistics(const vtkKWProcessStatistics&);  // Not implemented.
  void operator=(const vtkKWProcessStatistics&);  // Not implemented.
  
  long TotalVirtualMemory;
  long AvailableVirtualMemory;
  long TotalPhysicalMemory;
  long AvailablePhysicalMemory;
  
};

#endif





