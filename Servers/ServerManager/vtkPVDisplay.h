/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDisplay - Superclass for display classes.
// .SECTION Description
// This class defines the API for display objects.  Currently
// we have two toplevel types: Plot display and part display.
// Although I could put the update suppressor and related methods
// in this superclass, I am trying to only define the API here.

#ifndef __vtkPVDisplay_h
#define __vtkPVDisplay_h

#include "vtkObject.h"

class vtkSMPart;

class VTK_EXPORT vtkPVDisplay : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkPVDisplay, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Connects the parts data to the mapper and actor.
  virtual void SetInput(vtkSMPart* input) = 0;

  // Description:
  // This method updates the piece that has been assigned to this process.
  virtual void Update() = 0;

  // Description:
  // For flip books.
  virtual void CacheUpdate(int idx, int total) = 0;  

  // Description:
  // PVSource calls this when it gets modified.
  virtual void InvalidateGeometry() = 0;

protected:
  vtkPVDisplay();
  ~vtkPVDisplay();
  
  vtkPVDisplay(const vtkPVDisplay&); // Not implemented
  void operator=(const vtkPVDisplay&); // Not implemented
};

#endif
