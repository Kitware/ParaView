/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExtractSelectionProxyInitializationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSMExtractSelectionProxyInitializationHelper
 * @brief Initialization helper for the ExtractSelection filter proxy.
 *
 * vtkSMExtractSelectionProxyInitializationHelper initializes the selection
 * input from the selection set on the input source, if it exists.
 */

#ifndef vtkSMExtractSelectionProxyInitializationHelper_h
#define vtkSMExtractSelectionProxyInitializationHelper_h

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMProxyInitializationHelper.h"

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMExtractSelectionProxyInitializationHelper
  : public vtkSMProxyInitializationHelper
{
public:
  static vtkSMExtractSelectionProxyInitializationHelper* New();
  vtkTypeMacro(vtkSMExtractSelectionProxyInitializationHelper, vtkSMProxyInitializationHelper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void PostInitializeProxy(vtkSMProxy*, vtkPVXMLElement*, vtkMTimeType) override;

protected:
  vtkSMExtractSelectionProxyInitializationHelper();
  ~vtkSMExtractSelectionProxyInitializationHelper() override;

private:
  vtkSMExtractSelectionProxyInitializationHelper(
    const vtkSMExtractSelectionProxyInitializationHelper&) = delete;
  void operator=(const vtkSMExtractSelectionProxyInitializationHelper&) = delete;
};

#endif // vtkSMExtractSelectionProxyInitializationHelper
