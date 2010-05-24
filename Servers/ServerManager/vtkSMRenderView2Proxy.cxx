/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRenderView2Proxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMRenderView2Proxy);
vtkCxxRevisionMacro(vtkSMRenderView2Proxy, "$Revision$");
//----------------------------------------------------------------------------
vtkSMRenderView2Proxy::vtkSMRenderView2Proxy()
{
}

//----------------------------------------------------------------------------
vtkSMRenderView2Proxy::~vtkSMRenderView2Proxy()
{
}

//----------------------------------------------------------------------------
void vtkSMRenderView2Proxy::EndCreateVTKObjects()
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->GetID()
         << "Initialize"
         << static_cast<unsigned int>(this->GetSelfID().ID)
         << vtkClientServerStream::End;

  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID,
    this->Servers, stream);

  this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMRenderView2Proxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
