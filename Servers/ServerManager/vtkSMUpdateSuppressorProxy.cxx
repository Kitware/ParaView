/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUpdateSuppressorProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUpdateSuppressorProxy.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMUpdateSuppressorProxy);

//---------------------------------------------------------------------------
vtkSMUpdateSuppressorProxy::vtkSMUpdateSuppressorProxy()
{
  this->DoInsertExtractPieces = 0;
}

//---------------------------------------------------------------------------
vtkSMUpdateSuppressorProxy::~vtkSMUpdateSuppressorProxy()
{
}

////---------------------------------------------------------------------------
//void vtkSMUpdateSuppressorProxy::UpdatePipeline(double vtkNotUsed(time))
//{
//  // UpdatePipeline doesn't update anything in case of UpdateSuppressor. Hence
//  // we just shunt update pipeline calls. One should use ForceUpdate().
//  vtkWarningMacro("Try using ForceUpdate() to update pipeline.");
//}
//
////---------------------------------------------------------------------------
//void vtkSMUpdateSuppressorProxy::UpdatePipeline()
//{
//  // UpdatePipeline doesn't update anything in case of UpdateSuppressor. Hence
//  // we just shunt update pipeline calls. One should use ForceUpdate().
//  vtkWarningMacro("Try using ForceUpdate() to update pipeline.");
//}

//---------------------------------------------------------------------------
void vtkSMUpdateSuppressorProxy::ForceUpdate()
{
  if (this->NeedsUpdate)
    {
    this->InvokeCommand("ForceUpdate");
    this->Superclass::UpdatePipeline();
    }
}

//---------------------------------------------------------------------------
void vtkSMUpdateSuppressorProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}







