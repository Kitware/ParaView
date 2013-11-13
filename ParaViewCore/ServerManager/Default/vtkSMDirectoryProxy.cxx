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
#include "vtkSMDirectoryProxy.h"

#include "vtkObjectFactory.h"
#include "vtkClientServerStream.h"
#include "vtkSMSession.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMDirectoryProxy);
//----------------------------------------------------------------------------
vtkSMDirectoryProxy::vtkSMDirectoryProxy()
{
}

//----------------------------------------------------------------------------
vtkSMDirectoryProxy::~vtkSMDirectoryProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMDirectoryProxy::List(const char* dir)
{
  this->CreateVTKObjects();
  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << VTKOBJECT(this) << "Open"
    << dir
    << vtkClientServerStream::End;
  this->ExecuteStream(stream, false, vtkProcessModule::DATA_SERVER_ROOT);
  this->UpdatePropertyInformation();
}
//----------------------------------------------------------------------------
bool vtkSMDirectoryProxy::MakeDirectory(const char* dir, vtkTypeUInt32 processes)
{
  this->CreateVTKObjects();
  if (!this->ObjectsCreated)
    {
    return false;
    }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << VTKOBJECT(this) << "MakeDirectory"
    << dir
    << vtkClientServerStream::End;
  this->ExecuteStream(stream, false, processes);

  vtkClientServerStream result = this->GetSession()->GetLastResult(processes);
  if(result.GetNumberOfMessages() == 1 &&
    result.GetNumberOfArguments(0) == 1)
    {
    int tmp;
    if(result.GetArgument(0, 0, &tmp) && tmp)
      {
      return true;
      }
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMDirectoryProxy::DeleteDirectory(const char* dir, vtkTypeUInt32 processes)
{
  this->CreateVTKObjects();
  if (!this->ObjectsCreated)
    {
    return false;
    }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << VTKOBJECT(this) << "DeleteDirectory"
    << dir
    << vtkClientServerStream::End;
  this->ExecuteStream(stream, false, processes);

  vtkClientServerStream result = this->GetSession()->GetLastResult(processes);
  if(result.GetNumberOfMessages() == 1 &&
    result.GetNumberOfArguments(0) == 1)
    {
    int tmp;
    if(result.GetArgument(0, 0, &tmp) && tmp)
      {
      return true;
      }
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMDirectoryProxy::Rename(const char* oldname, const char* newname,
  vtkTypeUInt32 processes)
{
  this->CreateVTKObjects();
  if (!this->ObjectsCreated)
    {
    return false;
    }

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
    << VTKOBJECT(this) << "Rename"
    << oldname << newname
    << vtkClientServerStream::End;
  this->ExecuteStream(stream, false, processes);

  vtkClientServerStream result = this->GetSession()->GetLastResult(processes);
  if(result.GetNumberOfMessages() == 1 &&
    result.GetNumberOfArguments(0) == 1)
    {
    int tmp;
    if(result.GetArgument(0, 0, &tmp) && tmp)
      {
      return true;
      }
    }

  return false;
}

//----------------------------------------------------------------------------
void vtkSMDirectoryProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
