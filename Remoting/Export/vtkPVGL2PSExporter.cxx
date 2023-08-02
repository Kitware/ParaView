// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVGL2PSExporter.h"

#include "vtkActorCollection.h"
#include "vtkCubeAxesActor.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkProp3DCollection.h"
#include "vtkPropCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"

#include "vtksys/SystemTools.hxx"

#include <cstdio> // for rename

vtkStandardNewMacro(vtkPVGL2PSExporter);

//----------------------------------------------------------------------------
vtkPVGL2PSExporter::vtkPVGL2PSExporter()
  : ExcludeCubeAxesActorsFromRasterization(true)
{
}

//----------------------------------------------------------------------------
vtkPVGL2PSExporter::~vtkPVGL2PSExporter() = default;

//----------------------------------------------------------------------------
void vtkPVGL2PSExporter::WriteData()
{
  // Write the output to a temporary file (vtkGL2PSExporter takes a prefix
  // and sets the extension itself, while the ParaView export mechanism uses the
  // full filename). The full name of the temporary file will be
  // this->FileName + ".pvtmp.[format extension]".
  std::string tmpFilePrefix(this->FileName + ".pvtmp");
  this->SetFilePrefix(tmpFilePrefix.c_str());

  // Setup raster exclusions if needed
  if (this->Write3DPropsAsRasterImage != 0)
  {
    if (this->RasterExclusions == nullptr)
    {
      vtkNew<vtkPropCollection> coll;
      this->SetRasterExclusions(coll.GetPointer());
    }

    vtkRendererCollection* renCol = this->RenderWindow->GetRenderers();
    vtkRenderer* ren;
    for (renCol->InitTraversal(); (ren = renCol->GetNextItem());)
    {
      vtkActorCollection* actorCol = ren->GetActors();
      vtkActor* actor;
      for (actorCol->InitTraversal(); (actor = actorCol->GetNextItem());)
      {
        // Add cubeaxes actors to raster exclusions if requested
        if (this->ExcludeCubeAxesActorsFromRasterization != 0 && actor->IsA("vtkCubeAxesActor") &&
          !this->RasterExclusions->IsItemPresent(actor))
        {
          this->RasterExclusions->AddItem(actor);
        }
      }

      vtkPropCollection* viewProps = ren->GetViewProps();
      vtkProp* prop;
      for (viewProps->InitTraversal(); (prop = viewProps->GetNextProp());)
      {
        // Always exclude instances of PVAxesActor from rasterization
        if (prop->IsA("vtkPVAxesActor") && !this->RasterExclusions->IsItemPresent(prop))
        {
          this->RasterExclusions->AddItem(prop);
        }
      }
    }
  }

  this->Superclass::WriteData();

  // Move to the requested destination
  std::string tmpFileName(tmpFilePrefix);

  switch (this->FileFormat)
  {
    case PS_FILE:
      tmpFileName += ".ps";
      break;
    case EPS_FILE:
      tmpFileName += ".eps";
      break;
    case PDF_FILE:
      tmpFileName += ".pdf";
      break;
    case TEX_FILE:
      tmpFileName += ".tex";
      break;
    case SVG_FILE:
      tmpFileName += ".svg";
      break;
  }

  std::string fromPath(tmpFileName);
  std::string toPath(this->FileName);

  if (this->Compress)
  {
    fromPath += ".gz";
    toPath += ".gz";
  }

  if (vtksys::SystemTools::FileExists(toPath))
  {
    vtksys::SystemTools::RemoveFile(toPath);
  }
  int result = std::rename(fromPath.c_str(), toPath.c_str());

  if (result != 0)
  {
    std::perror("Could not rename exported graphics file");
    vtkErrorMacro("Cannot rename exported graphics file.");
    return;
  }
}

//----------------------------------------------------------------------------
void vtkPVGL2PSExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
