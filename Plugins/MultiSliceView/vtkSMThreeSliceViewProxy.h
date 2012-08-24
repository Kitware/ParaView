/*=========================================================================

  Program:   ParaView
  Module:    vtkSMThreeSliceViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMThreeSliceViewProxy
// .SECTION Description
// Custom RenderViewProxy to override CreateDefaultRepresentation method
// so only the Multi-Slice representation will be available to the user
// as well as providing access to the 4 internal renderview proxy

#ifndef __vtkSMThreeSliceViewProxy_h
#define __vtkSMThreeSliceViewProxy_h

#include "vtkSMViewProxy.h"

class vtkSMRenderViewProxy;
class vtkPVThreeSliceView;

class VTK_EXPORT vtkSMThreeSliceViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMThreeSliceViewProxy* New();
  vtkTypeMacro(vtkSMThreeSliceViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Helper methods used to easily access the different view proxies
  vtkSMRenderViewProxy* GetTopLeftViewProxy();

  // Description
  // Helper methods used to easily access the different view proxies
  vtkSMRenderViewProxy* GetTopRightViewProxy();

  // Description
  // Helper methods used to easily access the different view proxies
  vtkSMRenderViewProxy* GetBottomLeftViewProxy();

  // Description
  // Helper methods used to easily access the different view proxies
  vtkSMRenderViewProxy* GetBottomRightViewProxy();

  // Description:
  // Create a default representation for the given source proxy.
  // Returns a new proxy.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(vtkSMProxy*, int);

  // Description:
  // Similar to IsSelectionAvailable(), however, on failure returns the
  // error message otherwise 0.
  virtual const char* IsSelectVisiblePointsAvailable();

  // Description:
  // Dirty means this algorithm will execute during next update.
  // This all marks all consumers as dirty.
  virtual void MarkDirty(vtkSMProxy* modifiedProxy);

//BTX
protected:
  vtkSMThreeSliceViewProxy();
  ~vtkSMThreeSliceViewProxy();

  void InvokeConfigureEvent();

  vtkPVThreeSliceView* GetInternalClientSideView();
  bool NeedToBeInitialized;

  // Description:
  // Override CreateVTKObjects to automatically bind view properties across them
  virtual void CreateVTKObjects();

private:
  vtkSMThreeSliceViewProxy(const vtkSMThreeSliceViewProxy&); // Not implemented
  void operator=(const vtkSMThreeSliceViewProxy&); // Not implemented
//ETX
};

#endif
