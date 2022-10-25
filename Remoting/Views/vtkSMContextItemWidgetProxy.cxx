/*=========================================================================

  Program:   ParaView
  Module:    vtkSMContextItemWidgetProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMContextItemWidgetProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMContextItemWidgetProxy);

//---------------------------------------------------------------------------
void vtkSMContextItemWidgetProxy::SendRepresentation()
{
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "Modified"
         << vtkClientServerStream::End;
  this->ExecuteStream(stream);
}

//---------------------------------------------------------------------------
void vtkSMContextItemWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
