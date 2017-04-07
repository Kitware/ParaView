/*=========================================================================

  Program:   ParaView
  Module:    vtkCGNSFileSeriesReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkCGNSFileSeriesReader
 * @brief Adds support for reading temporal or partitioned CGNS files.
 *
 * vtkCGNSFileSeriesReader is a meta-reader that add support for reading
 * CGNS file series using vtkCGNSReader. We encounter two types of file series
 * with CGNS:
 * \li 1. temporal file series - where each file is simply a single timestep.
 * \li 2. partitioned file series - where each file corresponds to data dumped
 *        out from a rank but has all timesteps.
 *
 *  vtkCGNSFileSeriesReader determines the nature of the file series
 *  encountered and reads the files accordingly. For partitioned files, the
 *  files are distributed among data-processing ranks, while for temporal file
 *  series, blocks are distributed among data-processing ranks (using logic in
 *  vtkCGNSReader itself).
 *
 *  @sa vtkFileSeriesHelper
 */

#ifndef vtkCGNSFileSeriesReader_h
#define vtkCGNSFileSeriesReader_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkNew.h"                             // for vtkNew.
#include "vtkPVVTKExtensionsCGNSReaderModule.h" // for export macros

#include <string> // for std::string
#include <vector> // for std::vector

class vtkFileSeriesHelper;
class vtkCGNSReader;
class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSCGNSREADER_EXPORT vtkCGNSFileSeriesReader
  : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkCGNSFileSeriesReader* New();
  vtkTypeMacro(vtkCGNSFileSeriesReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the controller.
   */
  void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  //@{
  /**
   * Get/Set the reader.
   */
  virtual void SetReader(vtkCGNSReader* reader);
  vtkGetObjectMacro(Reader, vtkCGNSReader);
  //@}

  /**
   * Test a file for readability. Ensure that vtkCGNSFileSeriesReader::SetReader
   * is called before using this method.
   */
  int CanReadFile(const char* filename);

  //@{
  /**
   * Add/remove files names in the file series.
   */
  void AddFileName(const char* fname);
  void RemoveAllFileNames();
  //@}

  //@{
  /**
   * If true, then treat file series like it does not contain any time step
   * values. False by default.
   */
  vtkGetMacro(IgnoreReaderTime, bool);
  vtkSetMacro(IgnoreReaderTime, bool);
  vtkBooleanMacro(IgnoreReaderTime, bool);
  //@}

  /**
   * Returns the filename being used for current timesteps.
   * This is only reasonable for temporal file series. For a partitioned file
   * series, this will return the filename being used on the current rank.
   */
  const char* GetCurrentFileName() const;

  /**
   * Overridden to setup the `Reader` and then forward the pass to the reader.
   */
  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

protected:
  vtkCGNSFileSeriesReader();
  ~vtkCGNSFileSeriesReader();

  /**
   * Handles the RequestData pass.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  /**
   * Update `this->ActiveFiles`, a collection of files to be read to satisfy the
   * current request.
   *
   * @returns false if the update failed for some reason, otherwise true.
   */
  bool UpdateActiveFileSet(vtkInformation* info);

  /**
   * Select the file from `this->ActiveFiles` at the given index and set that on
   * `this->Reader`.
   */
  void ChooseActiveFile(int index);

  vtkNew<vtkFileSeriesHelper> FileSeriesHelper;
  vtkCGNSReader* Reader;
  bool IgnoreReaderTime;

private:
  vtkCGNSFileSeriesReader(const vtkCGNSFileSeriesReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCGNSFileSeriesReader&) VTK_DELETE_FUNCTION;
  void OnReaderModifiedEvent();

  vtkMultiProcessController* Controller;
  unsigned long ReaderObserverId;
  bool InProcessRequest;
  std::vector<std::string> ActiveFiles;
};

#endif
