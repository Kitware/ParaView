/*=========================================================================

  Program:   ParaView
  Module:    vtkSMGL2PSExporterProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMGL2PSExporterProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVGL2PSExporter.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMContextViewProxy.h"

vtkStandardNewMacro(vtkSMGL2PSExporterProxy)

//----------------------------------------------------------------------------
vtkSMGL2PSExporterProxy::vtkSMGL2PSExporterProxy()
{
}

//----------------------------------------------------------------------------
vtkSMGL2PSExporterProxy::~vtkSMGL2PSExporterProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMGL2PSExporterProxy::CanExport(vtkSMProxy* proxy)
{
  return proxy && ( proxy->IsA("vtkSMRenderViewProxy") ||
                    proxy->IsA("vtkSMContextViewProxy") );
}

//----------------------------------------------------------------------------
void vtkSMGL2PSExporterProxy::Write()
{
  this->CreateVTKObjects();

  vtkPVGL2PSExporter* exporter = vtkPVGL2PSExporter::SafeDownCast(
        this->GetClientSideObject());

  vtkRenderWindow *renWin = NULL;
  // Forces vtkGL2PSExporter::NO_SORT for charts, as they use a painter.
  bool forceNoSort = false;
  if (vtkSMRenderViewProxy* rv = vtkSMRenderViewProxy::SafeDownCast(this->View))
    {
    renWin = rv->GetRenderWindow();
    }
  else if (vtkSMContextViewProxy *cv =
           vtkSMContextViewProxy::SafeDownCast(this->View))
    {
    renWin = cv->GetRenderWindow();
    forceNoSort = true;
    }

  if (exporter && renWin)
    {
    int oldSort = -1;
    if (forceNoSort)
      {
      oldSort = exporter->GetSort();
      exporter->SetSortToOff();
      }
    exporter->SetRenderWindow(renWin);
    exporter->Write();
    exporter->SetRenderWindow(0);
    if (forceNoSort)
      {
      exporter->SetSort(oldSort);
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMGL2PSExporterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
