// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMDataSourceProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMDataSourceProxy);

//---------------------------------------------------------------------------
vtkSMDataSourceProxy::vtkSMDataSourceProxy() = default;

//---------------------------------------------------------------------------
vtkSMDataSourceProxy::~vtkSMDataSourceProxy() = default;

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
