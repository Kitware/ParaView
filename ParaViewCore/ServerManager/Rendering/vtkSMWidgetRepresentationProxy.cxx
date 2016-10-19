/*=========================================================================

  Program:   ParaView
  Module:    vtkSMWidgetRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMWidgetRepresentationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkWidgetRepresentation.h"

vtkStandardNewMacro(vtkSMWidgetRepresentationProxy);

//---------------------------------------------------------------------------
vtkSMWidgetRepresentationProxy::vtkSMWidgetRepresentationProxy()
{
  this->RepresentationState = -1;
}

//---------------------------------------------------------------------------
vtkSMWidgetRepresentationProxy::~vtkSMWidgetRepresentationProxy()
{
}

//---------------------------------------------------------------------------
void vtkSMWidgetRepresentationProxy::OnStartInteraction()
{
}

//---------------------------------------------------------------------------
void vtkSMWidgetRepresentationProxy::OnInteraction()
{
  this->SendRepresentation();
}

//---------------------------------------------------------------------------
void vtkSMWidgetRepresentationProxy::OnEndInteraction()
{
  this->SendRepresentation();
}

//---------------------------------------------------------------------------
void vtkSMWidgetRepresentationProxy::SendRepresentation()
{
}

//---------------------------------------------------------------------------
void vtkSMWidgetRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
