/*=========================================================================

  Program:   ParaView
  Module:    vtkSMGenericViewDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMGenericViewDisplay - proxy for any entity that must be rendered.
// .SECTION Description
// vtkSMGenericViewDisplay is a sink for display objects. Anything that can
// be rendered has to be a vtkSMGenericViewDisplay, otherwise it can't be added
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

#ifndef __vtkSMGenericViewDisplay_h
#define __vtkSMGenericViewDisplay_h

#include "vtkSMAbstractDisplayProxy.h"

class VTK_EXPORT vtkSMGenericViewDisplay : public vtkSMAbstractDisplayProxy
{
public:
  static vtkSMGenericViewDisplay* New();
  vtkTypeRevisionMacro(vtkSMGenericViewDisplay, vtkSMAbstractDisplayProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMGenericViewDisplay();
  ~vtkSMGenericViewDisplay();

private:
  vtkSMGenericViewDisplay(const vtkSMGenericViewDisplay&); // Not implemented.
  void operator=(const vtkSMGenericViewDisplay&); // Not implemented.
};



#endif


