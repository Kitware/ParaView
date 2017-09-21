/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMaterialDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMMaterialDomain
 * @brief   ...
 *
 * ...
 */

#ifndef vtkSMMaterialDomain_h
#define vtkSMMaterialDomain_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMStringListDomain.h"

class vtkSMMaterialObserver;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMMaterialDomain : public vtkSMStringListDomain
{
public:
  static vtkSMMaterialDomain* New();
  vtkTypeMacro(vtkSMMaterialDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Overridden to get list of materials from materiallibrary singleton.
   */
  virtual void Update(vtkSMProperty*) VTK_OVERRIDE;

protected:
  vtkSMMaterialDomain();
  ~vtkSMMaterialDomain();

  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) VTK_OVERRIDE;

  void CallMeSometime();
  friend class vtkSMMaterialObserver;
  vtkSMMaterialObserver* Observer;

private:
  vtkSMMaterialDomain(const vtkSMMaterialDomain&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMMaterialDomain&) VTK_DELETE_FUNCTION;
};

#endif
