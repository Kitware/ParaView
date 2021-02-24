/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMProxyConfigurationReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyConfigurationReader.h"
#include "vtkSMCameraConfigurationFileInfo.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"

#include <string>

#define safeio(a) ((a) ? (a) : "NULL")

vtkStandardNewMacro(vtkSMProxyConfigurationReader);

//-----------------------------------------------------------------------------
vtkSMProxyConfigurationReader::vtkSMProxyConfigurationReader()
  : FileName(nullptr)
  , ValidateProxyType(1)
  , Proxy(nullptr)
  , FileIdentifier(nullptr)
  , FileDescription(nullptr)
  , FileExtension(nullptr)
{
  vtkSMCameraConfigurationFileInfo info;
  this->SetFileIdentifier(info.FileIdentifier);
  this->SetFileDescription(info.FileDescription);
  this->SetFileExtension(info.FileExtension);
}

//-----------------------------------------------------------------------------
vtkSMProxyConfigurationReader::~vtkSMProxyConfigurationReader()
{
  this->SetFileName(nullptr);
  this->SetProxy(nullptr);
  this->SetFileIdentifier(nullptr);
  this->SetFileDescription(nullptr);
  this->SetFileExtension(nullptr);
}

//-----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkSMProxyConfigurationReader, Proxy, vtkSMProxy);

//-----------------------------------------------------------------------------
bool vtkSMProxyConfigurationReader::CanReadVersion(const char* version)
{
  return std::string(version) == this->GetReaderVersion();
}

//-----------------------------------------------------------------------------
int vtkSMProxyConfigurationReader::ReadConfiguration()
{
  return this->ReadConfiguration(this->FileName);
}

//-----------------------------------------------------------------------------
int vtkSMProxyConfigurationReader::ReadConfiguration(const char* filename)
{
  if (filename == nullptr)
  {
    vtkErrorMacro("Cannot read from filename NULL.");
    return 0;
  }

  vtkSmartPointer<vtkPVXMLParser> parser = vtkSmartPointer<vtkPVXMLParser>::New();
  parser->SetFileName(filename);
  if (parser->Parse() == 0)
  {
    vtkErrorMacro("Invalid XML in file: " << filename << ".");
    return 0;
  }

  vtkPVXMLElement* xmlStream = parser->GetRootElement();
  if (xmlStream == nullptr)
  {
    vtkErrorMacro("Invalid XML in file: " << filename << ".");
    return 0;
  }

  return this->ReadConfiguration(xmlStream);
}

//-----------------------------------------------------------------------------
int vtkSMProxyConfigurationReader::ReadConfiguration(vtkPVXMLElement* configXml)
{
  std::string requiredIdentifier(this->GetFileIdentifier());
  const char* foundIdentifier = configXml->GetName();
  if (foundIdentifier == nullptr || foundIdentifier != requiredIdentifier)
  {
    vtkErrorMacro(<< "This is not a valid " << this->GetFileDescription() << " XML hierarchy.");
    return 0;
  }

  const char* foundVersion = configXml->GetAttribute("version");
  if (foundVersion == nullptr)
  {
    vtkErrorMacro("No \"version\" attribute was found.");
    return 0;
  }

  if (!this->CanReadVersion(foundVersion))
  {
    vtkErrorMacro("Unsupported version " << foundVersion << ".");
    return 0;
  }

  // Find a proxy emlement, this hierarchy is expected to contain one
  // and only one Proxy element.
  vtkPVXMLElement* proxyXml = configXml->FindNestedElementByName("Proxy");
  if (proxyXml == nullptr)
  {
    vtkErrorMacro("No \"Proxy\" element was found.");
    return 0;
  }

  // Compare type of proxy in the file with the one we have to make
  // sure they match.
  const char* foundType = proxyXml->GetAttribute("type");
  std::string requiredType = this->Proxy->GetXMLName();
  if (this->ValidateProxyType && (foundType == nullptr || foundType != requiredType))
  {
    vtkErrorMacro(<< "This is not a valid " << requiredType << " XML hierarchy.");
    return 0;
  }

  // Push hierarchy to the proxy.
  int ok = this->Proxy->LoadXMLState(proxyXml, nullptr);
  if (!ok)
  {
    vtkErrorMacro("Proxy::LoadState failed.");
    return 0;
  }
  this->Proxy->UpdateVTKObjects();

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMProxyConfigurationReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << safeio(this->FileName) << endl
     << indent << "Proxy: " << Proxy << endl
     << indent << "FileIdentifier: " << safeio(this->GetFileIdentifier()) << endl
     << indent << "FileDescription: " << safeio(this->GetFileDescription()) << endl
     << indent << "FileExtension: " << safeio(this->GetFileExtension()) << endl
     << indent << "ReaderVersion: " << safeio(this->GetReaderVersion()) << endl;
}
