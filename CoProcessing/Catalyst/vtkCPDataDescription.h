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
#include "vtkPVCatalystModule.h" // needed for exports

class vtkFieldData;
class vtkCPInputDataDescription;

/// @ingroup CoProcessing
/// This class provides the description of the data for the coprocessor
/// pipelines.
class VTKPVCATALYST_EXPORT vtkCPDataDescription : public vtkObject
{
public:
  static vtkCPDataDescription* New();
  vtkTypeMacro(vtkCPDataDescription, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Set the time step and current simulation time.
  void SetTimeData(double time, vtkIdType timeStep);

  /// Get the current time step that should be set in the adaptor.
  vtkGetMacro(TimeStep, vtkIdType);

  /// Get the current time that should be set in the adaptor.
  vtkGetMacro(Time, double);

  /// Specify whether or not to force output of all coprocessing pipelines.
  /// This is meant to be set in the adaptor and used in the coprocessing
  /// pipeline.  Default is false.  If this is true then
  /// GetIsGridNecessary() and GetIfAnyGridIsNecessary() will return true.
  vtkSetMacro(ForceOutput, bool);

  /// Specify whether or not to force output of all coprocessing pipelines.
  /// This is meant to be set in the adaptor and used in the coprocessing
  /// pipeline.  Default is false.  If this is true then
  /// GetIsGridNecessary() and GetIfAnyGridIsNecessary() will return true.
  vtkBooleanMacro(ForceOutput, bool);

  /// Return whether or not output is forced for all coprocessing pipelines.
  vtkGetMacro(ForceOutput, bool);

  /// Add names for grids produced by the simulation. This allocates a new
  /// vtkCPInputDataDescription for that grid, if a grid by that name does not
  /// already exist.
  void AddInput(const char* gridName);

  /// Returns the number of input descriptions.
  unsigned int GetNumberOfInputDescriptions();

  /// Reset the names of the fields that are needed, the required meshes,
  /// etc. that are stored in the vtkCPInputDescriptions.
  void ResetInputDescriptions();

  /// Reset the names of the fields that are needed, the required meshes,
  /// etc. that are stored in the vtkCPInputDescriptions as well as
  /// the time information and output forcing.  Automatically
  /// called after vtkCPProcessor::CoProcess() is called.
  void ResetAll();

  /// Provides access to a grid description using the index.
  vtkCPInputDataDescription* GetInputDescription(unsigned int);

  /// Provides the name for the input description at the given index.
  const char* GetInputDescriptionName(unsigned int);

  /// Provides access to a grid description using the grid name.
  vtkCPInputDataDescription* GetInputDescriptionByName(const char*);

  /// Returns true if the grid is necessary, given the grid's name.
  bool GetIfGridIsNecessary(const char*);

  /// Returns true if any of the grids is necessary.
  bool GetIfAnyGridNecessary();

  /// Set user defined information that can be passed from the
  /// adaptor to the coprocessing pipelines.
  void SetUserData(vtkFieldData* UserData);

  /// Set user defined information that can be passed from the
  /// adaptor to the coprocessing pipelines.
  vtkGetObjectMacro(UserData, vtkFieldData);

  /// Copy of dataDescription. Does a deep copy of the data members
  /// but a shallow copy of the vtkDataObjects.
  void Copy(vtkCPDataDescription*);

protected:
  vtkCPDataDescription();
  virtual ~vtkCPDataDescription();

private:
  vtkCPDataDescription(const vtkCPDataDescription&) = delete;
  void operator=(const vtkCPDataDescription&) = delete;

  /// The current simulation time.  This should be set in the adaptor.
  double Time;

  /// The current simulation time step.  This should be set in the adaptor.
  vtkIdType TimeStep;

  /// Specify whether or not a value for Time and TimeStep have been set.
  bool IsTimeDataSet;

  /// Flag to specify whether or not to force output of all coprocessing pipelines.
  /// This is meant to be set in the adaptor and used in the coprocessing
  /// pipeline.  Default is false.  If this is true then
  /// GetIsGridNecessary() and GetIfAnyGridIsNecessary() will return true.
  bool ForceOutput;

  /// UserData allows the user to pass information from the adaptor to
  /// the coprocessing pipelines.  We currently use vtkFieldData since
  /// it can store a wide variety of data types which are all python wrapped.
  vtkFieldData* UserData;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
