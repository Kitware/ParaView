/*=========================================================================

  Program:   ParaView
  Module:    vtkSMConsumerDisplayProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMConsumerDisplayProxy - proxy for displays that take input.
// .SECTION Description
// This is the abstract superclass for display proxies that take an input.
// It mimicks the part of the interface of vtkSMSourceProxy so that this proxy
// (and subclasses) can provide a vtkSMInputProperty.

#ifndef __vtkSMConsumerDisplayProxy_h
#define __vtkSMConsumerDisplayProxy_h

#include "vtkSMDisplayProxy.h"

class vtkSMSourceProxy;
class VTK_EXPORT vtkSMConsumerDisplayProxy : public vtkSMDisplayProxy
{
public:
  vtkTypeRevisionMacro(vtkSMConsumerDisplayProxy, vtkSMDisplayProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Called when setting input using the Input property.
  // Subclasses must override this method to set the input 
  // to the display pipeline.
  // Typically, none of the displays use method/hasMultipleInputs
  // arguements.
  virtual void AddInput(vtkSMSourceProxy* input, const char* method, 
    int hasMultipleInputs) = 0;

  // Description:
  // Chains to superclass and calls InvalidateGeometryInternal(UseCache).
  virtual void MarkModified(vtkSMProxy* modifiedProxy); 

  // Description:
  // Invalidates Geometry. Results in removal of any cached geometry. Also,
  // marks the current geometry as invalid, thus a subsequent call to Update
  // will result in call to ForceUpdate on the UpdateSuppressor(s), if any.
  virtual void InvalidateGeometry();

  // Description:
  // UseCache tells the display to whether to try to use geometry cache
  // (when true) or not (when false) when invalidating geometry. If
  // UseCache is true, cached geometry is not marked as invalid (and
  // is not updated on server).
  static void SetUseCache(int useCache);
  static int GetUseCache();

protected:
  vtkSMConsumerDisplayProxy();
  ~vtkSMConsumerDisplayProxy();

  // Invalidate geometry. If useCache is true, do not invalidate
  // cached geometry
  virtual void InvalidateGeometryInternal(int /*useCache*/) {};

  static int UseCache;
  
private:
  vtkSMConsumerDisplayProxy(const vtkSMConsumerDisplayProxy&); // Not implemented.
  void operator=(const vtkSMConsumerDisplayProxy&); // Not implemented.
};



#endif

