/*=========================================================================

  Program:   ParaView
  Module:    vtkSIDirectoryProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIDirectoryProxy.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVSessionCore.h"
#include "vtkPVXMLElement.h"
#include "vtkSMMessage.h"

#include <sstream>
#include <string>
#include <vector>

//****************************************************************************
vtkStandardNewMacro(vtkSIDirectoryProxy);
//----------------------------------------------------------------------------
vtkSIDirectoryProxy::vtkSIDirectoryProxy() = default;

//----------------------------------------------------------------------------
vtkSIDirectoryProxy::~vtkSIDirectoryProxy() = default;

//----------------------------------------------------------------------------
void vtkSIDirectoryProxy::Pull(vtkSMMessage* message)
{
  if (!this->ObjectsCreated)
  {
    return;
  }

  // We just fill the SMMessage with our 2 fake properties
  message->ClearExtension(PullRequest::arguments);
  if (message->req_def())
  {
    // Add definition
    message->SetExtension(ProxyState::xml_group, this->XMLGroup);
    message->SetExtension(ProxyState::xml_name, this->XMLName);
    if (this->XMLSubProxyName)
    {
      message->SetExtension(ProxyState::xml_sub_proxy_name, this->XMLSubProxyName);
    }
  }

  // Extract file information
  // for idx in GetNumberOfFiles
  //    name = GetFile(idx)
  //    isDir = FileIsDirectory(name)
  //    (isDir ? dirList : fileList).push(name)
  vtkClientServerStream str;
  str << vtkClientServerStream::Invoke << this->GetVTKObject() << "GetNumberOfFiles"
      << vtkClientServerStream::End;

  this->GetInterpreter()->ProcessStream(str);

  // Get the result
  vtkIdType nbFiles = 0;
  if (!this->GetInterpreter()->GetLastResult().GetArgument(0, 0, &nbFiles))
  {
    vtkErrorMacro("Error getting return value of command: GetNumberOfFiles()");
  }

  std::string fileName;
  int isDirectory;
  std::vector<std::string> fileList;
  std::vector<std::string> directoryList;
  for (vtkIdType idx = 0; idx < nbFiles; ++idx)
  {
    vtkClientServerStream css;
    css << vtkClientServerStream::Invoke << this->GetVTKObject() << "GetFile" << idx
        << vtkClientServerStream::End;
    this->GetInterpreter()->ProcessStream(css);
    if (!this->GetInterpreter()->GetLastResult().GetArgument(0, 0, &fileName))
    {
      vtkErrorMacro("Error getting return value of command: GetFile(" << idx << ")");
      return;
    }
    css << vtkClientServerStream::Invoke << this->GetVTKObject() << "FileIsDirectory"
        << fileName.c_str() << vtkClientServerStream::End;
    this->GetInterpreter()->ProcessStream(css);
    if (!this->GetInterpreter()->GetLastResult().GetArgument(0, 0, &isDirectory))
    {
      vtkErrorMacro(
        "Error getting return value of command: FileIsDirectory(" << fileName.c_str() << ")");
      return;
    }
    if (isDirectory == 0)
    {
      fileList.push_back(fileName);
    }
    else
    {
      directoryList.push_back(fileName);
    }
  }

  // Fill message response
  // Files...
  ProxyState_Property* prop = message->AddExtension(ProxyState::property);
  prop->set_name("FileList");
  Variant* var = prop->mutable_value();
  var->set_type(Variant::STRING);
  for (size_t cc = 0; cc < fileList.size(); ++cc)
  {
    var->add_txt(fileList[cc]);
  }
  // Directories
  prop = message->AddExtension(ProxyState::property);
  prop->set_name("DirectoryList");
  var = prop->mutable_value();
  var->set_type(Variant::STRING);
  for (size_t cc = 0; cc < directoryList.size(); ++cc)
  {
    var->add_txt(directoryList[cc]);
  }
}

//----------------------------------------------------------------------------
bool vtkSIDirectoryProxy::ReadXMLProperty(vtkPVXMLElement* propElement)
{
  // We skip fake properties ;-)
  std::string name = propElement->GetAttributeOrEmpty("name");
  if (strcmp(name.c_str(), "FileList") == 0 || strcmp(name.c_str(), "DirectoryList"))
  {
    return true;
  }

  return this->Superclass::ReadXMLProperty(propElement);
}

//----------------------------------------------------------------------------
void vtkSIDirectoryProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
