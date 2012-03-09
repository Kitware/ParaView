/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSystemInformation
// Information object used to collect miscellaneous system and memory
// information from all processes.
// .SECTION Description
// vtkPVProcessMemoryInformation is used to collect miscellaneous information
// from all processes involved. Implementation uses vtksys::SystemInformation to
// obtain the relevant information for each of the processes.

#ifndef __vtkPVSystemInformation_h
#define __vtkPVSystemInformation_h

#include "vtkProcessModule.h" // needed for vtkProcessModule::ProcessTypes
#include "vtkPVInformation.h"
#include "vtkStdString.h" // needed for vtkStdString
#include <vector> // needed for std::vector

class VTK_EXPORT vtkPVSystemInformation : public vtkPVInformation
{
public:
  static vtkPVSystemInformation* New();
  vtkTypeMacro(vtkPVSystemInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*);

  //BTX
  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);
  //ETX

  struct SystemInformationType
    {
    vtkProcessModule::ProcessTypes ProcessType;
    int ProcessId; // for parallel processes, this indicates the process id.
    int NumberOfProcesses;
    vtkStdString Hostname;
    vtkStdString OSName;
    vtkStdString OSRelease;
    vtkStdString OSVersion;
    vtkStdString OSPlatform;
    bool Is64Bits;
    unsigned int NumberOfPhyicalCPUs;
    unsigned int NumberOfLogicalCPUs; // per physical cpu
    size_t TotalPhysicalMemory;
    size_t AvailablePhysicalMemory;
    size_t TotalVirtualMemory;
    size_t AvailableVirtualMemory;
    };

  //BTX
  //  Provides access to the vector of informations.
  const std::vector<SystemInformationType>& GetSystemInformations()
    { return this->SystemInformations; }
  //ETX


//BTX
protected:
  vtkPVSystemInformation();
  ~vtkPVSystemInformation();

  std::vector<SystemInformationType> SystemInformations;
private:
  vtkPVSystemInformation(const vtkPVSystemInformation&); // Not implemented
  void operator=(const vtkPVSystemInformation&); // Not implemented
//ETX
};

#endif
