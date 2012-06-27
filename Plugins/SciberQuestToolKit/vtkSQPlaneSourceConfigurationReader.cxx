/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQPlaneSourceConfigurationReader.h"
#include "vtkSQPlaneSourceConfigurationFileInfo.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxy.h"

vtkStandardNewMacro(vtkSQPlaneSourceConfigurationReader);

//-----------------------------------------------------------------------------
vtkSQPlaneSourceConfigurationReader::vtkSQPlaneSourceConfigurationReader()
{
  // Valid camera configuration can come from a various
  // proxy types, eg RenderView,IceTRenderView and so on.
  this->SetValidateProxyType(0);

  vtkSQPlaneSourceConfigurationFileInfo info;
  this->SetFileIdentifier(info.FileIdentifier);
  this->SetFileDescription(info.FileDescription);
  this->SetFileExtension(info.FileExtension);
}

//-----------------------------------------------------------------------------
vtkSQPlaneSourceConfigurationReader::~vtkSQPlaneSourceConfigurationReader()
{}

//-----------------------------------------------------------------------------
int vtkSQPlaneSourceConfigurationReader::ReadConfiguration(const char *filename)
{
  int ok=this->Superclass::ReadConfiguration(filename);
  if (!ok)
    {
    return 0;
    }

  this->GetProxy()->UpdateVTKObjects();

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQPlaneSourceConfigurationReader::ReadConfiguration(vtkPVXMLElement *x)
{
  int ok=this->Superclass::ReadConfiguration(x);
  if (!ok)
    {
    return 0;
    }

  this->GetProxy()->UpdateVTKObjects();

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQPlaneSourceConfigurationReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
