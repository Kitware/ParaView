/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCSVProxiesInitializationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMCSVProxiesInitializationHelper
 * @brief   initialization helper for
 * (writers, PCSVReader) proxy.
 *
 * vtkSMCSVProxiesInitializationHelper is an initialization helper for
 * the PCSVReader or PCSVWriter proxy that sets up the delimiter to use based on the
 * file extension. If the file extension is .txt or .tsv then '\t' is set as
 * the delimiter.
*/

#ifndef vtkSMCSVProxiesInitializationHelper_h
#define vtkSMCSVProxiesInitializationHelper_h

#include "vtkPVServerManagerDefaultModule.h" //needed for exports
#include "vtkSMProxyInitializationHelper.h"

class VTKPVSERVERMANAGERDEFAULT_EXPORT vtkSMCSVProxiesInitializationHelper
  : public vtkSMProxyInitializationHelper
{
public:
  static vtkSMCSVProxiesInitializationHelper* New();
  vtkTypeMacro(vtkSMCSVProxiesInitializationHelper, vtkSMProxyInitializationHelper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void PostInitializeProxy(vtkSMProxy*, vtkPVXMLElement*, vtkMTimeType) override;

protected:
  vtkSMCSVProxiesInitializationHelper();
  ~vtkSMCSVProxiesInitializationHelper() override;

private:
  vtkSMCSVProxiesInitializationHelper(const vtkSMCSVProxiesInitializationHelper&) = delete;
  void operator=(const vtkSMCSVProxiesInitializationHelper&) = delete;
};

#endif
