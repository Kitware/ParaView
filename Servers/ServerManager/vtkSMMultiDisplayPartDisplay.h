/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiDisplayPartDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMMultiDisplayPartDisplay - For tiled diplay (what on client).
// .SECTION Description
// This class has an unfortunate name ...  It always collects and displays
// the LOD on node zero (client when we enable client server).

#ifndef __vtkSMMultiDisplayPartDisplay_h
#define __vtkSMMultiDisplayPartDisplay_h

#include "vtkSMCompositePartDisplay.h"


class VTK_EXPORT vtkSMMultiDisplayPartDisplay : public vtkSMCompositePartDisplay
{
public:
  static vtkSMMultiDisplayPartDisplay* New();
  vtkTypeRevisionMacro(vtkSMMultiDisplayPartDisplay, vtkSMCompositePartDisplay);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Override super class so that the LOD is always collected.
  virtual void SetLODCollectionDecision(int val);

  // Description:
  // Update like normal, but make sure the LOD is collected.
  // I encountered a bug. First render was missing the LOD on the client.
  void Update();
  
protected:
  vtkSMMultiDisplayPartDisplay();
  ~vtkSMMultiDisplayPartDisplay();
  
   virtual void CreateVTKObjects(int num);
  
  vtkSMMultiDisplayPartDisplay(const vtkSMMultiDisplayPartDisplay&); // Not implemented
  void operator=(const vtkSMMultiDisplayPartDisplay&); // Not implemented
};

#endif
