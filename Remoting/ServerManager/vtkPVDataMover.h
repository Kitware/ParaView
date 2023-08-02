// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPVDataMover
 * @brief helper to move data between ParaView processes.
 *
 * vtkPVDataMover is used to transfer data between ParView processes without any
 * transformations. This is primarily used by `simple.FetchData` to transfer
 * data to the client.
 */

#ifndef vtkPVDataMover_h
#define vtkPVDataMover_h

#include "vtkObject.h"
#include "vtkRemotingServerManagerModule.h" // for exports

#include <map>    // for std::map
#include <vector> // for std::vector

class vtkAlgorithm;
class vtkDataObject;

class VTKREMOTINGSERVERMANAGER_EXPORT vtkPVDataMover : public vtkObject
{
public:
  static vtkPVDataMover* New();
  vtkTypeMacro(vtkPVDataMover, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the data-producer to fetch data from.
   */
  void SetProducer(vtkAlgorithm* producer);
  vtkGetObjectMacro(Producer, vtkAlgorithm);
  ///@}

  ///@{
  /**
   * Get/Set the output-port number to use to fetch data. Defaults to `0`.
   */
  vtkSetClampMacro(PortNumber, int, 0, VTK_INT_MAX);
  vtkGetMacro(PortNumber, int);
  ///@}

  ///@{
  /**
   * In symmetric batch mode, set this to true to indicate that the data must be
   * cloned on all ranks. Otherwise, the data is only generated on the root
   * node. Defaults to `false`.
   *
   * This has no effect in non-symmetric MPI mode and is simply ignored.
   */
  vtkSetMacro(GatherOnAllRanks, bool);
  vtkGetMacro(GatherOnAllRanks, bool);
  ///@}

  ///@{
  /**
   * When set to true (default is false), skips moving empty datasets. A dataset
   * is treated as empty if it has no cells, points, etc. i.e.
   * `vtkDataObject::GetNumberOfElements` for all types returns 0.
   */
  vtkSetMacro(SkipEmptyDataSets, bool);
  vtkGetMacro(SkipEmptyDataSets, bool);
  vtkBooleanMacro(SkipEmptyDataSets, bool);
  ///@}

  ///@}

  ///@{
  /**
   * API to select source ranks. This allows users to limit fetching data from
   * only certain ranks as listed. If none are provided, default, data from all
   * ranks is fetched. Otherwise, data only from the ranks listed is fetched.
   */
  void AddSourceRank(int rank);
  void ClearAllSourceRanks();
  void SetSourceRanks(const std::vector<int>& ranks);
  const std::vector<int>& GetSourceRanks() const { return this->SourceRanks; }
  ///@}

  /**
   * Once the helper has been setup, use this method to fetch the data.
   */
  bool Execute();

  ///@{
  /**
   */
  unsigned int GetNumberOfDataSets() const;
  int GetDataSetRank(unsigned int index) const;
  vtkDataObject* GetDataSetAtIndex(unsigned int index) const;
  vtkDataObject* GetDataSetFromRank(int rank) const;
  ///@}
protected:
  vtkPVDataMover();
  ~vtkPVDataMover() override;

private:
  vtkPVDataMover(const vtkPVDataMover&) = delete;
  void operator=(const vtkPVDataMover&) = delete;

  vtkAlgorithm* Producer = nullptr;
  int PortNumber = 0;
  bool GatherOnAllRanks = false;
  bool SkipEmptyDataSets = false;

  std::vector<int> SourceRanks;
  std::map<int, vtkSmartPointer<vtkDataObject>> DataSets;
};

#endif
