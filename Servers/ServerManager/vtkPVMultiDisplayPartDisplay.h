/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMultiDisplayPartDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMultiDisplayPartDisplay - For tiled diplay (what on client).
// .SECTION Description
// This class has an unfortunate name ...  It always collects and displays
// the LOD on node zero (client when we enable client server).

#ifndef __vtkPVMultiDisplayPartDisplay_h
#define __vtkPVMultiDisplayPartDisplay_h

#include "vtkPVCompositePartDisplay.h"


class VTK_EXPORT vtkPVMultiDisplayPartDisplay : public vtkPVCompositePartDisplay
{
public:
  static vtkPVMultiDisplayPartDisplay* New();
  vtkTypeRevisionMacro(vtkPVMultiDisplayPartDisplay, vtkPVCompositePartDisplay);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Override super class so that the LOD is always collected.
  virtual void SetLODCollectionDecision(int val);

  // Description:
  // Update like normal, but make sure the LOD is collected.
  // I encountered a bug. First render was missing the LOD on the client.
  void Update();
  
protected:
  vtkPVMultiDisplayPartDisplay();
  ~vtkPVMultiDisplayPartDisplay();
  
  // Description:
  // This method should be called immediately after the object is constructed.
  // It create VTK objects which have to exeist on all processes.
  virtual void CreateParallelTclObjects(vtkPVProcessModule *pm);

  vtkPVMultiDisplayPartDisplay(const vtkPVMultiDisplayPartDisplay&); // Not implemented
  void operator=(const vtkPVMultiDisplayPartDisplay&); // Not implemented
};

#endif
