/*=========================================================================

  Program:   ParaView
  Module:    vtkCPDataDescription.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCPDataDescription - Class for describing the data for coprocessing.
// .SECTION Description
// This class provides the description of the data for the coprocessor
// pipelines.

#ifndef vtkCPDataDescription_h
#define vtkCPDataDescription_h

#include "vtkObject.h"

class vtkDataObject;
class vtkDataSet;
class vtkFieldData;
class vtkStringArray;
class vtkCPInputDataDescription;

class VTK_EXPORT vtkCPDataDescription : public vtkObject
{
public:
  static vtkCPDataDescription* New();
  vtkTypeRevisionMacro(vtkCPDataDescription,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the time step and current simulation time.
  void SetTimeData(double time, vtkIdType timeStep);

  // Description:
  // Macros for getting the time step and simulation time.
  vtkGetMacro(TimeStep, vtkIdType);
  vtkGetMacro(Time, double);
 
  // Description:
  // Add names for grids produced by the simulation. This allocates a new
  // vtkCPInputDataDescription for that grid, if a grid by that name does not
  // already exist.
  void AddInput(const char* gridName);

  // Description:
  // Returns the number of input descriptions.
  unsigned int GetNumberOfInputDescriptions();

  // Description:
  // Reset the names of the fields that are needed, the required meshes etc.
  void Reset();

  // Description:
  // Provides access to a grid description using the index.
  vtkCPInputDataDescription *GetInputDescription(unsigned int);

  // Description:
  // Provides access to a grid description using the grid name.
  vtkCPInputDataDescription *GetInputDescriptionByName(const char*);

  // Description:
  // Returns true if the grid is necessary, given the grid's name.
  bool GetIfGridIsNecessary(const char*);

  // Description:
  // Returns true if any of the grids is necessary.
  bool GetIfAnyGridNecessary();

//BTX
protected:
  vtkCPDataDescription();
  virtual ~vtkCPDataDescription();

private:
  vtkCPDataDescription(const vtkCPDataDescription&); // Not implemented
  void operator=(const vtkCPDataDescription&); // Not implemented

  // Description:
  // Information about the current simulation time and whether is has been
  // set for this object.
  double Time;
  vtkIdType TimeStep;
  bool IsTimeDataSet;

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
