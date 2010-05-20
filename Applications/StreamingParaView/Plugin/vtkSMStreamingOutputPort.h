/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStreamingOutputPort.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStreamingOutputPort - an output port proxy with streaming capabilities.
// .SECTION Description
// This object is meant to replace vtkSMOutputPort objects.  This object has
// an associated object factory, vtkStreamingFactory.

#ifndef __vtkSMStreamingOutputPort_h
#define __vtkSMStreamingOutputPort_h

#include "vtkSMOutputPort.h"

class vtkPVDataInformation;

class VTK_EXPORT vtkSMStreamingOutputPort : public vtkSMOutputPort
{
public:
  static vtkSMStreamingOutputPort* New();
  vtkTypeMacro(vtkSMStreamingOutputPort, vtkSMOutputPort);
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
  vtkSMStreamingOutputPort();
  ~vtkSMStreamingOutputPort();

  // Description:
  // An internal update pipeline method that subclasses may override.
  virtual void UpdatePipelineInternal(double time, bool doTime);

private:
  vtkSMStreamingOutputPort(const vtkSMStreamingOutputPort&); // Not implemented
  void operator=(const vtkSMStreamingOutputPort&); // Not implemented
};

#endif

