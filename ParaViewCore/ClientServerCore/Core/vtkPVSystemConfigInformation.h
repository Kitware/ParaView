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
 * @class   vtkPVSystemConfigInformation
 *
 * A vtkClientServerStream serializable conatiner of information describing
 * memory configuration of the host of a single process.
*/

#ifndef vtkPVSystemConfigInformation_h
#define vtkPVSystemConfigInformation_h

#include "vtkPVInformation.h"

#include <string> // for string
using std::string;
#include <vector> // for vector
using std::vector;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVSystemConfigInformation : public vtkPVInformation
{
public:
  class ConfigInfo
  {
  public:
    ConfigInfo()
      : OSDescriptor("")
      , CPUDescriptor("")
      , MemDescriptor("")
      , HostName("")
      , ProcessType(-1)
      , SystemType(-1)
      , Rank(-1)
      , Pid(0)
      , HostMemoryTotal(0)
      , HostMemoryAvailable(0)
      , ProcMemoryAvailable(0)
    {
    }

    void Print();

    bool operator<(const ConfigInfo& other) const { return this->Rank < other.Rank; }

  public:
    string OSDescriptor;
    string CPUDescriptor;
    string MemDescriptor;
    string HostName;
    int ProcessType;
    int SystemType;
    int Rank;
    long long Pid;
    long long HostMemoryTotal;
    long long HostMemoryAvailable;
    long long ProcMemoryAvailable;
  };

public:
  static vtkPVSystemConfigInformation* New();
  vtkTypeMacro(vtkPVSystemConfigInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Transfer information about a single object into this object.
   */
  virtual void CopyFromObject(vtkObject* obj) VTK_OVERRIDE;

  /**
   * Merge another information object.
   */
  virtual void AddInformation(vtkPVInformation* info) VTK_OVERRIDE;

  //@{
  /**
   * Manage a serialized version of the information.
   */
  virtual void CopyToStream(vtkClientServerStream* css) VTK_OVERRIDE;
  virtual void CopyFromStream(const vtkClientServerStream* css) VTK_OVERRIDE;
  //@}

  /**
   * Access managed information
   */
  size_t GetSize() { return this->Configs.size(); }

  const char* GetOSDescriptor(size_t i) { return this->Configs[i].OSDescriptor.c_str(); }
  const char* GetCPUDescriptor(size_t i) { return this->Configs[i].CPUDescriptor.c_str(); }
  const char* GetMemoryDescriptor(size_t i) { return this->Configs[i].MemDescriptor.c_str(); }
  const char* GetHostName(size_t i) { return this->Configs[i].HostName.c_str(); }
  int GetProcessType(size_t i) { return this->Configs[i].ProcessType; }
  int GetSystemType(size_t i) { return this->Configs[i].SystemType; }
  int GetRank(size_t i) { return this->Configs[i].Rank; }
  long long GetPid(size_t i) { return this->Configs[i].Pid; }
  long long GetHostMemoryTotal(size_t i) { return this->Configs[i].HostMemoryTotal; }
  long long GetHostMemoryAvailable(size_t i) { return this->Configs[i].HostMemoryAvailable; }
  long long GetProcMemoryAvailable(size_t i) { return this->Configs[i].ProcMemoryAvailable; }

  /**
   * Sort elements by mpi rank.
   */
  void Sort();

protected:
  vtkPVSystemConfigInformation();
  ~vtkPVSystemConfigInformation();

private:
  vector<ConfigInfo> Configs;

private:
  vtkPVSystemConfigInformation(const vtkPVSystemConfigInformation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVSystemConfigInformation&) VTK_DELETE_FUNCTION;
};

#endif
