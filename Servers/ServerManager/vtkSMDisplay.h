/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDisplay - Superclass for display classes.
// .SECTION Description
// This class defines the API for display objects.  Currently
// we have two toplevel types: Plot display and part display.
// Although I could put the update suppressor and related methods
// in this superclass, I am trying to only define the API here.

#ifndef __vtkSMDisplay_h
#define __vtkSMDisplay_h

#include "vtkSMProxy.h"

class vtkPVRenderModule;
class vtkSMSourceProxy;

class VTK_EXPORT vtkSMDisplay : public vtkSMProxy
{
public:
  vtkTypeRevisionMacro(vtkSMDisplay, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description:
  // This may have to change when 3D widgets need to be added to the interactor.
  virtual void AddToRenderer(vtkPVRenderModule*) = 0;
  virtual void RemoveFromRenderer(vtkPVRenderModule*) = 0;
//ETX

  // Description:
  // Connects the parts data to the mapper and actor.
  virtual void SetInput(vtkSMSourceProxy* input) = 0;

  // Description:
  // Each display implements visibility.
  virtual void SetVisibility(int v) = 0;
  virtual int GetVisibility() = 0;

  // Description:
  // This method updates the piece that has been assigned to this process.
  virtual void Update() = 0;

  // Description:
  // For flip books.
  virtual void CacheUpdate(int idx, int total) = 0;  

  // Description:
  // Calls MarkConsumersAsModified() on all consumers. Sub-classes
  // should add their functionality and call this.
  virtual void MarkConsumersAsModified();

protected:
  vtkSMDisplay();
  ~vtkSMDisplay();
  
  // Description:
  // PVSource calls this when it gets modified.
  virtual void InvalidateGeometry() = 0;

  vtkSMDisplay(const vtkSMDisplay&); // Not implemented
  void operator=(const vtkSMDisplay&); // Not implemented
};

#endif
