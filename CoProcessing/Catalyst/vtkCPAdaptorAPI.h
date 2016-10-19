/*=========================================================================

  Program:   ParaView
  Module:    vtkCPAdaptorAPI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkCPAdaptorAPI_h
#define vtkCPAdaptorAPI_h

#include "vtkObject.h"
#include "vtkPVCatalystModule.h" // For windows import/export of shared libraries

class vtkCPDataDescription;
class vtkCPProcessor;
class vtkDataSet;

/// vtkCPAdaptorAPI provides the implementation for API exposed to typical
/// adaptor, such as C, Fortran.
class VTKPVCATALYST_EXPORT vtkCPAdaptorAPI : public vtkObject
{
public:
  vtkTypeMacro(vtkCPAdaptorAPI, vtkObject);

  /// call at the start of the simulation
  static void CoProcessorInitialize();

  /// call at the end of the simulation
  static void CoProcessorFinalize();

  /// this is the function that determines whether or not there
  /// is anything to coprocess this time step
  static void RequestDataDescription(int* timeStep, double* time, int* coprocessThisTimeStep);

  /// this function sets needgrid to 1 if it does not have a copy of the grid
  /// it sets needgrid to 0 if it does have a copy of the grid but does not
  /// check if the grid is modified or needs to be updated
  static void NeedToCreateGrid(int* needGrid);

  /// do the actual coprocessing.  it is assumed that the vtkCPDataDescription
  /// has been filled in elsewhere.
  static void CoProcess();

  /// provides access to the vtkCPDataDescription instance.
  static vtkCPDataDescription* GetCoProcessorData() { return vtkCPAdaptorAPI::CoProcessorData; }

  /// provides access to the vtkCPProcessor instance.
  static vtkCPProcessor* GetCoProcessor() { return vtkCPAdaptorAPI::CoProcessor; }

protected:
  static vtkCPDataDescription* CoProcessorData;
  static vtkCPProcessor* CoProcessor;

  // IsTimeDataSet is meant to be used to make sure that
  // needtocoprocessthistimestep() is called before
  // calling any of the other coprocessing functions.
  // It is reset to falase after calling coprocess as well
  // as if coprocessing is not needed for this time/time step
  static bool IsTimeDataSet;
};
#endif
// VTK-HeaderTest-Exclude: vtkCPAdaptorAPI.h
