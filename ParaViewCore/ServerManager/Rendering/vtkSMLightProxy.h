/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLightProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMLightProxy
 * @brief   a configurable light proxy.
 *
 * vtkSMLightProxy is a configurable light. One or more can exist in a view.
*/

#ifndef vtkSMLightProxy_h
#define vtkSMLightProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMProxy.h"

class vtkSMLightObserver;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMLightProxy : public vtkSMProxy
{
public:
  static vtkSMLightProxy* New();
  vtkTypeMacro(vtkSMLightProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMLightProxy();
  ~vtkSMLightProxy() override;

  void CreateVTKObjects() override;

  void PropertyChanged();
  friend class vtkSMLightObserver;
  vtkSMLightObserver* Observer;

private:
  vtkSMLightProxy(const vtkSMLightProxy&) = delete;
  void operator=(const vtkSMLightProxy&) = delete;
};

#endif
