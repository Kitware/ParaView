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
/**
 * @class   vtkPVSystemInformation
 * Information object used to collect miscellaneous system and memory
 * information from all processes.
 *
 * vtkPVProcessMemoryInformation is used to collect miscellaneous information
 * from all processes involved. Implementation uses vtksys::SystemInformation to
 * obtain the relevant information for each of the processes.
*/

#ifndef vtkPVSystemInformation_h
#define vtkPVSystemInformation_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkPVInformation.h"
#include "vtkProcessModule.h" // needed for vtkProcessModule::ProcessTypes
#include "vtkStdString.h"     // needed for vtkStdString
#include <vector>             // needed for std::vector

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVSystemInformation : public vtkPVInformation
{
public:
  static vtkPVSystemInformation* New();
  vtkTypeMacro(vtkPVSystemInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  /**
   * Merge another information object.
   */
  void AddInformation(vtkPVInformation*) override;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  //@}

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

  //  Provides access to the vector of information.
  const std::vector<SystemInformationType>& GetSystemInformations()
  {
    return this->SystemInformations;
  }

protected:
  vtkPVSystemInformation();
  ~vtkPVSystemInformation() override;

  std::vector<SystemInformationType> SystemInformations;

private:
  vtkPVSystemInformation(const vtkPVSystemInformation&) = delete;
  void operator=(const vtkPVSystemInformation&) = delete;
};

#endif
