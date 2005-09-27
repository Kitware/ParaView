/*=========================================================================

  Program:   ParaView
  Module:    vtkHDF5RawImageReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHDF5RawImageReader - Read some HDF5 containing image data.
// .SECTION Description

#ifndef __vtkHDF5RawImageReader_h
#define __vtkHDF5RawImageReader_h

#include "vtkSource.h"

//BTX
class vtkCallbackCommand;
class vtkDataArray;
class vtkDataArraySelection;
class vtkImageData;
class vtkHDF5RawImageReaderDataSetsType;
class vtkHDF5RawImageReaderDataSet;
class vtkHDF5RawImageReader;

void vtkHDF5RawImageReaderAddDataSet(vtkHDF5RawImageReader* reader,
                                     vtkHDF5RawImageReaderDataSet* ds);
//ETX

class VTK_EXPORT vtkHDF5RawImageReader : public vtkSource
{
public:
  static vtkHDF5RawImageReader *New();
  vtkTypeRevisionMacro(vtkHDF5RawImageReader,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the reader's output.
  void SetOutput(vtkImageData *output);
  vtkImageData *GetOutput();
  vtkImageData *GetOutput(int idx);
  
  // Description:
  // Set or get the file name of the HDF5 file. 
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Set or get the subsample step.
  vtkSetVector3Macro(Stride, int);
  vtkGetVector3Macro(Stride, int);

  // Description:
  // Test whether the file with the given name can be read by this
  // reader.
  virtual int CanReadFile(const char* name);
  
  // Description:
  // Get the data array selection tables used to configure which data
  // arrays are loaded by the reader.
  vtkGetObjectMacro(PointDataArraySelection, vtkDataArraySelection);
  vtkGetObjectMacro(CellDataArraySelection, vtkDataArraySelection);
  
  // Description:  
  // Get the number of point or cell arrays available in the input.
  int GetNumberOfPointArrays();
  int GetNumberOfCellArrays();
  
  // Description:
  // Get the name of the point or cell array with the given index in
  // the input.
  const char* GetPointArrayName(int index);
  const char* GetCellArrayName(int index);
  
  // Description:
  // Get/Set whether the point or cell array with the given name is to
  // be read.
  int GetPointArrayStatus(const char* name);
  int GetCellArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);  
  void SetCellArrayStatus(const char* name, int status);  
  
protected:
  vtkHDF5RawImageReader();
  ~vtkHDF5RawImageReader();
   
  virtual void ExecuteInformation();
  virtual void Execute();

  // Description:
  // Strings that describe the data.
  char* FileName;
  int InformationError;

  // Are wholeExtent and updateExtent same?
  int UpdateExtentIsWholeExtent();

  // The list of data sets in the input file.
  vtkHDF5RawImageReaderDataSetsType* AvailableDataSets;
  
  void SetToEmptyExtent(int* extent);
  void ConvertDimsToExtent(int rank, const int* dims, int* extent);
  
  //BTX
  void AddDataSet(vtkHDF5RawImageReaderDataSet* ds);
  
  friend void vtkHDF5RawImageReaderAddDataSet(vtkHDF5RawImageReader* reader,
                                              vtkHDF5RawImageReaderDataSet* ds);
  //ETX

  // The array selections.
  vtkDataArraySelection* PointDataArraySelection;
  vtkDataArraySelection* CellDataArraySelection;
  
  // The observer to modify this object when the array selections are
  // modified.
  vtkCallbackCommand* SelectionObserver;
  
  char** CreateStringArray(int numStrings);
  void DestroyStringArray(int numStrings, char** strings);  
  
  // Check whether the given array is enabled.
  int PointDataArrayIsEnabled(const vtkHDF5RawImageReaderDataSet* ds);
  int CellDataArrayIsEnabled(const vtkHDF5RawImageReaderDataSet* ds);
  
  // Setup the data array selections for the input's set of arrays.
  void SetDataArraySelections(vtkDataArraySelection* sel);
  
  // Callback registered with the SelectionObserver.
  static void SelectionModifiedCallback(vtkObject* caller, unsigned long eid,
                                        void* clientdata, void* calldata);
  
private:
  int UpdateExtent[6];
  int WholeExtent[6];
  int Stride[3];
  int Rank;
  int Total[3];

  vtkHDF5RawImageReader(const vtkHDF5RawImageReader&);  // Not implemented.
  void operator=(const vtkHDF5RawImageReader&);  // Not implemented.
};

#endif
