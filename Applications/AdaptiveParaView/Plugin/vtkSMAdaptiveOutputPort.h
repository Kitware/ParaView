/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAdaptiveOutputPort.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMAdaptiveOutputPort - an output port proxy with streaming capabilities.
// .SECTION Description
// This object is meant to replace vtkSMOutputPort objects.  This object has
// an associated object factory, vtkAdaptiveFactory.

#ifndef __vtkSMAdaptiveOutputPort_h
#define __vtkSMAdaptiveOutputPort_h

#include "vtkSMOutputPort.h"

class vtkPVDataInformation;

class VTK_EXPORT vtkSMAdaptiveOutputPort : public vtkSMOutputPort
{
public:
  static vtkSMAdaptiveOutputPort* New();
  vtkTypeRevisionMacro(vtkSMAdaptiveOutputPort, vtkSMOutputPort);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns data information. If data information is marked
  // invalid, calls GatherDataInformation.
  // If data information is gathered then this fires the 
  // vtkCommand::UpdateInformationEvent event.
  virtual vtkPVDataInformation* GetDataInformation();

  // Description:
  // Mark data information as invalid.
  virtual void InvalidateDataInformation();

  // Description:
  // Get information about dataset from server.
  // Fires the vtkCommand::UpdateInformationEvent event.
  virtual void GatherDataInformation(int doUpdate=1);

protected:
  vtkSMAdaptiveOutputPort();
  ~vtkSMAdaptiveOutputPort();

  // Description:
  // An internal update pipeline method that subclasses may override.
  virtual void UpdatePipelineInternal(double time, bool doTime);

private:
  vtkSMAdaptiveOutputPort(const vtkSMAdaptiveOutputPort&); // Not implemented
  void operator=(const vtkSMAdaptiveOutputPort&); // Not implemented
};

#endif

