// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
vtkSMWidgetRepresentationProxy::~vtkSMWidgetRepresentationProxy() = default;

//---------------------------------------------------------------------------
void vtkSMWidgetRepresentationProxy::OnStartInteraction() {}

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
void vtkSMWidgetRepresentationProxy::SendRepresentation() {}

//---------------------------------------------------------------------------
void vtkSMWidgetRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
