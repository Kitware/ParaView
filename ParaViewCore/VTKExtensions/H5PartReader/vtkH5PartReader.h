/*=========================================================================

  Program:   ParaView
  Module:    vtkH5PartReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Project                 : vtkCSCS
  Module                  : vtkH5PartReader.h
  Revision of last commit : $Rev: 754 $
  Author of last commit   : $Author: utkarsh $
  Date of last commit     : $Date: 2009-10-01 17:55:30 $

  Copyright (C) CSCS - Swiss National Supercomputing Centre.
  You may use modify and and distribute this code freely providing
  1) This copyright notice appears on all copies of source code
  2) An acknowledgment appears with any substantial usage of the code
  3) If this code is contributed to any other open source project, it
  must not be reformatted such that the indentation, bracketing or
  overall style is modified significantly.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

=========================================================================*/
// .NAME vtkH5PartReader - Write H5Part (HDF5) Particle files
// .SECTION Description
// vtkH5PartReader reads compatible with H5Part : documented here
// http://amas.web.psi.ch/docs/H5Part-doc/h5part.html
// .SECTION Thanks
// John Bidiscombe of
// CSCS - Swiss National Supercomputing Centre for creating and contributing
// this class.

#ifndef vtkH5PartReader_h
#define vtkH5PartReader_h

#include "vtkPVConfig.h" // For PARAVIEW_USE_MPI
#include "vtkPolyDataAlgorithm.h"
#include <string> // for string
#include <vector> // for vector

#include "vtkPVVTKExtensionsH5PartReaderModule.h" // for export macro

class vtkDataArraySelection;
class vtkMultiProcessController;

struct H5PartFile;

class VTKPVVTKEXTENSIONSH5PARTREADER_EXPORT vtkH5PartReader : public vtkPolyDataAlgorithm
{
public:
  static vtkH5PartReader* New();
  vtkTypeMacro(vtkH5PartReader, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // Description:
  // Specify file name.
  void SetFileName(char* filename);
  vtkGetStringMacro(FileName);

  // Description:
  // Set/Get the array that will be used for the X coordinates
  vtkGetStringMacro(Xarray);
  vtkSetStringMacro(Xarray);

  // Description:
  // Set/Get the array that will be used for the Y coordinates
  vtkGetStringMacro(Yarray);
  vtkSetStringMacro(Yarray);

  // Description:
  // Set/Get the array that will be used for the Z coordinates
  vtkGetStringMacro(Zarray);
  vtkSetStringMacro(Zarray);

  // Description:
  // Set/Get the timestep to be read
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);

  // Description:
  // Get the number of timesteps in the file
  vtkGetMacro(NumberOfTimeSteps, int);

  // Description:
  // When set (default no), the reader will generate a vertex cell
  // for each point/particle read. When using the points directly
  // this is unnecessary and time can be saved by omitting cell generation
  // vtkPointSpriteMapper does not require them.
  // When using ParaView, cell generation is recommended, without them
  // many filter operations are unavailable
  vtkSetMacro(GenerateVertexCells, int);
  vtkGetMacro(GenerateVertexCells, int);
  vtkBooleanMacro(GenerateVertexCells, int);

  // Description:
  // When this option is set, scalar fields with names which form a pattern
  // of the form scalar_0, scalar_1, scalar_2 will be combined into a single
  // vector field with N components
  vtkSetMacro(CombineVectorComponents, int);
  vtkGetMacro(CombineVectorComponents, int);
  vtkBooleanMacro(CombineVectorComponents, int);

  // Description:
  // Normally, a request for data at time t=x, where x is either before the start of
  // time for the data, or after the end, will result in the first or last
  // timestep of data to be retrieved (time is clamped to max/min values).
  // Forsome applications/animations, it may be desirable to not display data
  // for invalid times. When MaskOutOfTimeRangeOutput is set to ON, the reader
  // will return an empty dataset for out of range requests. This helps
  // avoid corruption of animations.
  vtkSetMacro(MaskOutOfTimeRangeOutput, int);
  vtkGetMacro(MaskOutOfTimeRangeOutput, int);
  vtkBooleanMacro(MaskOutOfTimeRangeOutput, int);

  bool HasStep(int Step);

  // Description:
  // An H5Part file may contain multiple arrays
  // a GUI (eg Paraview) can provide a mechanism for selecting which data arrays
  // are to be read from the file. The PointArray variables and members can
  // be used to query the names and number of arrays available
  // and set the status (on/off) for each array, thereby controlling which
  // should be read from the file. Paraview queries these point arrays after
  // the (update) information part of the pipeline has been updated, and before the
  // (update) data part is updated.
  int GetNumberOfPointArrays();
  const char* GetPointArrayName(int index);
  int GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void DisableAll();
  void EnableAll();
  void Disable(const char* name);
  void Enable(const char* name);
  //
  int GetNumberOfPointArrayStatusArrays() { return GetNumberOfPointArrays(); }
  const char* GetPointArrayStatusArrayName(int index) { return GetPointArrayName(index); }
  int GetPointArrayStatusArrayStatus(const char* name) { return GetPointArrayStatus(name); }
  void SetPointArrayStatusArrayStatus(const char* name, int status)
  {
    SetPointArrayStatus(name, status);
  }

  int GetNumberOfCoordinateArrays() { return GetNumberOfPointArrays(); }
  const char* GetCoordinateArrayName(int index) { return GetPointArrayName(index); }
  int GetCoordinateArrayStatus(const char* name);
  void SetCoordinateArrayStatus(const char* name, int status);

#ifdef PARAVIEW_USE_MPI

  // Description:
  // Set/Get the controller use in compositing (set to
  // the global controller by default)
  // If not using the default, this must be called before any
  // other methods.
  virtual void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

#endif

protected:
  vtkH5PartReader();
  ~vtkH5PartReader();
  //
  int RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;
  int OpenFile();
  void CloseFile();
  //  void  CopyIntoCoords(int offset, vtkDataArray *source, vtkDataArray *dest);
  // returns 0 if no, returns 1,2,3,45 etc for the first, second...
  // example : if CombineVectorComponents is true, then
  // velocity_0 returns 1, velocity_1 returns 2 etc
  // if CombineVectorComponents is false, then
  // velocity_0 returns 0, velocity_1 returns 0 etc
  int IndexOfVectorComponent(const char* name);

  std::string NameOfVectorComponent(const char* name);

  //
  // Internal Variables
  //
  char* FileName;
  int NumberOfTimeSteps;
  int TimeStep;
  int ActualTimeStep;
  double TimeStepTolerance;
  int CombineVectorComponents;
  int GenerateVertexCells;
  H5PartFile* H5FileId;
  vtkTimeStamp FileModifiedTime;
  vtkTimeStamp FileOpenedTime;
  int UpdatePiece;
  int UpdateNumPieces;
  int MaskOutOfTimeRangeOutput;
  int TimeOutOfRange;
  //
  char* Xarray;
  char* Yarray;
  char* Zarray;

  std::vector<double> TimeStepValues;
  typedef std::vector<std::string> stringlist;
  std::vector<stringlist> FieldArrays;

  // To allow paraview gui to enable/disable scalar reading
  vtkDataArraySelection* PointDataArraySelection;

  // To allow paraview gui to enable/disable scalar reading
  vtkDataArraySelection* CoordinateSelection;

#ifdef PARAVIEW_USE_MPI

  vtkMultiProcessController* Controller;

#endif

private:
  vtkH5PartReader(const vtkH5PartReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkH5PartReader&) VTK_DELETE_FUNCTION;
};

#endif
