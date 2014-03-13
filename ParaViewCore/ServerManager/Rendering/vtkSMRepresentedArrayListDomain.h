/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRepresentedArrayListDomain - extends vtkSMArrayListDomain to add
// support for arrays from represented data information.
// .SECTION Description
// Representations often add new arrays on top of the ones provided by the
// inputs to the representations. In that case, the domains for properties that
// allow users to pick one of those newly added arrays need to show those
// arrays e.g. "ColorArrayName" property of geometry representations. This
// domain extends vtkSMArrayListDomain to add arrays from represented data
// for representations.
#ifndef __vtkSMRepresentedArrayListDomain_h
#define __vtkSMRepresentedArrayListDomain_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMArrayListDomain.h"
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkSMRepresentationProxy;
class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMRepresentedArrayListDomain : public vtkSMArrayListDomain
{
public:
  static vtkSMRepresentedArrayListDomain* New();
  vtkTypeMacro(vtkSMRepresentedArrayListDomain, vtkSMArrayListDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Update the domain.
  virtual void Update(vtkSMProperty*);

//BTX
protected:
  vtkSMRepresentedArrayListDomain();
  ~vtkSMRepresentedArrayListDomain();

  // Description:
  // HACK: Provides a temporary mechanism for subclasses to provide an
  // "additional" vtkPVDataInformation instance to get available arrays list
  // from.
  virtual vtkPVDataInformation* GetExtraDataInformation();

  void SetRepresentationProxy(vtkSMRepresentationProxy*);
  void OnRepresentationDataUpdated();

  vtkWeakPointer<vtkSMRepresentationProxy> RepresentationProxy;
  unsigned long ObserverId;

private:
  vtkSMRepresentedArrayListDomain(const vtkSMRepresentedArrayListDomain&); // Not implemented
  void operator=(const vtkSMRepresentedArrayListDomain&); // Not implemented
//ETX
};

#endif
