/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAbstractDisplayProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMAbstractDisplayProxy - proxy for any entity that must be rendered.
// .SECTION Description
// vtkSMAbstractDisplayProxy is a sink for display objects. Anything that can
// be rendered has to be a vtkSMAbstractDisplayProxy, otherwise it can't be added
// be added to the vtkSMRenderModule, and hence cannot be rendered.
// This can have inputs (but not required, for displays such as 3Dwidgets/ Scalarbar).
// This is an abstract class, merely defining the interface.
//  This class (or subclasses) has a bunch of 
// "convenience methods" (method names appended with CM). These methods
// do the equivalent of getting the property by the name and
// setting/getting its value. They are there to simplify using the property
// interface for display objects. When adding a method to the proxies
// that merely sets some property on the proxy, make sure to append the method
// name with "CM" - implying it's a convenience method. That way, one knows
// its purpose and will not be confused with a update-self property method.

#ifndef __vtkSMAbstractDisplayProxy_h
#define __vtkSMAbstractDisplayProxy_h

#include "vtkSMProxy.h"
class vtkPVGeometryInformation;

class VTK_EXPORT vtkSMAbstractDisplayProxy : public vtkSMProxy
{
public:
  vtkTypeRevisionMacro(vtkSMAbstractDisplayProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get information about the geometry.
  // Some displays (like Scalar bar, 3DWidgets), may return NULL.
  virtual vtkPVGeometryInformation* GetGeometryInformation() { return NULL; }
  
  // Description:
  // Called to update the Display. Default implementation does nothing.
  virtual void Update() { }

  // Description:
  // When doing an ordered composite, some displays will need to run extra
  // filters that redistribute or clip their data before a render occurs.
  // This method makes sure that the distributed data is up to date.  The
  // default implementation just calls Update().
  virtual void UpdateDistributedGeometry() { this->Update(); }

  // Description:
  // This method returns if the Update() or UpdateDistributedGeometry()
  // calls will actually lead to an Update. This is used by the render module
  // to decide if it can expect any pipeline updates.
  virtual int UpdateRequired() { return false; }
  
  // Description:
  // Save the display in batch script. This will eventually get 
  // removed as we will generate batch script from ServerManager
  // state. However, until then.
  virtual void SaveInBatchScript(ofstream* file);
  
  // Description:
  // Convenience method to get/set Visibility property.
  void SetVisibilityCM(int v);
  int GetVisibilityCM(); 

protected:
  vtkSMAbstractDisplayProxy();
  ~vtkSMAbstractDisplayProxy();

private:
  vtkSMAbstractDisplayProxy(const vtkSMAbstractDisplayProxy&); // Not implemented.
  void operator=(const vtkSMAbstractDisplayProxy&); // Not implemented.
};



#endif


