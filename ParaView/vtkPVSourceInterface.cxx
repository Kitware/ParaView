/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSourceInterface.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/

#include "vtkPVSourceInterface.h"
#include "vtkStringList.h"
#include "vtkPVExtentTranslator.h"

int vtkPVSourceInterfaceCommand(ClientData cd, Tcl_Interp *interp,
			        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVSourceInterface::vtkPVSourceInterface()
{
  static int instanceCount = 0;
  
  this->InstanceCount = 1;
  ++instanceCount;
  
  this->SourceClassName = NULL;
  this->RootName = NULL;
  this->InputClassName = NULL;
  this->OutputClassName = NULL;
  this->DataFileName = NULL;
  this->DefaultScalars = 0;
  
  this->MethodInterfaces = vtkCollection::New();

  this->CommandFunction = vtkPVSourceInterfaceCommand;
  this->PVWindow = NULL;

}

//----------------------------------------------------------------------------
vtkPVSourceInterface::~vtkPVSourceInterface()
{
  this->MethodInterfaces->Delete();
  this->MethodInterfaces = NULL;
  
  this->SetSourceClassName(NULL);
  this->SetRootName(NULL);
  this->SetInputClassName(NULL);
  this->SetOutputClassName(NULL);
  this->SetDataFileName(NULL);
  
  this->SetPVWindow(NULL);
}

//----------------------------------------------------------------------------
vtkPVSourceInterface* vtkPVSourceInterface::New()
{
  return new vtkPVSourceInterface();
}

//----------------------------------------------------------------------------
void vtkPVSourceInterface::SetPVWindow(vtkPVWindow *w)
{
  this->PVWindow = w;
}

//----------------------------------------------------------------------------
vtkPVSource *vtkPVSourceInterface::CreateCallback()
{
  char tclName[100], extentTclName[100];
  const char *outputDataType;
  vtkDataSet *d;
  vtkPVData *pvd;
  vtkSource *s;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVMethodInterface *mInt;
  
  // Before we do anything, let see if we can determine the output type.
  outputDataType = this->GetOutputClassName();
  if (strcmp(outputDataType, "vtkDataSet") == 0)
    { // Output will be the same as the input.
    vtkPVData *current = this->PVWindow->GetCurrentPVData();
    if (current == NULL)
      {
      vtkErrorMacro("Cannot determine output type.");
      return NULL;
      }
    outputDataType = current->GetVTKData()->GetClassName();
    }
  if (strcmp(outputDataType, "vtkPointSet") == 0)
    { // Output will be the same as the input.
    vtkPVData *current = this->PVWindow->GetCurrentPVData();
    if (current == NULL)
      {
      vtkErrorMacro("Cannot determine output type.");
      return NULL;
      }
    outputDataType = current->GetVTKData()->GetClassName();
    }

  // Create the vtkSource.
  sprintf(tclName, "%s%d", this->RootName, this->InstanceCount);
  // Create the object through tcl on all processes.
  s = (vtkSource *)(pvApp->MakeTclObject(this->SourceClassName, tclName));
  if (s == NULL)
    {
    vtkErrorMacro("Could not get pointer from object.");
    return NULL;
    }
  
  pvs = vtkPVSource::New();
  pvs->SetPropertiesParent(this->PVWindow->GetMainView()->GetPropertiesParent());
  pvs->SetApplication(pvApp);
  pvs->SetInterface(this);
  pvs->SetVTKSource(s, tclName);
  pvs->SetName(tclName);  
  
  // Set the input if necessary.
  if (this->InputClassName)
    {
    vtkPVData *current = this->PVWindow->GetCurrentPVData();
    pvs->SetNthPVInput(0, current);
    }
  
  // Add the new Source to the View, and make it current.
  this->PVWindow->GetMainView()->AddComposite(pvs);
  pvs->CreateProperties();
  if (this->DefaultScalars)
    {
    pvs->PackScalarsMenu();
    }
  
  pvs->CreateInputList(this->InputClassName);
  this->PVWindow->SetCurrentPVSource(pvs);

  // Create the output.
  pvd = vtkPVData::New();
  pvd->SetApplication(pvApp);
  sprintf(tclName, "%sOutput%d", this->RootName, this->InstanceCount);
  // Create the object through tcl on all processes.
  d = (vtkDataSet *)(pvApp->MakeTclObject(outputDataType, tclName));
  pvd->SetVTKData(d, tclName);

  // Connect the source and data.
  pvs->SetNthPVOutput(0, pvd);
  // It would be nice to have the vtkPVSource set this up, but for multiple outputs,
  // How do we know the method.
  // Relay the connection to the VTK objects.  
  pvApp->BroadcastScript("%s SetOutput %s", pvs->GetVTKSourceTclName(),
			 pvd->GetVTKDataTclName());   

  if (!this->InputClassName)
    {
    sprintf(extentTclName, "%s%dTranslator", this->RootName,
	    this->InstanceCount);
    pvApp->MakeTclObject("vtkPVExtentTranslator", extentTclName);
    pvApp->BroadcastScript("%s SetOriginalSource [%s GetOutput]",
			   extentTclName, pvs->GetVTKSourceTclName());
    pvApp->BroadcastScript("%s SetExtentTranslator %s",
			   pvd->GetVTKDataTclName(), extentTclName);
    // Hold onto name so it can be deleted.
    pvs->SetExtentTranslatorTclName(extentTclName);
    }
  else
    {
    pvApp->BroadcastScript(
      "%s SetExtentTranslator [[%s GetInput] GetExtentTranslator]",
      pvd->GetVTKDataTclName(), pvs->GetVTKSourceTclName());
    }

  // Loop through the methods creating widgets.
  this->MethodInterfaces->InitTraversal();
  while ( (mInt = ((vtkPVMethodInterface*)(this->MethodInterfaces->GetNextItemAsObject()))) )
    {
    //---------------------------------------------------------------------
    // This is a poor way to create widgets.  Another method that integrates
    // with vtkPVMethodInterfaces should be created.
  
    if (mInt->GetWidgetType() == VTK_PV_METHOD_WIDGET_FILE)
      {
      if (this->GetDataFileName())
	{
	this->Script("%s %s %s",
		     pvs->GetVTKSourceTclName(), mInt->GetSetCommand(),
		     this->GetDataFileName());
	}
      pvs->AddFileEntry(mInt->GetVariableName(), 
			mInt->GetSetCommand(),
			mInt->GetGetCommand(), 
			mInt->GetFileExtension(),
                        mInt->GetBalloonHelp());
      }
    else if (mInt->GetWidgetType() == VTK_PV_METHOD_WIDGET_TOGGLE)
      {
      pvs->AddLabeledToggle(mInt->GetVariableName(), 
			    mInt->GetSetCommand(),
			    mInt->GetGetCommand(),
                            mInt->GetBalloonHelp());
      }
    else if (mInt->GetWidgetType() == VTK_PV_METHOD_WIDGET_SELECTION)
      {
      int i;
      vtkStringList *l;
      pvs->AddModeList(mInt->GetVariableName(),
		       mInt->GetSetCommand(),
		       mInt->GetGetCommand(),
                       mInt->GetBalloonHelp());
      l = mInt->GetSelectionEntries();
      for (i = 0; i < l->GetLength(); ++i)
	{
	pvs->AddModeListItem(l->GetString(i), i);
	}
      }
    else if (mInt->GetWidgetType() == VTK_PV_METHOD_WIDGET_EXTENT)
      {
      this->Script("eval %s %s [[%s GetInput] GetWholeExtent]",
		   pvs->GetVTKSourceTclName(), mInt->GetSetCommand(),
		   pvs->GetVTKSourceTclName());
      pvs->AddVector6Entry(mInt->GetVariableName(), "", "", "", "", "", "",
			   mInt->GetSetCommand(),
			   mInt->GetGetCommand(),
                           mInt->GetBalloonHelp());      
      }
    else if (mInt->GetNumberOfArguments() == 1)
      {
      if (mInt->GetArgumentType(0) == VTK_STRING)
	{
	pvs->AddStringEntry(mInt->GetVariableName(), 
			    mInt->GetSetCommand(),
			    mInt->GetGetCommand(),
                            mInt->GetBalloonHelp());
	}
      else
	{
	pvs->AddLabeledEntry(mInt->GetVariableName(), 
			     mInt->GetSetCommand(),
			     mInt->GetGetCommand(),
                             mInt->GetBalloonHelp());
	}
      }
    else if (mInt->GetNumberOfArguments() == 2)
      {
      pvs->AddVector2Entry(mInt->GetVariableName(), "", "", 
			   mInt->GetSetCommand(),
			   mInt->GetGetCommand(),
                           mInt->GetBalloonHelp());
      }
    else if (mInt->GetNumberOfArguments() == 3)
      {
      pvs->AddVector3Entry(mInt->GetVariableName(), "", "", "",
			   mInt->GetSetCommand(),
			   mInt->GetGetCommand(),
                           mInt->GetBalloonHelp());
      }
    else if (mInt->GetNumberOfArguments() == 4)
      {
      pvs->AddVector4Entry(mInt->GetVariableName(), "", "", "", "",
			   mInt->GetSetCommand(),
			   mInt->GetGetCommand(),
                           mInt->GetBalloonHelp());
      }
    else if (mInt->GetNumberOfArguments() == 6)
      {
      pvs->AddVector6Entry(mInt->GetVariableName(), "", "", "", "", "", "",
			   mInt->GetSetCommand(),
			   mInt->GetGetCommand(),
                           mInt->GetBalloonHelp());
      }
    else
      {
      vtkErrorMacro("I do not handle this widget type yet.");
      }
    }
  pvs->UpdateParameterWidgets();
  
  pvs->Delete();

  ++this->InstanceCount;
  return pvs;
} 

//----------------------------------------------------------------------------
void vtkPVSourceInterface::AddMethodInterface(vtkPVMethodInterface *mInt)
{
  this->MethodInterfaces->AddItem(mInt);
}


//----------------------------------------------------------------------------
vtkPVApplication *vtkPVSourceInterface::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->GetApplication());
}

//----------------------------------------------------------------------------
int vtkPVSourceInterface::GetIsValidInput(vtkPVData *pvd)
{
  vtkDataObject *data;
  
  if (this->InputClassName == NULL)
    {
    return 0;
    }
  
  data = pvd->GetVTKData();
  return data->IsA(this->InputClassName);
}

//----------------------------------------------------------------------------
void vtkPVSourceInterface::Save(ofstream *file, const char *sourceName)
{
  vtkCollection *methods;
  vtkPVMethodInterface *currentMethod;
  int i;
  char *result;
  
  methods = this->GetMethodInterfaces();
  for (i = 0; i < methods->GetNumberOfItems(); i++)
    {
    currentMethod = (vtkPVMethodInterface*)methods->GetItemAsObject(i);
    *file << "\t" << sourceName << " " << currentMethod->GetSetCommand();
    this->Script("set tempValue [%s %s]", sourceName,
                 currentMethod->GetGetCommand());
    result = this->Application->GetMainInterp()->result;
    *file << " " << result << "\n";
    }
}
