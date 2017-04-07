/*=========================================================================

  Program:   ParaView
  Module:    vtkFileSeriesHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkFileSeriesHelper
 * @brief Helper class to process file series.
 *
 * vtkFileSeriesHelper is intended to be a helper class that processes file
 * series. File series encountered in ParaView are of two types: temporal or
 * spatial. This class encapsulates logic to determine which form it is in.
 *
 * Currently, this is used by vtkCGNSFileSeriesReader. Eventually, we should be
 * able to refactor vtkFileSeriesReader to use this class.
 */

#ifndef vtkFileSeriesHelper_h
#define vtkFileSeriesHelper_h

#include "vtkObject.h"

#include "vtkPVVTKExtensionsCGNSReaderModule.h" // for export macros
#include <string>                               // for std::string
#include <utility>                              // for std::pair
#include <vector>                               // for std::vector

class vtkAlgorithm;
class vtkMultiProcessController;
class vtkInformation;
class vtkMultiProcessStream;

class VTKPVVTKEXTENSIONSCGNSREADER_EXPORT vtkFileSeriesHelper : public vtkObject
{
public:
  static vtkFileSeriesHelper* New();
  vtkTypeMacro(vtkFileSeriesHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  typedef bool (*FileNameFunctorType)(vtkAlgorithm* reader, const std::string& filename);

  //@{
  /**
   * Specify the set of files that comprise the series.
   */
  void RemoveAllFileNames();
  void AddFileName(const char* fname);
  void SetFileNames(const std::vector<std::string>& filenames);
  //@}

  /**
   * Get the number of files in the series.
   */
  unsigned int GetNumberOfFiles() const;

  /**
   * Setup file names in the series using a meta-file. The meta-file is simply
   * lists the names of the files in the series sequentially in an ASCII file.
   * All files in the file are relative to the location of the meta-file or
   * absolute paths.
   *
   * This will remove an preexisting files added to the helper irrespective of
   * whether reading of the meta-file succeeded or not.
   *
   * @returns false if failed to read the meta-file, true otherwise.
   */
  virtual bool ReadMetaFile(const char* metafilename);

  //@{
  /**
   * Get/Set the parallel controller. By default
   * vtkMultiProcessController::GetGlobalController() will be used.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  //@{
  /**
   * In vtkFileSeriesHelper::UpdateInformation,
   * vtkFileSeriesHelper tries to determine time information from the reader by
   * making it read each of the files and then collecting the timesteps provided
   * by each. Sometimes, however, the time steps in the files in the fileseries
   * are invalid (or same) in which case one may want to to simply use the
   * time-step index as the time value. For that case, one should set
   * `IgnoreReaderTime` to `true` (default: `false`).
   */
  vtkSetMacro(IgnoreReaderTime, bool);
  vtkGetMacro(IgnoreReaderTime, bool);
  vtkBooleanMacro(IgnoreReaderTime, bool);
  //@}

  /**
   * vtkFileSeriesHelper needs to collect information about the nature of the
   * fileseries. This method should be called to collect this information.
   *
   * This method has any effect only when the filenames (or any ivar
   * that could affect the times e.g. IgnoreReaderTime) have changed, hence
   * calling this repeatedly is acceptable.
   *
   * @param[in] reader pointer to the single file reader to use.
   * @param[in] ftor callback to set the filename on the reader.
   * @returns false if no filenames are specified or failed to collect
   * meta-data.
   */
  virtual bool UpdateInformation(vtkAlgorithm* reader, const FileNameFunctorType& ftor);

  /**
   * Returns the timesteps determined. This method will return valid values only
   * after a successful call to `UpdateInformation`.
   */
  const std::vector<double>& GetTimeSteps() const { return this->AggregatedTimeSteps; }

  /**
   * Returns the time range determined.
   */
  const std::pair<double, double>& GetTimeRange(bool* isvalid = NULL) const
  {
    if (isvalid != NULL)
    {
      *isvalid = this->AggregatedTimeRangeValid;
    }
    return this->AggregatedTimeRange;
  }

  /**
   * Fills up info with information about timesteps and timerange.
   */
  void FillTimeInformation(vtkInformation* info) const;

  /**
   * Returns true if the file series is a series of partitions rather than a
   * series of timesteps.
   */
  vtkGetMacro(PartitionedFiles, bool);

  /**
   * Returns the list of files to read on current rank to satisfy the request.
   * @param outInfo vtkInformation object with `UPDATE_*` keys that indicate the
   *                requested time and piece information.
   */
  std::vector<std::string> GetActiveFiles(vtkInformation* outInfo) const;

protected:
  vtkFileSeriesHelper();
  ~vtkFileSeriesHelper();

  class vtkTimeInformation
  {
  public:
    vtkTimeInformation(vtkInformation* outInfo);
    vtkTimeInformation(double time);
    vtkTimeInformation();
    bool operator==(const vtkTimeInformation& other) const;

    const std::pair<double, double>& GetTimeRange() const { return this->TimeRange; }
    const std::vector<double>& GetTimeSteps() const { return this->TimeSteps; }
    bool GetTimeStepsValid() const { return this->TimeStepsValid; }
    bool GetTimeRangeValid() const { return this->TimeRangeValid; }

    void Save(vtkMultiProcessStream& stream) const;
    void Load(vtkMultiProcessStream& stream);

  private:
    std::pair<double, double> TimeRange;
    std::vector<double> TimeSteps;
    bool TimeRangeValid;
    bool TimeStepsValid;
  };

  vtkMultiProcessController* Controller;
  std::vector<std::string> FileNames;
  bool IgnoreReaderTime;
  bool PartitionedFiles;
  std::vector<vtkTimeInformation> Information;

private:
  vtkFileSeriesHelper(const vtkFileSeriesHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkFileSeriesHelper&) VTK_DELETE_FUNCTION;

  void Broadcast(int srcRank);

  std::vector<double> AggregatedTimeSteps;
  bool AggregatedTimeRangeValid;
  std::pair<double, double> AggregatedTimeRange;
  vtkTimeStamp UpdateInformationTime;
};

#endif
