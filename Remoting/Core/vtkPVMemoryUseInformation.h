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
 * @class   vtkPVMemoryUseInformation
 *
 * A vtkClientServerStream serializable container for a single process's
 * instantaneous memory usage.
*/

#ifndef vtkPVMemoryUseInformation_h
#define vtkPVMemoryUseInformation_h

#include "vtkPVInformation.h"

#include <vector> // needed for std::vector
using std::vector;

class vtkClientServerStream;

class VTKREMOTINGCORE_EXPORT vtkPVMemoryUseInformation : public vtkPVInformation
{
public:
  static vtkPVMemoryUseInformation* New();
  vtkTypeMacro(vtkPVMemoryUseInformation, vtkPVInformation);
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

  /**
   * access the managed information.
   */
  size_t GetSize() { return this->MemInfos.size(); }
  int GetProcessType(int i) { return this->MemInfos[i].ProcessType; }
  int GetRank(int i) { return this->MemInfos[i].Rank; }
  long long GetProcMemoryUse(int i) { return this->MemInfos[i].ProcMemUse; }
  long long GetHostMemoryUse(int i) { return this->MemInfos[i].HostMemUse; }

protected:
  vtkPVMemoryUseInformation();
  ~vtkPVMemoryUseInformation() override;

private:
  class MemInfo
  {
  public:
    MemInfo()
      : ProcessType(-1)
      , Rank(0)
      , ProcMemUse(0)
      , HostMemUse(0)
    {
    }
    void Print();

  public:
    int ProcessType;
    int Rank;
    long long ProcMemUse;
    long long HostMemUse;
  };
  vector<MemInfo> MemInfos;

private:
  vtkPVMemoryUseInformation(const vtkPVMemoryUseInformation&) = delete;
  void operator=(const vtkPVMemoryUseInformation&) = delete;
};

#endif
