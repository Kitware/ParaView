/*=========================================================================

  Program:   ParaView
  Module:    vtkPVUpdateSuppressor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVUpdateSuppressor - prevents propagation of update
// .SECTION Description 
// vtkPVUpdateSuppressor is simple filter that work with
// vtkUpdateSuppressorPipeline to block pipeline updates. One should call
// ForceUpdate() to update the input, if needed, explicitly.
// Note that ForceUpdate() may not result in input updating at all if it has
// been already updated by some other means.

#ifndef __vtkPVUpdateSuppressor_h
#define __vtkPVUpdateSuppressor_h

#include "vtkDataObjectAlgorithm.h"

class VTK_EXPORT vtkPVUpdateSuppressor : public vtkDataObjectAlgorithm
{
public:
  vtkTypeMacro(vtkPVUpdateSuppressor,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with user-specified implicit function.
  static vtkPVUpdateSuppressor *New();

  // Description:
  // Force update on the input.
  virtual void ForceUpdate();

  // Description:
  // Get/Set if the update suppressor is enabled. If the update suppressor 
  // is not enabled, it won't supress any updates. Enabled by default.
  void SetEnabled(bool);
  vtkGetMacro(Enabled, bool);

//BTX
  // Description:
  // Provides access to the timestamp when the most recent ForceUpdate() was
  // called.
  vtkGetMacro(ForcedUpdateTimeStamp, vtkTimeStamp);

protected:
  vtkPVUpdateSuppressor();
  ~vtkPVUpdateSuppressor();

  int RequestDataObject(vtkInformation* request, vtkInformationVector **inputVector,
    vtkInformationVector *outputVector);
  int RequestData(vtkInformation* request, vtkInformationVector **inputVector,
    vtkInformationVector *outputVector);

  vtkTimeStamp ForcedUpdateTimeStamp;

  bool Enabled;

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();
private:
  vtkPVUpdateSuppressor(const vtkPVUpdateSuppressor&);  // Not implemented.
  void operator=(const vtkPVUpdateSuppressor&);  // Not implemented.
//ETX
};

#endif
