/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkAcuSolveReaderProxy.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAcuSolveReaderProxy - This dummy reader has the correct API, but
// just produces junk output.
// .SECTION Description
// The user can select the filename, node attributes to load, and time step.
// It handles multiple sets by having multiple outputs.
// It only produces unstructured grid outputs.
// It will not read partitions of the data.  

#ifndef __vtkAcuSolveReaderProxy
#define __vtkAcuSolveReaderProxy

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkDynamicLoader.h" // Needed for vtkLibHandle;

class vtkAcuSolveReaderBase;
class vtkCallbackCommand;
class vtkDataArray;
class vtkDataArraySelection;
class vtkDataSet;
class vtkDataSetAttributes;
class vtkUnstructuredGrid;

class VTK_EXPORT vtkAcuSolveReaderProxy : public vtkMultiBlockDataSetAlgorithm
{

public:
  static vtkAcuSolveReaderProxy *New();
  vtkTypeRevisionMacro(vtkAcuSolveReaderProxy,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
        
  // Description:
  // Get/Set the name of the input file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
        
  // Description:
  // Test whether the file with the given name can be read by this
  // reader.
  virtual int CanReadFile(const char* name);
        
  // Description:
  // Get the data array selection tables used to configure which 
  // data arrays are loaded by the reader.
  vtkGetObjectMacro(PointDataArraySelection, vtkDataArraySelection);
        
  // Description:
  // Get the number of point arrays available in the input.
  int GetNumberOfPointArrays();
        
  // Description:
  // Get the name of the point array with the given index in
  // the input.
  const char* GetPointArrayName(int index);
        
  // Description:
  // Get/Set whether the point or cell array with the given name 
  // is to be read.
  int GetPointArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
        
  // Description:
  // I am assuming that time steps are just specified as an index.
  // You will have to add the variables that transform the index 
  // into real world time values.
  vtkGetMacro(NumberOfTimeSteps, int);
        
  // Description:
  // Set this index before updating to get a specific time step.
  // I am indexing time steps starting at 0.
  //vtkSetMacro(UpdateTimeStep, int);
  //vtkGetMacro(UpdateTimeStep, int);
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);

  // To support the latest time step and its bounds in the GUI
  vtkGetVector2Macro(TimeStepRange, int );
  vtkSetVector2Macro(TimeStepRange, int );
        
protected:
  vtkAcuSolveReaderProxy();
  ~vtkAcuSolveReaderProxy();
        
  // Standard pipeline exectution methods.
  vtkDynamicLoader* adbLoader;
  vtkLibHandle adbLibHandle;

  vtkUnstructuredGrid* ReadSet(int set);
  void ReadGeometry();
  // Callback registered with the SelectionObserver.
        
  static void SelectionModifiedCallback(        vtkObject*      caller, 
                                                unsigned long   eid,
                                                void*           clientdata,
                                                void*           calldata);
        
  // For specifying data sets
  int NumberOfSets;
        
  // The input file's name.
  char* FileName;
        
  // AcuSolve DataReader Object
  vtkAcuSolveReaderBase* AdbReader;
  int ReadGeometryFlag;
        
  // For specifying time
  int NumberOfTimeSteps;
  //int UpdateTimeStep;
  int TimeStep;
        
  // For Ale data
  bool AleFlag;
  int PrevTimeStep;
        
  // The array selections.
  vtkDataArraySelection* PointDataArraySelection;

  // The observer to modify this object when the array 
  // selections are modified.
  vtkCallbackCommand* SelectionObserver;
        
  // Whether there was an error reading the file in ExecuteInformation.
  int InformationError;
        
  // Whether there was an error reading the file in ExecuteData.
  int DataError;
        
  // The current range over which progress is moving.  This allows for
  // incrementally fine-tuned progress updates.
  virtual void GetProgressRange(float*  range    );
  virtual void SetProgressRange(float*  range, 
                                int     curStep,
                                int     numSteps );

  virtual void SetProgressRange(float*  range,
                                int     curStep,
                                float*  fractions);
  virtual void UpdateProgressDiscrete(  float   progress);
  float ProgressRange[2];
        
  int TimeStepRange[2];
        
  // Caches because connectivity does not change between time steps.
  vtkUnstructuredGrid** Cache;
  void DeleteCache();
  char* CacheFileName;
  vtkSetStringMacro(CacheFileName);

  virtual int RequestInformation(vtkInformation *, 
                                 vtkInformationVector **, 
                                 vtkInformationVector *);
  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, 
                          vtkInformationVector *);

private:
  vtkAcuSolveReaderProxy(const vtkAcuSolveReaderProxy&); // Not Implemented
  void operator=(const vtkAcuSolveReaderProxy&); // Not Implemented
};

#endif
