/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "vtkSQPlaneSourceConfigurationWriter.h"
#include "vtkSQPlaneSourceConfigurationFileInfo.h"

#include "vtkObjectFactory.h"
#include "vtkStringList.h"
#include "vtkSMNamedPropertyIterator.h"

vtkCxxRevisionMacro(vtkSQPlaneSourceConfigurationWriter,"$Revision: 1.0$");
vtkStandardNewMacro(vtkSQPlaneSourceConfigurationWriter);

//-----------------------------------------------------------------------------
vtkSQPlaneSourceConfigurationWriter::vtkSQPlaneSourceConfigurationWriter()
{
  vtkStringList *propNames=vtkStringList::New();
  propNames->AddString("Name");
  propNames->AddString("Origin");
  propNames->AddString("Point1");
  propNames->AddString("Point2");
  propNames->AddString("XResolution");
  propNames->AddString("YResolution");

  vtkSMNamedPropertyIterator *propIt=vtkSMNamedPropertyIterator::New();
  propIt->SetPropertyNames(propNames);
  propNames->Delete();
  this->SetPropertyIterator(propIt);
  propIt->Delete();

  vtkSQPlaneSourceConfigurationFileInfo info;
  this->SetFileIdentifier(info.FileIdentifier);
  this->SetFileDescription(info.FileDescription);
  this->SetFileExtension(info.FileExtension);
}

//-----------------------------------------------------------------------------
vtkSQPlaneSourceConfigurationWriter::~vtkSQPlaneSourceConfigurationWriter()
{}

//-----------------------------------------------------------------------------
void vtkSQPlaneSourceConfigurationWriter::SetProxy(
      vtkSMProxy *proxy)
{
  this->vtkSMProxyConfigurationWriter::SetProxy(proxy);
  this->GetPropertyIterator()->SetProxy(proxy);
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSourceConfigurationWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

