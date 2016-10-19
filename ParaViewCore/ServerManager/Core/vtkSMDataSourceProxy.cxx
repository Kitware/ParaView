/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataSourceProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDataSourceProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMDataSourceProxy);

//---------------------------------------------------------------------------
vtkSMDataSourceProxy::vtkSMDataSourceProxy()
{
}

//---------------------------------------------------------------------------
vtkSMDataSourceProxy::~vtkSMDataSourceProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMDataSourceProxy::CopyData(vtkSMSourceProxy* sourceProxy)
{
  if (!sourceProxy || this->Location != sourceProxy->GetLocation())
  {
    return;
  }
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(sourceProxy) << "GetOutput"
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);

  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "CopyData"
         << vtkClientServerStream::LastResult << vtkClientServerStream::End;
  this->ExecuteStream(stream);

  this->MarkModified(this);
}

//----------------------------------------------------------------------------
void vtkSMDataSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
