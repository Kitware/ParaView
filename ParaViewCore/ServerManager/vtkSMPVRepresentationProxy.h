/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPVRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPVRepresentationProxy - a composite representation proxy suitable
// for showing data in ParaView.
// .SECTION Description
// vtkSMPVRepresentationProxy combines surface representation and volume
// representation proxies typically used for displaying data.
// This class also takes over the selection obligations for all the internal
// representations, i.e. is disables showing of selection in all the internal
// representations, and manages it. This avoids duplicate execution of extract
// selection filter for each of the internal representations.

#ifndef __vtkSMPVRepresentationProxy_h
#define __vtkSMPVRepresentationProxy_h

#include "vtkSMRepresentationProxy.h"

class VTK_EXPORT vtkSMPVRepresentationProxy : public vtkSMRepresentationProxy
{
public:
  static vtkSMPVRepresentationProxy* New();
  vtkTypeMacro(vtkSMPVRepresentationProxy, vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMPVRepresentationProxy();
  ~vtkSMPVRepresentationProxy();

  // Description:
  // Overridden to ensure that the RepresentationTypesInfo and
  // Representations's domain are up-to-date.
  virtual void CreateVTKObjects();

  // Whenever the "Representation" property is modified, we ensure that the
  // this->InvalidateDataInformation() is called.
  void OnPropertyUpdated(vtkObject*, unsigned long, void* calldata);

  // Description:
  // Overridden to ensure that whenever "Input" property changes, we update the
  // "Input" properties for all internal representations (including setting up
  // of the link to the extract-selection representation).
  virtual void SetPropertyModifiedFlag(const char* name, int flag);

  // Description:
  // Overridden to process "RepresentationType" elements.
  int ReadXMLAttributes(vtkSMProxyManager* pm, vtkPVXMLElement* element);

private:
  vtkSMPVRepresentationProxy(const vtkSMPVRepresentationProxy&); // Not implemented
  void operator=(const vtkSMPVRepresentationProxy&); // Not implemented

  bool InReadXMLAttributes;
  class vtkStringSet;
  vtkStringSet* RepresentationSubProxies;
//ETX
};

#endif

