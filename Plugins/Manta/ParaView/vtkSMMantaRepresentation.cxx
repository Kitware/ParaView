/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMantaRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMMantaRepresentation.h"
#include "vtkObjectFactory.h"

#include "vtkSMIntVectorProperty.h"
#include "vtkSMOutputPort.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMSurfaceRepresentationProxy.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMMantaRepresentation);

//----------------------------------------------------------------------------
vtkSMMantaRepresentation::vtkSMMantaRepresentation()
{
  this->MaterialType = NULL;
  this->Reflectance = 0.0;
  this->Thickness = 1.0;
  this->Eta = 1.52;
  this->N = 0.0;
  this->Nt = 0.0;
}

//----------------------------------------------------------------------------
vtkSMMantaRepresentation::~vtkSMMantaRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkSMMantaRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkSMMantaRepresentation::BeginCreateVTKObjects()
{
  //kicks off object factory override, that swaps GL classes for manta ones
  //does so ONLY on the server
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkClientServerID id = pm->NewStreamObject("vtkServerSideFactory", stream);
  stream << vtkClientServerStream::Invoke
         << id << "EnableFactory"
         << vtkClientServerStream::End;
  pm->DeleteStreamObject(id, stream);
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::RENDER_SERVER,
                 stream);

  return this->Superclass::BeginCreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMMantaRepresentation::CallMethod(char *methodName, 
                                          char *strArg, 
                                          double doubleArg)
{
  vtkSMProxy *property = NULL;
  vtkSMSurfaceRepresentationProxy *surface = 
    vtkSMSurfaceRepresentationProxy::SafeDownCast
    (this->GetSubProxy("SurfaceRepresentation"));
  if (surface)
    {
    property = surface->GetPropertyProxy();
    }

  if (!property)
    {
    return;
    }

  //use streams instead of properties because the client
  //doesn't have a vtkMantaRenderer
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  vtkClientServerID id = property->GetID();
  stream << vtkClientServerStream::Invoke
         << id << methodName;
  if (strArg)
    {
    stream << strArg;
    }
  else
    {
    stream << doubleArg;
    }
  stream << vtkClientServerStream::End;
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::RENDER_SERVER,
                 stream);
}

//----------------------------------------------------------------------------
void vtkSMMantaRepresentation::SetMaterialType(char *newval)
{
  if ( this->MaterialType == NULL && newval == NULL) 
    {
    return;
    } 
  if ( this->MaterialType && newval && (!strcmp(this->MaterialType,newval))) 
    { 
    return;
    }
  if (this->MaterialType) 
    { 
    delete [] this->MaterialType; 
    }
  if (newval)
    {
    size_t n = strlen(newval) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = (newval);
    this->MaterialType = cp1;
    do { *cp1++ = *cp2++; } while ( --n );
    }
  else
    {
    this->MaterialType = NULL;
    }

  this->CallMethod("SetMaterialType", newval, -1);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMMantaRepresentation::SetReflectance(double newval)
{
  if (this->Reflectance == newval)
    {
    return;
    }

  this->Reflectance = newval;
  this->CallMethod("SetReflectance", NULL, newval);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMMantaRepresentation::SetThickness(double newval)
{
  if (this->Thickness == newval)
    {
    return;
    }
  this->Thickness = newval;
  this->CallMethod("SetThickness", NULL, newval);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMMantaRepresentation::SetEta(double newval)
{
  if (this->Eta == newval)
    {
    return;
    }
  this->Eta = newval;
  this->CallMethod("SetEta", NULL, newval);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMMantaRepresentation::SetN(double newval)
{
  if (this->N == newval)
    {
    return;
    }
  this->N = newval;
  this->CallMethod("SetN", NULL, newval);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMMantaRepresentation::SetNt(double newval)
{
  if (this->Nt == newval)
    {
    return;
    }
  this->Nt = newval;
  this->CallMethod("SetNt", NULL, newval);
  this->Modified();
}

