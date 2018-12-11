/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSpreadSheetRepresentationInitializationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMSpreadSheetRepresentationInitializationHelper
 * @brief initialization helper for SpreadSheetRepresentation proxy.
 *
 * vtkSMSpreadSheetRepresentationInitializationHelper helps initialization of
 * properties on the SpreadSheetRepresentation proxy.
 *
 * Currently, this handles choosing a good default value for
 * "CompositeDataSetIndex" property. We have plans of refactoring the
 * spreadsheet representation to support partial arrays when showing multiple
 * blocks in a composite dataset at the same time. Until then, however, if the
 * default is the root index, then for filters like the statics filters, we end
 * up with an empty view. This helper avoids that.
 */

#ifndef vtkSMSpreadSheetRepresentationInitializationHelper_h
#define vtkSMSpreadSheetRepresentationInitializationHelper_h

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMProxyInitializationHelper.h"

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMSpreadSheetRepresentationInitializationHelper
  : public vtkSMProxyInitializationHelper
{
public:
  static vtkSMSpreadSheetRepresentationInitializationHelper* New();
  vtkTypeMacro(vtkSMSpreadSheetRepresentationInitializationHelper, vtkSMProxyInitializationHelper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void PostInitializeProxy(vtkSMProxy*, vtkPVXMLElement*, vtkMTimeType) override;

protected:
  vtkSMSpreadSheetRepresentationInitializationHelper();
  ~vtkSMSpreadSheetRepresentationInitializationHelper();

private:
  vtkSMSpreadSheetRepresentationInitializationHelper(
    const vtkSMSpreadSheetRepresentationInitializationHelper&) = delete;
  void operator=(const vtkSMSpreadSheetRepresentationInitializationHelper&) = delete;
};

#endif
