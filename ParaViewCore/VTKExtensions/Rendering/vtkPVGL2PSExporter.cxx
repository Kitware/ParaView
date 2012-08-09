/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGL2PSExporter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGL2PSExporter.h"

#include "vtkActorCollection.h"
#include "vtkCubeAxesActor.h"
#include "vtkNew.h"
#include "vtkProp3DCollection.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"

#include <cstdio> // for rename

vtkStandardNewMacro(vtkPVGL2PSExporter)

//----------------------------------------------------------------------------
vtkPVGL2PSExporter::vtkPVGL2PSExporter()
{
}

//----------------------------------------------------------------------------
vtkPVGL2PSExporter::~vtkPVGL2PSExporter()
{
}

//----------------------------------------------------------------------------
void vtkPVGL2PSExporter::WriteData()
{
  // Write the output to a temporary file (vtkGL2PSExporter takes a prefix
  // and sets the extension itself, while the ParaView export mechanism uses the
  // full filename). The full name of the temporary file will be
  // this->FileName + ".pvtmp.[format extension]".
  vtkStdString tmpFilePrefix (this->FileName + ".pvtmp");
  this->SetFilePrefix(tmpFilePrefix.c_str());

  // Add cubeaxes actors to raster exclusions if requested
  if (this->Write3DPropsAsRasterImage != 0 &&
      this->ExcludeCubeAxesActorsFromRasterization != 0)
    {
    if (this->RasterExclusions == NULL)
      {
      vtkNew<vtkProp3DCollection> coll;
      this->SetRasterExclusions(coll.GetPointer());
      }

    vtkRendererCollection *renCol = this->RenderWindow->GetRenderers();
    vtkRenderer *ren;
    for (renCol->InitTraversal(); ren = renCol->GetNextItem();)
      {
      vtkActorCollection *actorCol = ren->GetActors();
      vtkActor *actor;
      for (actorCol->InitTraversal(); actor = actorCol->GetNextItem();)
        {
        if (actor->IsA("vtkCubeAxesActor"))
          {
          if (!this->RasterExclusions->IsItemPresent(actor))
            {
            this->RasterExclusions->AddItem(actor);
            }
          }
        }
      }
    }

  this->Superclass::WriteData();

  // Move to the requested destination
  vtkStdString tmpFileName(tmpFilePrefix);

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

  int result = std::rename(tmpFileName.c_str(), this->FileName.c_str());

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
