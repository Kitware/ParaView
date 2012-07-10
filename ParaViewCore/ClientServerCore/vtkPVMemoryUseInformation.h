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
// .NAME vtkPVMemoryUseInformation
// .SECTION Description
// A vtkClientServerStream serializable container for a single process's
// instantaneous memory usage.

#ifndef __vtkPVMemoryUseInformation_h
#define __vtkPVMemoryUseInformation_h

#include "vtkPVInformation.h"

#include <vector> // needed for std::vector
using std::vector;

class vtkClientServerStream;

class VTK_EXPORT vtkPVMemoryUseInformation : public vtkPVInformation
{
public:
  static vtkPVMemoryUseInformation* New();
  vtkTypeMacro(vtkPVMemoryUseInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

  // Description:
  // access the managed information.
  size_t GetSize(){ return this->MemInfos.size(); }
  int GetProcessType(int i){ return this->MemInfos[i].ProcessType; }
  int GetRank(int i){ return this->MemInfos[i].Rank; }
  unsigned long long GetMemoryUse(int i){ return this->MemInfos[i].MemUse; }

protected:
  vtkPVMemoryUseInformation();
  ~vtkPVMemoryUseInformation();

private:
  //BTX
  class MemInfo
    {
    public:
      MemInfo() : ProcessType(-1), Rank(0), MemUse(0) {}
      void Print();
    public:
      int ProcessType;
      int Rank;
      unsigned long long MemUse;
    };
  vector<MemInfo> MemInfos;
  //ETX

private:
  vtkPVMemoryUseInformation(const vtkPVMemoryUseInformation&); // Not implemented
  void operator=(const vtkPVMemoryUseInformation&); // Not implemented
};

#endif
