/*=========================================================================

  Program:   ParaView
  Module:    vtkGmshMetaReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkGmshMetaReader
 *
 * Reader for visualization of high-order polynomial solutions under
 * the Gmsh format.
 * @par Thanks:
 * ParaViewGmshReaderPlugin - Copyright (C) 2015 Cenaero
 * See the Copyright.txt and License.txt files provided
 * with ParaViewGmshReaderPlugin for license information.
 *
 */

#ifndef vtkGmshMetaReader_h
#define vtkGmshMetaReader_h

#include "vtkGmshReaderModule.h"
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"

class vtkCallbackCommand;
class vtkDataArraySelection;
class vtkGmshReader;
class vtkPVXMLParser;
struct vtkGmshMetaReaderInternal;

class VTKGMSHREADER_EXPORT vtkGmshMetaReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkGmshMetaReader* New();
  vtkTypeMacro(vtkGmshMetaReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set and get the GMsh meta file name
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Set the step number for the geometry.
   */
  vtkSetClampMacro(TimeStepIndex, int, 0, VTK_INT_MAX);
  vtkGetMacro(TimeStepIndex, int);
  //@}

  /**
   * The min and max values of timesteps.
   */
  vtkGetVector2Macro(TimeStepRange, int);

  //@{
  /** The following methods allow selective reading of solutions fields. By
   * default, ALL point data fields are read,
   * but this can be modified (e.g. from the ParaView GUI).
   */
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int index);
  int GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);

  void DisableAllPointArrays();
  void EnableAllPointArrays();
  //@}

  //@{
  /**
   * Set and get the adaptation level
   */
  vtkSetMacro(AdaptationLevel, int);
  vtkGetMacro(AdaptationLevel, int);
  vtkGetMacro(AdaptationLevelInfo, int);
  //@}

  //@{
  /**
   * Set and get the adaptation tolerance
   */
  vtkSetMacro(AdaptationTolerance, double);
  vtkGetMacro(AdaptationTolerance, double);
  vtkGetMacro(AdaptationToleranceInfo, double);
  //@}

  /**
   * Static method to know if a file can be read
   * based on its filename
   */
  static bool CanReadFile(const char* filename);

protected:
  vtkGmshMetaReader();
  ~vtkGmshMetaReader() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  static void SelectionModifiedCallback(
    vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  char* FileName;

  int TimeStepIndex;
  int TimeStepRange[2];

  vtkNew<vtkGmshReader> Reader;
  vtkPVXMLParser* Parser;

  vtkNew<vtkCallbackCommand> SelectionObserver;
  vtkNew<vtkDataArraySelection> PointDataArraySelection;

  int ActualTimeStep;
  int AdaptationLevel;
  int AdaptationLevelInfo;
  double AdaptationTolerance;
  double AdaptationToleranceInfo;

private:
  vtkGmshMetaReaderInternal* Internal;

  vtkGmshMetaReader(const vtkGmshMetaReader&) = delete;
  void operator=(const vtkGmshMetaReader&) = delete;
};

#endif
