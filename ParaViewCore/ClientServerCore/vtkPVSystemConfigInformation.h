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
// .NAME vtkPVSystemConfigInformation
// .SECTION Description
// A vtkClientServerStream serializable conatiner of information describing
// memory configuration of the host of a single process.

#ifndef __vtkPVSystemConfigInformation_h
#define __vtkPVSystemConfigInformation_h

#include "vtkPVInformation.h"

#include "vtkStdString.h"

#include <string> // for string
using std::string;
#include <vector> // for vector
using std::vector;

class VTK_EXPORT vtkPVSystemConfigInformation : public vtkPVInformation
{
public:
  static vtkPVSystemConfigInformation* New();
  vtkTypeMacro(vtkPVSystemConfigInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject* obj);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation* info);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream *css);
  virtual void CopyFromStream(const vtkClientServerStream *css);
  //BTX
  //ETX

  // Description:
  // Access managed information
  int GetSize(){ return this->Configs.size(); }

  const char * GetOSDescriptor(int i){ return this->Configs[i].OSDescriptor.c_str(); }
  const char * GetCPUDescriptor(int i){ return this->Configs[i].CPUDescriptor.c_str(); }
  const char * GetMemoryDescriptor(int i){ return this->Configs[i].MemDescriptor.c_str(); }
  const char *GetHostName(int i){ return this->Configs[i].HostName.c_str(); }
  const char *GetFullyQualifiedDomainName(int i){ return this->Configs[i].FullyQualifiedDomainName.c_str(); }
  int GetProcessType(int i){ return this->Configs[i].ProcessType; }
  int GetSystemType(int i){ return this->Configs[i].SystemType; }
  int GetRank(int i){ return this->Configs[i].Rank; }
  unsigned long long GetPid(int i){ return this->Configs[i].Pid; }
  unsigned long long GetCapacity(int i){ return this->Configs[i].Capacity; }

protected:
  vtkPVSystemConfigInformation();
  ~vtkPVSystemConfigInformation();

private:
  //BTX
  class ConfigInfo
    {
    public:
      ConfigInfo()
            :
        OSDescriptor(""),
        CPUDescriptor(""),
        MemDescriptor(""),
        HostName(""),
        FullyQualifiedDomainName(""),
        ProcessType(-1),
        SystemType(-1),
        Rank(-1),
        Pid(0),
        Capacity(0)
      {}

      void Print();
    public:
      string OSDescriptor;
      string CPUDescriptor;
      string MemDescriptor;
      string HostName;
      string FullyQualifiedDomainName;
      int ProcessType;
      int SystemType;
      int Rank;
      unsigned long long Pid;
      unsigned long long Capacity;
    };

  vector<ConfigInfo> Configs;
  //ETX

private:
  vtkPVSystemConfigInformation(const vtkPVSystemConfigInformation&); // Not implemented
  void operator=(const vtkPVSystemConfigInformation&); // Not implemented
};

#endif
