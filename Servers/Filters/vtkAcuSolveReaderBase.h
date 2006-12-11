/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkAcuSolveReaderBase.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
// .NAME vtkAcuSolveReaderBase - This reader accesses the AcuSolve database
// and creates the vtk points,cells and point data
// vtkAcuSolveReaderBase provides the AcuSolve log file name to this object
// and gets the point, cells and the point data
// It will not read partitions of the data.
// This class was developed by ACUSIM Software Inc.
// Please address all comments to support@acusim.com

#ifndef __vtkAcuSolveReaderBase_h
#define __vtkAcuSolveReaderBase_h

#include "vtkObject.h"

class vtkPoints;
class vtkCellArray;

typedef struct _Adb* AdbHd;

class vtkAcuSolveReaderBase : public vtkObject
{

public:

  //Description:
  //  Points or Nodes are the same. 
  // VTK uses points and AcuSolve uses Nodes
  //  Both the interfaces are provided just for convenience as 
  // this is not going to cause any excess overhead.
  //  GetNumberOfPoints or GetNumberOfNodes return number of nodes/points
  //    

  virtual vtkIdType GetNumberOfPoints() = 0;
  virtual vtkIdType GetNumberOfNodes() = 0;
        
  //Description:
  // Cells or Elements are the same. VTK uses cells , AcuSolve 
  // uses Elements. Both the interfaces are provided for convenience 
  // return number of cells/elements
  //    

  virtual vtkIdType GetNumberOfElements(int set) = 0;
  virtual vtkIdType GetNumberOfCells(int set) = 0;

  //Description:
  //  A set can be thought of as a part of the unstructured grid
  //  A surface  could be a set, a volume could be set.
  //  In AcuSolve each set should have elements/cells of same types
  //  e.g., set-1 might have cells that are all triangles
  //      set-2 might have cells that are all quads and so on

  virtual vtkIdType GetNumberOfElementSets() = 0;
  virtual vtkIdType GetNumberOfCellSets() = 0;


  //Description:
  //  These two functions return the type of cells/elements in the set
  //  return type could be 
  //    -       VTK_TRIANGLE
  //    -       VTK_QUAD
  //    -       VTK_TETRA
  //    -       VTK_HEXA
  //    -       VTK_PRISM

  virtual vtkIdType GetElementType(int set) = 0;
  virtual vtkIdType GetCellType(int set) = 0;

  //Description:
  //  These two functions return the name of cells/elements in the set

  virtual char* GetElementSetName(int set) = 0;
  virtual char* GetCellSetName(int set) = 0;

  //Description:
  //  These two functions return the pointer to vtkPoints object 
  //  containing *ALL* the points/nodes in the mesh 

  virtual vtkPoints* GetPoints(int stepId) = 0;
  virtual vtkPoints* GetNodes(int stepId) = 0;

  //Description:
  //  These two functions return the pointer to vtkCellArray object 
  //  containing cells/elements in a particular set 

  virtual vtkCellArray* GetCells(int set) = 0;
  virtual vtkCellArray* GetElements(int set) = 0;

  //Description:
  //  Utility Functions 
  //  With Ale PrintPoints always prints points/nodes corresponding
  //  to the stepId accesses the last, Basically current vector held
  //  by vtkPoints

  virtual void PrintPoints(char* filename) = 0;
  virtual void PrintCells(int set,char* filename) = 0;

  //Description:
  //  The following functions provide the number of point/nodal data  
  //  available for output. The key assumption is that the user is 
  //  using the same nodal output frequency for all the variables under
  //  consideration. 

  virtual vtkIdType GetNumberOfPointData() = 0;
  virtual vtkIdType GetNumberOfNodalData() = 0;

  //Description:
  //  The following functions provide the names of point/nodal data  
  //  available for output.  e.g., pressure, velocity, mesh_displacement etc.

  virtual char* GetPointOutputName(int outVar) = 0;
  virtual char* GetNodalOutputName(int outVar) = 0;

  //Description:
  //  The following functions provide the dimensions of point/nodal data  
  //  available for output.  e.g., pressure = 1, velocity = 3 etc.

  virtual int GetPointOutputDimension(int outVar) = 0;
  virtual int GetNodalOutputDimension(int outVar) = 0;

  //Description:
  //  The following functions provide the nodal/point data corresponding 
  //  to the variable pointed to by outVar at timestep = outStep.
  //  outStep is actually an index to the outputs. What it means is that if 
  //  the user is outputting his data once every 10 time steps, then
  //  timestep = 0 outStep = 0, 
  //  timestep = 10 outStep = 1, 
  //  timestep = 20 outStep = 2,  and so on.

  virtual double* GetPointData(int outVar, int outStep) = 0;
  virtual double* GetNodalData(int outVar, int outStep) = 0;

  //Description:
  //  In order to access the AcuSolve Database we need an opaque handle
  //  of type AdbHd.  AdbHd which is a member of this class is 
  //  appropriately initialized based on the logfile name.
  //  Let us say the log file name is "/home/username/channel.2.Log",
  //            WorkDir         = "/home/username/"
  //            ProblemDir      = "ACUSIM.DIR" (hardcoded)
  //            ProblemName     = "channel" 
  //            Runid           = "2" 
  //            The ProblemDir is always ACUSIM.DIR unless explicitly changed by
  //            the user after the analysis is done. 

  virtual void   GetAdbHandle(   char*   LogFileName ) = 0;

  //Description:
  //  VtkCellType mapping with AcuSolveElementType
  //    In the shared library shipped by ACUSIM, these are actually
  //  implemented.

  virtual int GetVtkCellType(int AcuSolveElementType) = 0;
                
  //Description:
  //    All the functions below access AcuSolve Database.  They access a few  
  //    C libraries and get the necessary data.

  virtual void ReadData(char* LogFileName) = 0;
  virtual void ReadGeometry() = 0;
  virtual void ReadPoints(int stepId) = 0;
  virtual void ReadCells() = 0;
  virtual void ReadNodalData() = 0;
  virtual bool GetAleFlag() = 0;
  virtual vtkIdType GetNumberOfTimeSteps() = 0;


protected:
  vtkAcuSolveReaderBase(); 
        
  AdbHd         adbHd;          // Adb handle

  vtkIdType     NumPoints;      // Points or Nodes
  vtkPoints*    Crds;           // Point Data/Node Data/Coordinates 
  vtkIdType     NumCells;       // Number Of Cells/Elements             
  vtkIdType     NumCellSets;    // Number Of Cell/Element Sets  
  vtkIdType*    NumCellsInSet;  // Number Of Cell/Elements per Set      
  vtkCellArray**        Cells;          // All The Cell/Element Data            
  vtkIdType*    CellType;       // Cell/Element Types In each Set
  char**                CellSetName;    // Cell/Element Types In each Set

  vtkIdType     NumberOfNodalData;// Number of Point/Nodal Data
  vtkIdType     NumberOfTimeSteps;// Number of Time Steps Available
  vtkIdType*    NodalDataDimension;// Dimension of The Point/Nodal Data
  char**                NodalDataName;  // Name of the Point/Nodal Data
  double**      NodalDataValues;// pressure, velocity etc. values
  bool          AleFlag;        // Is Arb-Lagrangian-Eulerian ON ?
  int           AleId;          // ALE ID in the output variables.

private:
  vtkAcuSolveReaderBase(const vtkAcuSolveReaderBase&); // Not Implemented
  void operator=(const vtkAcuSolveReaderBase&); // Not Implemented
};

// Function pointer typedefs helping the shared library implementation.

typedef vtkAcuSolveReaderBase* CreateFptr();
typedef void DestroyFptr(vtkAcuSolveReaderBase*);

#endif
