/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSpreadSheetRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMSpreadSheetRepresentationProxy
 *
 * vtkSMSpreadSheetRepresentationProxy is a representation proxy used for
 * spreadsheet view. This class overrides vtkSMRepresentationProxy to ensure
 * that the selection inputs are setup correctly.
*/

#ifndef vtkSMSpreadSheetRepresentationProxy_h
#define vtkSMSpreadSheetRepresentationProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMRepresentationProxy.h"

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMSpreadSheetRepresentationProxy
  : public vtkSMRepresentationProxy
{
public:
  static vtkSMSpreadSheetRepresentationProxy* New();
  vtkTypeMacro(vtkSMSpreadSheetRepresentationProxy, vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSMSpreadSheetRepresentationProxy();
  ~vtkSMSpreadSheetRepresentationProxy() override;

  /**
   * Overridden to ensure that whenever "Input" property changes, we update the
   * "Input" properties for all internal representations (including setting up
   * of the link to the extract-selection representation).
   */
  void SetPropertyModifiedFlag(const char* name, int flag) override;

private:
  vtkSMSpreadSheetRepresentationProxy(const vtkSMSpreadSheetRepresentationProxy&) = delete;
  void operator=(const vtkSMSpreadSheetRepresentationProxy&) = delete;
};

#endif
