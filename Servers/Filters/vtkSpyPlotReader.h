/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpyPlotReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSpyPlotReader - Read SPCTH Spy Plot file format
// .SECTION Description
// vtkSpyPlotReader is a reader that reads SPCTH Spy Plot file format
// through an ascii meta file called the "case" file (extension .spcth). This
// case file lists all the actual binary files that describe a dataset. Each
// binary file describes a part of the dataset. However, if only a single
// binary file describes the whole dataset, it is possible to load it directly
// without using a case file.
// The reader supports both Spy dataset types: flat mesh and AMR
// (Adaptive Mesh Refinement).
// It has parallel capabilities. Each processor is supposed to have access
// to the case file and to all the binary files. All the binary files
// have to be coherent: they describe the same fields of data.
// Each binary file may content multiple time stamp. The time stamp to read is
// specified with SetTimestamp().
//
// .SECTION Implementation Details
// - All processors read the first binary file listed in the case file to get
// informations about the fields.
// - Each block of data is already surrounded by ghost cells in the file,
// even on part of the block that don't have actual neighbor cells.
// - Each time step contains all the cell array name variables

#ifndef __vtkSpyPlotReader_h
#define __vtkSpyPlotReader_h

#include "vtkHierarchicalDataSetAlgorithm.h"

class vtkDataArraySelection;
class vtkCallbackCommand;
class vtkSpyPlotReaderMap;
class vtkMultiProcessController;
class vtkDataSetAttributes;
class vtkDataArray;

class VTK_EXPORT vtkSpyPlotReader : public vtkHierarchicalDataSetAlgorithm
{
public:
  static vtkSpyPlotReader* New();
  vtkTypeRevisionMacro(vtkSpyPlotReader,vtkHierarchicalDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get and set the file name. It is either the name of the case file or the
  // name of the single binary file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Set and get the time step. The time step is an index, NOT a time
  // given in seconds.
  vtkSetMacro(TimeStep, int);
  vtkGetMacro(TimeStep, int);

  // Description:
  // Get the time step range.
  vtkGetVector2Macro(TimeStepRange, int);

  // Description:
  // Get the data array selection tables used to configure which data
  // arrays are loaded by the reader.
  vtkGetObjectMacro(CellDataArraySelection, vtkDataArraySelection);
  
  // Description:
  // Cell array selection
  int GetNumberOfCellArrays();
  const char* GetCellArrayName(int idx);
  int GetCellArrayStatus(const char *name);
  void SetCellArrayStatus(const char *name, int status);  

  // Description:
  // Set the controller used to coordinate parallel reading.
  void SetController(vtkMultiProcessController* controller);
  
  // Return the controller used to coordinate parallel reading. By default,
  // it is the global controller.
  vtkGetObjectMacro(Controller,vtkMultiProcessController);

protected:
  vtkSpyPlotReader();
  ~vtkSpyPlotReader();

  // The array selections.
  vtkDataArraySelection *CellDataArraySelection;
  
  // Read the case file and the first binary file do get meta
  // informations (number of files, number of fields, number of timestep).
  virtual int RequestInformation(vtkInformation *request, 
                                 vtkInformationVector **inputVector, 
                                 vtkInformationVector *outputVector);
  
  // Read the data: get the number of pieces (=processors) and get
  // my piece id (=my processor id).
  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

  virtual int SetUpdateBlocks(vtkInformation *request,
                              vtkInformationVector **inputVector,
                              vtkInformationVector *outputVector);

  // Callback registered with the SelectionObserver.
  static void SelectionModifiedCallback(vtkObject *caller, unsigned long eid,
                                        void *clientdata, void *calldata);

  // Description:
  // This does the updating of meta data of the dataset from the
  // first binary file registered in the map:
  // - number of time steps
  // - number of fields
  // - name of fields
  int UpdateMetaData();

  // Description:
  // This does the updating of the meta data of the case file
  int UpdateCaseFile(const char *fname);

  void AddGhostLevelArray(int numLevels);

  // Description:
  // Get and set the current file name. Protected because
  // this method should only be used internally
  vtkSetStringMacro(CurrentFileName);
  vtkGetStringMacro(CurrentFileName);

  // The observer to modify this object when the array selections are
  // modified.
  vtkCallbackCommand *SelectionObserver;
  char *FileName;
  char *CurrentFileName;
  int TimeStep; // set by the user
  int TimeStepRange[2];
  int CurrentTimeStep; // computed

  int IsAMR; // AMR (hierarchy of uniform grids)
  // or flat mesh (set of rectilinear grids)?
  
  vtkMultiProcessController *Controller;
  
  // The file format stores a vector field as separated scalar component
  // fields. This method rebuilds the vector field from those scalar
  // component fields.
  void MergeVectors(vtkDataSetAttributes *da);
  int MergeVectors(vtkDataSetAttributes *da, 
                   vtkDataArray *a1,
                   vtkDataArray *a2);
  int MergeVectors(vtkDataSetAttributes *da, 
                   vtkDataArray *a1,
                   vtkDataArray *a2,
                   vtkDataArray *a3);
  
  vtkSpyPlotReaderMap *Map;
  
  // Description:
  // The processors are views as a heap tree. The root is the processor of
  // id 0.
  int GetParentProcessor(int proc);
  int GetLeftChildProcessor(int proc);
  
private:
  vtkSpyPlotReader(const vtkSpyPlotReader&);  // Not implemented.
  void operator=(const vtkSpyPlotReader&);  // Not implemented.
};

#endif
