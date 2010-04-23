/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMSurfaceLICRepresentationProxy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMSurfaceLICRepresentationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMPropertyHelper.h"

vtkStandardNewMacro(vtkSMSurfaceLICRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMSurfaceLICRepresentationProxy::vtkSMSurfaceLICRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSMSurfaceLICRepresentationProxy::~vtkSMSurfaceLICRepresentationProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMSurfaceLICRepresentationProxy::SetUseLICForLOD(int use)
{
  vtkSMProxy* licPainterLOD = this->GetSubProxy("SurfaceLICPainterLOD");    
  vtkSMPropertyHelper(licPainterLOD, "Enable").Set(use);
  licPainterLOD->UpdateProperty("Enable");
}

//----------------------------------------------------------------------------
bool vtkSMSurfaceLICRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  vtkSMProxy* licPainter = this->GetSubProxy("SurfaceLICPainter");
  licPainter->SetServers(vtkProcessModule::RENDER_SERVER|vtkProcessModule::CLIENT);

  vtkSMProxy* licPainterLOD = this->GetSubProxy("SurfaceLICPainterLOD");
  licPainterLOD->SetServers(vtkProcessModule::RENDER_SERVER|vtkProcessModule::CLIENT);

  vtkSMProxy* licDefaultPainter = this->GetSubProxy("SurfaceLICDefaultPainter");
  licDefaultPainter->SetServers(vtkProcessModule::RENDER_SERVER|vtkProcessModule::CLIENT);

  vtkSMProxy* licDefaultPainterLOD = this->GetSubProxy("SurfaceLICDefaultPainterLOD");
  licDefaultPainterLOD->SetServers(vtkProcessModule::RENDER_SERVER|vtkProcessModule::CLIENT);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSurfaceLICRepresentationProxy::EndCreateVTKObjects()
{
  vtkSMProxy* licPainter = this->GetSubProxy("SurfaceLICPainter");
  vtkSMProxy* licPainterLOD = this->GetSubProxy("SurfaceLICPainterLOD");
  vtkSMProxy* licDefaultPainter = this->GetSubProxy("SurfaceLICDefaultPainter");
  vtkSMProxy* licDefaultPainterLOD = this->GetSubProxy("SurfaceLICDefaultPainterLOD");
  
  vtkSMPropertyHelper( licPainterLOD, "EnhancedLIC").Set( 0 );
  licPainterLOD->UpdateProperty( "EnhancedLIC" );

  vtkSMPropertyHelper(licDefaultPainter, "SurfaceLICPainter").Set(licPainter);
  vtkSMPropertyHelper(licDefaultPainterLOD, "SurfaceLICPainter").Set(licPainterLOD);
  licDefaultPainter->UpdateProperty("SurfaceLICPainter");
  licDefaultPainterLOD->UpdateProperty("SurfaceLICPainter");

  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->Mapper->GetID()
          << "GetPainter"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << vtkClientServerStream::LastResult
          << "GetDelegatePainter"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << licDefaultPainter->GetID()
          << "SetDelegatePainter"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << this->Mapper->GetID()
          << "SetPainter"
          << licDefaultPainter->GetID()
          << vtkClientServerStream::End;

  stream  << vtkClientServerStream::Invoke
          << this->LODMapper->GetID()
          << "GetPainter"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << vtkClientServerStream::LastResult
          << "GetDelegatePainter"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << licDefaultPainterLOD->GetID()
          << "SetDelegatePainter"
          << vtkClientServerStream::LastResult
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << this->LODMapper->GetID()
          << "SetPainter"
          << licDefaultPainterLOD->GetID()
          << vtkClientServerStream::End;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->GetConnectionID(), 
    vtkProcessModule::RENDER_SERVER | vtkProcessModule::CLIENT,
    stream);
  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMSurfaceLICRepresentationProxy::SelectInputVectors(int, int, int, 
  int attributeMode, const char* name)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Objects not created yet!");
    return;
    }

  vtkSMProxy* licPainter = this->GetSubProxy("SurfaceLICPainter");
  licPainter->SetServers(vtkProcessModule::RENDER_SERVER|vtkProcessModule::CLIENT);

  vtkSMProxy* licPainterLOD = this->GetSubProxy("SurfaceLICPainterLOD");
  licPainterLOD->SetServers(vtkProcessModule::RENDER_SERVER|vtkProcessModule::CLIENT);

  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << licPainter->GetID()
          << "SetInputArrayToProcess"
          << attributeMode
          << name
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << licPainterLOD->GetID()
          << "SetInputArrayToProcess"
          << attributeMode
          << name
          << vtkClientServerStream::End;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->GetConnectionID(), 
    vtkProcessModule::RENDER_SERVER | vtkProcessModule::CLIENT,
    stream);
}

//----------------------------------------------------------------------------
void vtkSMSurfaceLICRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
