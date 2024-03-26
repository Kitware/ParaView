// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSIImporterProxy.h"
#include "vtkMetaImporter.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSIImporterProxy);

//----------------------------------------------------------------------------
vtkSIImporterProxy::vtkSIImporterProxy() = default;

//----------------------------------------------------------------------------
vtkSIImporterProxy::~vtkSIImporterProxy() = default;

//----------------------------------------------------------------------------
void vtkSIImporterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSIImporterProxy::UpdatePipelineInformation()
{
  auto metaImporter = vtkMetaImporter::SafeDownCast(this->VTKObject);
  if (metaImporter == nullptr)
  {
    return;
  }
  metaImporter->UpdateInformation();
}
