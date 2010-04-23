/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiDisplayProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMMultiDisplayProxy - For tiled diplay (what on client).
// .SECTION Description
// This class has an unfortunate name ...  It always collects and displays
// the LOD on node zero (client when we enable client server).
// TODO: I wonder if we can get rid of this class altogether!
// It doesn't really do much.


#ifndef __vtkSMMultiDisplayProxy_h
#define __vtkSMMultiDisplayProxy_h

#include "vtkSMCompositeDisplayProxy.h"

class VTK_EXPORT vtkSMMultiDisplayProxy : public vtkSMCompositeDisplayProxy
{
public:
  static vtkSMMultiDisplayProxy* New();
  vtkTypeMacro(vtkSMMultiDisplayProxy, vtkSMCompositeDisplayProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overridden to set LOD collection decision to always Collect.
  virtual void SetLODCollectionDecision(int);

  // Description:
  // Update like normal, but make sure the LOD is collected.
  // I encountered a bug. First render was missing the LOD on the client.
  virtual void Update(vtkSMAbstractViewModuleProxy*);
  virtual void Update() { this->Superclass::Update(); }

protected:
  vtkSMMultiDisplayProxy();
  ~vtkSMMultiDisplayProxy();

   virtual void CreateVTKObjects();

private:
  vtkSMMultiDisplayProxy(const vtkSMMultiDisplayProxy&); // Not implemented.
  void operator=(const vtkSMMultiDisplayProxy&); // Not implemented.
};


#endif
