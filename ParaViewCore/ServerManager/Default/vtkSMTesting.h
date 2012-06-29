/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTesting.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMTesting - vtkTesting adaptor for Server Manager.
// .DESCRIPTION
// This provides helper methods to use render module proxy for testing.
// This is also required for python testing, since when SM is python wrapped,
// VTK need not by python wrapped, hence we cannot use vtkTesting in python 
// testing.

#ifndef __vtkSMTesting_h
#define __vtkSMTesting_h

#include "vtkSMObject.h"

class vtkTesting;
class vtkSMRenderViewProxy;

class VTK_EXPORT vtkSMTesting : public vtkSMObject
{
public:
  static vtkSMTesting* New();
  vtkTypeMacro(vtkSMTesting, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get the render module proxy.
  void SetRenderViewProxy(vtkSMRenderViewProxy* );
  vtkGetObjectMacro(RenderViewProxy, vtkSMRenderViewProxy);

  // Description:
  // Add argument
  virtual void AddArgument(const char* arg);

  // Description:
  // Perform the actual test.
  virtual int RegressionTest(float thresh);

protected:
  vtkSMTesting();
  ~vtkSMTesting();

  vtkTesting* Testing;
  vtkSMRenderViewProxy* RenderViewProxy;

private:
  vtkSMTesting(const vtkSMTesting&); // Not implemented.
  void operator=(const vtkSMTesting&); // Not implemented.
};


#endif

