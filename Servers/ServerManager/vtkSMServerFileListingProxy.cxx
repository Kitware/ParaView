/*=========================================================================

  Program:   ParaView
  Module:    vtkSMServerFileListingProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMServerFileListingProxy.h"
#include "vtkObjectFactory.h"
#include "vtkClientServerStream.h"
#include "vtkProcessModule.h"

vtkStandardNewMacro(vtkSMServerFileListingProxy);
vtkCxxRevisionMacro(vtkSMServerFileListingProxy, "1.1.2.1");
//-----------------------------------------------------------------------------
vtkSMServerFileListingProxy::vtkSMServerFileListingProxy()
{
  this->ActiveFileIsReadable = 0;
  this->ActiveFileIsDirectory = 0;
  this->ActiveFileName = 0;
}

//-----------------------------------------------------------------------------
vtkSMServerFileListingProxy::~vtkSMServerFileListingProxy()
{
  if (this->ActiveFileName)
    {
    delete [] this->ActiveFileName;
    this->ActiveFileName = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkSMServerFileListingProxy::SetActiveFileName(const char* name)
{
  this->ActiveFileIsReadable = 0;
  this->ActiveFileIsDirectory = 0;
  if (this->ActiveFileName)
    {
    delete [] this->ActiveFileName;
    this->ActiveFileName = 0;
    }
  if (!name || !name[0])
    {
    return;
    }
  int length = strlen(name);
  this->ActiveFileName = new char[length + 10];
  strcpy(this->ActiveFileName, name);
}

//-----------------------------------------------------------------------------
void vtkSMServerFileListingProxy::UpdateInformation()
{
  if (this->ObjectsCreated && this->ActiveFileName && 
    this->GetNumberOfIDs() > 0)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    vtkClientServerID id = this->GetID(0);
    stream << vtkClientServerStream::Invoke
      << id << "FileIsDirectory" << this->ActiveFileName
      << vtkClientServerStream::End;
    pm->SendStream(this->GetServers(), stream);
    int isdir;
    if(!pm->GetLastResult(this->GetServers()).GetArgument(0, 0, &isdir))
      {
      vtkErrorMacro("Error checking whether file is directory on server.");
      }
    else
      {
      this->ActiveFileIsDirectory = isdir;
      }
    
    stream << vtkClientServerStream::Invoke
      << id << "FileIsReadable" << this->ActiveFileName
      << vtkClientServerStream::End;
    pm->SendStream(this->GetServers(), stream);
    int isreadble;
    if(!pm->GetLastResult(this->GetServers()).GetArgument(0, 0, &isreadble))
      {
      vtkErrorMacro("Error checking whether file is readable on server.");
      }
    else
      {
      this->ActiveFileIsReadable = isreadble;
      }
    }
  this->Superclass::UpdateInformation();
}

//-----------------------------------------------------------------------------
void vtkSMServerFileListingProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ActiveFileIsReadable: " 
    << this->ActiveFileIsReadable << endl;
  os << indent << "ActiveFileIsDirectory: " 
    << this->ActiveFileIsDirectory << endl;
  os << indent << "ActiveFileName: " 
    << ( this->ActiveFileName? this->ActiveFileName : "(null)")
    << endl;
}
