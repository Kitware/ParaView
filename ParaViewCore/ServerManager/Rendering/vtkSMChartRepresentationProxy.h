/*=========================================================================

  Program:   ParaView
  Module:    vtkSMChartRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMChartRepresentationProxy
 *
 *
*/

#ifndef vtkSMChartRepresentationProxy_h
#define vtkSMChartRepresentationProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMRepresentationProxy.h"

class vtkChartRepresentation;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMChartRepresentationProxy
  : public vtkSMRepresentationProxy
{
public:
  static vtkSMChartRepresentationProxy* New();
  vtkTypeMacro(vtkSMChartRepresentationProxy, vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Returns client side representation object.
   */
  vtkChartRepresentation* GetRepresentation();

  /**
   * Overridden to handle links with subproxy properties.
   */
  int ReadXMLAttributes(vtkSMSessionProxyManager* pm, vtkPVXMLElement* element) VTK_OVERRIDE;

protected:
  vtkSMChartRepresentationProxy();
  ~vtkSMChartRepresentationProxy();

  /**
   * Overridden to ensure that whenever "Input" property changes, we update the
   * "Input" properties for all internal representations (including setting up
   * of the link to the extract-selection representation).
   */
  virtual void SetPropertyModifiedFlag(const char* name, int flag) VTK_OVERRIDE;

private:
  vtkSMChartRepresentationProxy(const vtkSMChartRepresentationProxy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMChartRepresentationProxy&) VTK_DELETE_FUNCTION;
};

#endif
