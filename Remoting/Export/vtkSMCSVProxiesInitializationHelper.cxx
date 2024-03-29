// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMCSVProxiesInitializationHelper.h"

#include "vtkObjectFactory.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

#include <cassert>
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkSMCSVProxiesInitializationHelper);
//----------------------------------------------------------------------------
vtkSMCSVProxiesInitializationHelper::vtkSMCSVProxiesInitializationHelper() = default;

//----------------------------------------------------------------------------
vtkSMCSVProxiesInitializationHelper::~vtkSMCSVProxiesInitializationHelper() = default;

//----------------------------------------------------------------------------
void vtkSMCSVProxiesInitializationHelper::PostInitializeProxy(
  vtkSMProxy* proxy, vtkPVXMLElement*, vtkMTimeType)
{
  std::string fileName = vtkSMPropertyHelper(proxy, "FileName").GetAsString();
  if (vtksys::SystemTools::GetFilenameLastExtension(fileName) == ".tsv")
  {
    if (proxy->IsA("vtkSMWriterProxy"))
    { // exporter
      vtkSMPropertyHelper(proxy, "FieldDelimiter").Set("\t");
    }
    else
    { // reader
      vtkSMPropertyHelper(proxy, "AddTabFieldDelimiter").Set(1);
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMCSVProxiesInitializationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
