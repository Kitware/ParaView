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
#ifndef vtkCPDataDescription_h
#define vtkCPDataDescription_h

#include "vtkObject.h"
#include "CPWin32Header.h" // For windows import/export of shared libraries

class vtkDataObject;
class vtkDataSet;
class vtkFieldData;
class vtkStringArray;
class vtkCPInputDataDescription;

/// @ingroup CoProcessing
/// This class provides the description of the data for the coprocessor
/// pipelines.
class COPROCESSING_EXPORT vtkCPDataDescription : public vtkObject
{
public:
  static vtkCPDataDescription* New();
  vtkTypeMacro(vtkCPDataDescription,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Description:
  /// Set the time step and current simulation time.
  void SetTimeData(double time, vtkIdType timeStep);

  /// Macros for getting the time step and simulation time.
  vtkGetMacro(TimeStep, vtkIdType);
  vtkGetMacro(Time, double);
 
  /// Add names for grids produced by the simulation. This allocates a new
  /// vtkCPInputDataDescription for that grid, if a grid by that name does not
  /// already exist.
  void AddInput(const char* gridName);

  /// Returns the number of input descriptions.
  unsigned int GetNumberOfInputDescriptions();

  /// Reset the names of the fields that are needed, the required meshes etc.
  void Reset();

  /// Provides access to a grid description using the index.
  vtkCPInputDataDescription *GetInputDescription(unsigned int);

  /// Provides access to a grid description using the grid name.
  vtkCPInputDataDescription *GetInputDescriptionByName(const char*);

  /// Returns true if the grid is necessary, given the grid's name.
  bool GetIfGridIsNecessary(const char*);

  /// Returns true if any of the grids is necessary.
  bool GetIfAnyGridNecessary();

//BTX
protected:
  vtkCPDataDescription();
  virtual ~vtkCPDataDescription();

private:
  vtkCPDataDescription(const vtkCPDataDescription&); // Not implemented
  void operator=(const vtkCPDataDescription&); // Not implemented

  /// Information about the current simulation time and whether is has been
  /// set for this object.
  double Time;
  vtkIdType TimeStep;
  bool IsTimeDataSet;

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif
