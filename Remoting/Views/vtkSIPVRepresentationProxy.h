/*=========================================================================

  Program:   ParaView
  Module:    vtkSIPVRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSIPVRepresentationProxy
 *
 * vtkSIPVRepresentationProxy is the helper for vtkSMPVRepresentationProxy.
*/

#ifndef vtkSIPVRepresentationProxy_h
#define vtkSIPVRepresentationProxy_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSIProxy.h"

class VTKREMOTINGVIEWS_EXPORT vtkSIPVRepresentationProxy : public vtkSIProxy
{
public:
  static vtkSIPVRepresentationProxy* New();
  vtkTypeMacro(vtkSIPVRepresentationProxy, vtkSIProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void AboutToDelete() override;

protected:
  vtkSIPVRepresentationProxy();
  ~vtkSIPVRepresentationProxy() override;

  /**
   * Parses the XML to create property/subproxy helpers.
   * Overridden to parse all the "RepresentationType" elements.
   */
  bool ReadXMLAttributes(vtkPVXMLElement* element) override;

private:
  vtkSIPVRepresentationProxy(const vtkSIPVRepresentationProxy&) = delete;
  void operator=(const vtkSIPVRepresentationProxy&) = delete;

  void OnVTKObjectModified();

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
