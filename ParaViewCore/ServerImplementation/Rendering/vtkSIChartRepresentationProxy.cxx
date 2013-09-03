/*=========================================================================

  Program:   ParaView
  Module:    vtkSIChartRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIChartRepresentationProxy.h"

#include "vtkChartRepresentation.h"
#include "vtkChartSelectionRepresentation.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSIChartRepresentationProxy);
//----------------------------------------------------------------------------
vtkSIChartRepresentationProxy::vtkSIChartRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSIChartRepresentationProxy::~vtkSIChartRepresentationProxy()
{
}

//----------------------------------------------------------------------------
void vtkSIChartRepresentationProxy::OnCreateVTKObjects()
{
  vtkSIProxy* selectionProxy = this->GetSubSIProxy("SelectionRepresentation");
  if (selectionProxy)
    {
    vtkChartRepresentation* repr = vtkChartRepresentation::SafeDownCast(
      this->GetVTKObject());
    vtkChartSelectionRepresentation* selRepr =
      vtkChartSelectionRepresentation::SafeDownCast(selectionProxy->GetVTKObject());
    repr->SetSelectionRepresentation(selRepr);
    }
}

//----------------------------------------------------------------------------
void vtkSIChartRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
