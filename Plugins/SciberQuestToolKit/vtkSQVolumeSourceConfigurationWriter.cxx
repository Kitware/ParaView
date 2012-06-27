/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQVolumeSourceConfigurationWriter.h"
#include "vtkSQVolumeSourceConfigurationFileInfo.h"

#include "vtkObjectFactory.h"
#include "vtkStringList.h"
#include "vtkSMNamedPropertyIterator.h"


vtkStandardNewMacro(vtkSQVolumeSourceConfigurationWriter);

//-----------------------------------------------------------------------------
vtkSQVolumeSourceConfigurationWriter::vtkSQVolumeSourceConfigurationWriter()
{
  vtkStringList *propNames=vtkStringList::New();
  propNames->AddString("Origin");
  propNames->AddString("Point1");
  propNames->AddString("Point2");
  propNames->AddString("Point3");
  propNames->AddString("Resolution");

  vtkSMNamedPropertyIterator *propIt=vtkSMNamedPropertyIterator::New();
  propIt->SetPropertyNames(propNames);
  propNames->Delete();
  this->SetPropertyIterator(propIt);
  propIt->Delete();

  vtkSQVolumeSourceConfigurationFileInfo info;
  this->SetFileIdentifier(info.FileIdentifier);
  this->SetFileDescription(info.FileDescription);
  this->SetFileExtension(info.FileExtension);
}

//-----------------------------------------------------------------------------
vtkSQVolumeSourceConfigurationWriter::~vtkSQVolumeSourceConfigurationWriter()
{}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceConfigurationWriter::SetProxy(
      vtkSMProxy *proxy)
{
  this->vtkSMProxyConfigurationWriter::SetProxy(proxy);
  this->GetPropertyIterator()->SetProxy(proxy);
}

//-----------------------------------------------------------------------------
void vtkSQVolumeSourceConfigurationWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
