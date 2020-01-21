/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyConfigurationWriter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyConfigurationWriter.h"

#include "vtkObjectFactory.h"
#include "vtkSMNamedPropertyIterator.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyConfigurationFileInfo.h"

#include "vtkPVXMLElement.h"
#include "vtkStringList.h"

#include "vtksys/FStream.hxx"

#include <iostream>

#define safeio(a) ((a) ? (a) : "NULL")

vtkStandardNewMacro(vtkSMProxyConfigurationWriter);

//---------------------------------------------------------------------------
vtkSMProxyConfigurationWriter::vtkSMProxyConfigurationWriter()
  : FileName(0)
  , Proxy(0)
  , PropertyIterator(0)
  , FileIdentifier(0)
  , FileDescription(0)
  , FileExtension(0)
{
  vtkSMProxyConfigurationFileInfo info;
  this->SetFileIdentifier(info.FileIdentifier);
  this->SetFileDescription(info.FileDescription);
  this->SetFileExtension(info.FileExtension);
}

//---------------------------------------------------------------------------
vtkSMProxyConfigurationWriter::~vtkSMProxyConfigurationWriter()
{
  this->SetFileName(0);
  this->SetProxy(0);
  this->SetPropertyIterator(0);
  this->SetFileIdentifier(0);
  this->SetFileDescription(0);
  this->SetFileExtension(0);
}

//---------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkSMProxyConfigurationWriter, PropertyIterator, vtkSMPropertyIterator);

//---------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkSMProxyConfigurationWriter, Proxy, vtkSMProxy);

//---------------------------------------------------------------------------
int vtkSMProxyConfigurationWriter::WriteConfiguration(std::ostream& os)
{
  // The user didn't set a iterator, assume he wants all
  // of the properties saved, use the default iterator.
  int deleteIter = 0;
  vtkSMPropertyIterator* iter = this->PropertyIterator;
  if (!iter)
  {
    iter = this->Proxy->NewPropertyIterator();
    deleteIter = 1;
  }

  os << "<?xml version=\"1.0\"?>" << endl;

  vtkPVXMLElement* state = vtkPVXMLElement::New();
  state->SetName(this->GetFileIdentifier());
  state->AddAttribute("description", this->GetFileDescription());
  state->AddAttribute("version", this->GetWriterVersion());

  // We don't want Sub-proxy
  this->Proxy->SaveXMLState(state, iter);

  state->PrintXML(os, vtkIndent());
  state->Delete();

  // clean up the default iterator
  if (deleteIter)
  {
    iter->Delete();
  }

  return 1;
}

//---------------------------------------------------------------------------
int vtkSMProxyConfigurationWriter::WriteConfiguration(const char* cFilename)
{
  if (cFilename == 0)
  {
    vtkErrorMacro("Cannot write filename NULL.");
    return 0;
  }

  // If there client has set an extension then we add it if it's not
  // present. To save with out th eextension, set it NULL.
  const char* cExt = this->GetFileExtension();
  cExt = (cExt == NULL ? "" : cExt);
  std::string filename(cFilename);
  std::string ext(cExt);
  if (!ext.empty() && (filename.size() <= ext.size() ||
                        filename.find(ext, filename.size() - ext.size()) == std::string::npos))
  {
    filename += ext;
  }

  vtksys::ofstream os(filename.c_str(), ios::out);
  if (!os.good())
  {
    vtkErrorMacro("Failed to open " << filename.c_str() << " for writing.");
    return 0;
  }
  this->WriteConfiguration(os);
  os.close();

  return 1;
}

//---------------------------------------------------------------------------
int vtkSMProxyConfigurationWriter::WriteConfiguration()
{
  return this->WriteConfiguration(this->FileName);
}

//---------------------------------------------------------------------------
void vtkSMProxyConfigurationWriter::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "FileName: " << safeio(this->FileName) << endl
     << indent << "Proxy: " << this->Proxy << endl
     << indent << "PropertyIterator: " << this->PropertyIterator << endl
     << indent << "Proxy: " << Proxy << endl
     << indent << "FileIdentifier: " << safeio(this->GetFileIdentifier()) << endl
     << indent << "FileDescription: " << safeio(this->GetFileDescription()) << endl
     << indent << "FileExtension: " << safeio(this->GetFileExtension()) << endl
     << indent << "WriterVersion: " << safeio(this->GetWriterVersion()) << endl;
}
