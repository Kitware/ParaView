/*=========================================================================

  Program:   ParaView
  Module:    vtkCPPipeline.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkCPPipeline_h
#define vtkCPPipeline_h

#include "vtkObject.h"
#include "vtkPVCatalystModule.h" // For windows import/export of shared libraries

class vtkCPDataDescription;

/// @ingroup CoProcessing
/// Generic interface for operating on pipelines.  The user can use this
/// if they only have a single pipeline that they want to operate on
/// or they can use this to create a single pipeline and add it to
/// vtkCPProcessor.  Each derived class should set itself up before
/// adding itself to vtkCPProcessor.
class VTKPVCATALYST_EXPORT vtkCPPipeline : public vtkObject
{
public:
  vtkTypeMacro(vtkCPPipeline, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Configuration Step:
  /// The coprocessor first determines if any coprocessing needs to be done
  /// at this TimeStep/Time combination returning 1 if it does and 0
  /// otherwise.  If coprocessing does need to be performed this time step
  /// it fills in the FieldNames array that the coprocessor requires
  /// in order to fulfill all the coprocessing requests for this
  /// TimeStep/Time combination.
  virtual int RequestDataDescription(vtkCPDataDescription* dataDescription) = 0;

  /// Execute the pipeline. Returns 1 for success and 0 for failure.
  virtual int CoProcess(vtkCPDataDescription* dataDescription) = 0;

  /// Finalize the pipeline before deleting it. A default no-op implementation
  /// is given. Returns 1 for success and 0 for failure.
  virtual int Finalize();

protected:
  vtkCPPipeline();
  virtual ~vtkCPPipeline();

private:
  vtkCPPipeline(const vtkCPPipeline&) = delete;
  void operator=(const vtkCPPipeline&) = delete;
};

#endif
