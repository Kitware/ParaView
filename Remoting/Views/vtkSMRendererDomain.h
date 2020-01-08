/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRendererDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMRendererDomain
 * @brief   Manages the list of available ray traced renderers
 * This domain builds the list of ray traced renderer backends on the
 * View section of the Qt GUI.
 */

#ifndef vtkSMRendererDomain_h
#define vtkSMRendererDomain_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMStringListDomain.h"

class VTKREMOTINGVIEWS_EXPORT vtkSMRendererDomain : public vtkSMStringListDomain
{
public:
  static vtkSMRendererDomain* New();
  vtkTypeMacro(vtkSMRendererDomain, vtkSMStringListDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual void Update(vtkSMProperty*) override;

protected:
  vtkSMRendererDomain(){};
  ~vtkSMRendererDomain() override{};

  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

private:
  vtkSMRendererDomain(const vtkSMRendererDomain&) = delete;
  void operator=(const vtkSMRendererDomain&) = delete;
};

#endif
