/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "vtkSQHemisphereSourceConfigurationWriter.h"
#include "vtkSQHemisphereSourceConfigurationFileInfo.h"

#include "vtkObjectFactory.h"
#include "vtkStringList.h"
#include "vtkSMNamedPropertyIterator.h"
#include "vtkSMRenderViewProxy.h"

vtkCxxRevisionMacro(vtkSQHemisphereSourceConfigurationWriter,"$Revision: 1.0$");
vtkStandardNewMacro(vtkSQHemisphereSourceConfigurationWriter);

//-----------------------------------------------------------------------------
vtkSQHemisphereSourceConfigurationWriter::vtkSQHemisphereSourceConfigurationWriter()
{
  vtkStringList *propNames=vtkStringList::New();
  propNames->AddString("Center");
  propNames->AddString("Radius");
  propNames->AddString("Resolution");

  vtkSMNamedPropertyIterator *propIt=vtkSMNamedPropertyIterator::New();
  propIt->SetPropertyNames(propNames);
  propNames->Delete();
  this->SetPropertyIterator(propIt);
  propIt->Delete();

  vtkSQHemisphereSourceConfigurationFileInfo info;
  this->SetFileIdentifier(info.FileIdentifier);
  this->SetFileDescription(info.FileDescription);
  this->SetFileExtension(info.FileExtension);
}

//-----------------------------------------------------------------------------
vtkSQHemisphereSourceConfigurationWriter::~vtkSQHemisphereSourceConfigurationWriter()
{}

//-----------------------------------------------------------------------------
void vtkSQHemisphereSourceConfigurationWriter::SetProxy(
      vtkSMProxy *proxy)
{
  this->vtkSMProxyConfigurationWriter::SetProxy(proxy);
  this->GetPropertyIterator()->SetProxy(proxy);
}

//-----------------------------------------------------------------------------
void vtkSQHemisphereSourceConfigurationWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

