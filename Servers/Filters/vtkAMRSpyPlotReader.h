/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRSpyPlotReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAMRSpyPlotReader - Reader for SPCTH Spy Plot file
// .SECTION Description
// vtkAMRSpyPlotReader is a reader that reads SPCTH Spy Plot file format
// .SECTION Caveats
// Assumption 1: All processors read first file in case file when
// running 'ExecuteInformation'
// Assumption 2: The first file contains all the cell array name variables
// Assumption 3: The first timestep contains all the call array name variables

#ifndef __vtkAMRSpyPlotReader_h
#define __vtkAMRSpyPlotReader_h

#include "vtkCTHSource.h"

class vtkDataArraySelection;
class vtkCallbackCommand;
class vtkAMRSpyPlotReaderInternal;
class vtkMultiProcessController;
class vtkDataSetAttributes;
class vtkDataArray;

class VTK_EXPORT vtkAMRSpyPlotReader : public vtkCTHSource
{
public:
  static vtkAMRSpyPlotReader* New();
  vtkTypeRevisionMacro(vtkAMRSpyPlotReader,vtkCTHSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get and set the file name
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Set and get the time step
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);

  // Description:
  // Get the time step range
  vtkGetVector2Macro(TimeStepRange, int);

  // Description:
  // Get the data array selection tables used to configure which data
  // arrays are loaded by the reader.
  vtkGetObjectMacro(PointDataArraySelection, vtkDataArraySelection);
  vtkGetObjectMacro(CellDataArraySelection, vtkDataArraySelection);
  
  // Description:
  // Cell array selection
  int GetNumberOfCellArrays();
  const char* GetCellArrayName(int idx);
  int GetCellArrayStatus(const char* name);
  void SetCellArrayStatus(const char* name, int status);  

  // Description:
  // SetThe controller used to coordinate parallel reading.
  void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller,vtkMultiProcessController);

protected:
  vtkAMRSpyPlotReader();
  ~vtkAMRSpyPlotReader();

  // The array selections.
  vtkDataArraySelection* PointDataArraySelection;
  vtkDataArraySelection* CellDataArraySelection;

  // Callback registered with the SelectionObserver.
  static void SelectionModifiedCallback(vtkObject* caller, unsigned long eid,
                                        void* clientdata, void* calldata);

  // Description:
  // This does the updating of meta data of the dataset
  void UpdateMetaData(const char* fname);

  // Description:
  // This does the updating of the meta data of the case file
  void UpdateCaseFile(const char* fname);

  virtual void ExecuteInformation();
  virtual void Execute();
  void AddGhostLevelArray(int numLevels);

  // Description:
  // Get and set the current file name. Protected because
  // this method should only be used internally
  vtkSetStringMacro(CurrentFileName);
  vtkGetStringMacro(CurrentFileName);

  // The observer to modify this object when the array selections are
  // modified.
  vtkCallbackCommand* SelectionObserver;
  char* FileName;
  char* CurrentFileName;
  int TimeStep;
  int TimeStepRange[2];

  vtkMultiProcessController* Controller;
  
  void MergeVectors(vtkDataSetAttributes* da);
  int MergeVectors(vtkDataSetAttributes* da, 
                   vtkDataArray * a1, vtkDataArray * a2);
  int MergeVectors(vtkDataSetAttributes* da, 
                   vtkDataArray * a1, vtkDataArray * a2, vtkDataArray * a3);

private:
  vtkAMRSpyPlotReader(const vtkAMRSpyPlotReader&);  // Not implemented.
  void operator=(const vtkAMRSpyPlotReader&);  // Not implemented.

  vtkAMRSpyPlotReaderInternal* Internals;
};

#endif

