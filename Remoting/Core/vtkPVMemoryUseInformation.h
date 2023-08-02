// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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

  ///@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  ///@}

  /**
   * access the managed information.
   */
  size_t GetSize() { return this->MemInfos.size(); }
  int GetProcessType(size_t i) { return this->MemInfos[i].ProcessType; }
  int GetRank(size_t i) { return this->MemInfos[i].Rank; }
  long long GetProcMemoryUse(size_t i) { return this->MemInfos[i].ProcMemUse; }
  long long GetHostMemoryUse(size_t i) { return this->MemInfos[i].HostMemUse; }

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

    int ProcessType;
    int Rank;
    long long ProcMemUse;
    long long HostMemUse;
  };
  vector<MemInfo> MemInfos;

  vtkPVMemoryUseInformation(const vtkPVMemoryUseInformation&) = delete;
  void operator=(const vtkPVMemoryUseInformation&) = delete;
};

#endif
