/*=========================================================================

Program:   ParaView
Module:    vtkSMWriterProxy.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMWriterProxy.h"

#include "vtkClientServerID.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMWriterProxy);
vtkCxxRevisionMacro(vtkSMWriterProxy, "Revision: 1.1 $");

void vtkSMWriterProxy::UpdatePipeline()
{
  this->Superclass::UpdatePipeline();

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int idx;
  for (idx = 0; idx < this->GetNumberOfIDs(); idx++)
    {
    str << vtkClientServerStream::Invoke
        << this->GetID(idx)
        << "Write"
        << vtkClientServerStream::End;
    }

  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers, str);
    }
}

void vtkSMWriterProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
