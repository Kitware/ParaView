// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkPVInformation.h"
#include "vtkProcessModule.h"      // needed for vtkProcessModule::ProcessTypes
#include "vtkRemotingCoreModule.h" //needed for exports
#include <vector>                  // needed for std::vector

class VTKREMOTINGCORE_EXPORT vtkPVSystemInformation : public vtkPVInformation
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

  ///@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  ///@}

  struct SystemInformationType
  {
    vtkProcessModule::ProcessTypes ProcessType;
    int ProcessId; // for parallel processes, this indicates the process id.
    int NumberOfProcesses;
    std::string Hostname;
    std::string OSName;
    std::string OSRelease;
    std::string OSVersion;
    std::string OSPlatform;
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
