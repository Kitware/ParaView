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
/**
 * @class   vtkSMTesting
 * @brief   vtkTesting adaptor for Server Manager.
 * .DESCRIPTION
 * This provides helper methods to use view proxy for testing.
 * This is also required for python testing, since when SM is python wrapped,
 * VTK need not by python wrapped, hence we cannot use vtkTesting in python
 * testing.
*/

#ifndef vtkSMTesting_h
#define vtkSMTesting_h

#include "vtkRemotingMiscModule.h" //needed for exports
#include "vtkSMObject.h"

class vtkSMViewProxy;
class vtkTesting;

class VTKREMOTINGMISC_EXPORT vtkSMTesting : public vtkSMObject
{
public:
  static vtkSMTesting* New();
  vtkTypeMacro(vtkSMTesting, vtkSMObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set/get the render module proxy.
   */
  void SetViewProxy(vtkSMViewProxy* view);

  /**
   * API for backwards compatibility. Simply calls SetViewProxy(..).
   */
  void SetRenderViewProxy(vtkSMViewProxy* proxy) { this->SetViewProxy(proxy); }

  /**
   * Add argument
   */
  virtual void AddArgument(const char* arg);

  /**
   * Perform the actual test.
   */
  virtual int RegressionTest(float thresh);

protected:
  vtkSMTesting();
  ~vtkSMTesting() override;

  vtkSMViewProxy* ViewProxy;
  vtkTesting* Testing;

private:
  vtkSMTesting(const vtkSMTesting&) = delete;
  void operator=(const vtkSMTesting&) = delete;
};
#endif
