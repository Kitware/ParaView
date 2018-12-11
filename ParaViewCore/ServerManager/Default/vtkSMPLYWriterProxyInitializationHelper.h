/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPLYWriterProxyInitializationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMPLYWriterProxyInitializationHelper
 * @brief   initialization helper for
 * (writers, PPLYWriter) proxy.
 *
 * vtkSMPLYWriterProxyInitializationHelper is an initialization helper for
 * the PPLYWriter proxy that sets up the "ColorArrayName" and "LookupTable"
 * using the coloring state in the active view.
*/

#ifndef vtkSMPLYWriterProxyInitializationHelper_h
#define vtkSMPLYWriterProxyInitializationHelper_h

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMProxyInitializationHelper.h"

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMPLYWriterProxyInitializationHelper
  : public vtkSMProxyInitializationHelper
{
public:
  static vtkSMPLYWriterProxyInitializationHelper* New();
  vtkTypeMacro(vtkSMPLYWriterProxyInitializationHelper, vtkSMProxyInitializationHelper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void PostInitializeProxy(vtkSMProxy*, vtkPVXMLElement*, vtkMTimeType) override;

protected:
  vtkSMPLYWriterProxyInitializationHelper();
  ~vtkSMPLYWriterProxyInitializationHelper() override;

private:
  vtkSMPLYWriterProxyInitializationHelper(const vtkSMPLYWriterProxyInitializationHelper&) = delete;
  void operator=(const vtkSMPLYWriterProxyInitializationHelper&) = delete;
};

#endif
