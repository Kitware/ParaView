/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSourceInterface.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkPVSourceInterface.h"
#include "vtkStringList.h"
#include "vtkPVExtentTranslator.h"
#include "vtkObjectFactory.h"

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
  this->DefaultVectors = 0;
  
  this->MethodInterfaces = vtkCollection::New();

  this->CommandFunction = vtkPVSourceInterfaceCommand;
  this->PVWindow = NULL;

  this->ReplaceInput = 1;
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
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVSourceInterface");
  if(ret)
    {
    return (vtkPVSourceInterface*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVSourceInterface;
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
  vtkPVData *current = this->PVWindow->GetCurrentPVData();
  
  // Before we do anything, let see if we can determine the output type.
  outputDataType = this->GetOutputClassName();
  if (strcmp(outputDataType, "vtkDataSet") == 0)
    { // Output will be the same as the input.
    if (current == NULL)
      {
      vtkErrorMacro("Cannot determine output type.");
      return NULL;
      }
    outputDataType = current->GetVTKData()->GetClassName();
    }
  if (strcmp(outputDataType, "vtkPointSet") == 0)
    { // Output will be the same as the input.
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

  this->Application->AddTraceEntry("# CreateCallback %s", tclName);
  
  pvs = vtkPVSource::New();
  pvs->SetReplaceInput(this->ReplaceInput);
  pvs->SetPropertiesParent(this->PVWindow->GetMainView()->GetPropertiesParent());
  pvs->SetApplication(pvApp);
  pvs->SetInterface(this);
  pvs->SetVTKSource(s, tclName);
  pvs->SetName(tclName);  
  
  // Set the input if necessary.
  if (this->InputClassName)
    {
    pvs->SetNthPVInput(0, current);
    }
  
  // Add the new Source to the View, and make it current.
  this->PVWindow->GetMainView()->AddComposite(pvs);
  pvs->CreateProperties();
  if (this->DefaultScalars)
    {
    pvs->PackScalarsMenu();
    }
  if (this->DefaultVectors)
    {
    pvs->PackVectorsMenu();
    }

  if (! this->InputClassName)
    {
    pvs->CreateInputList(NULL);
    }
  else
    {
    pvs->CreateInputList(pvs->GetNthPVInput(0)->GetVTKData()->GetClassName());
    }
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
    // Not as load balanced, but more efficient reading.
    if (s->IsA("vtkPOPReader"))
      {
      pvApp->BroadcastScript("%s SetSplitModeToYSlab", extentTclName);
      //pvApp->BroadcastScript("%s SetSplitModeToBlock", extentTclName);
      }
    }
  else
    {
    pvApp->BroadcastScript(
      "%s SetExtentTranslator [%s GetExtentTranslator]",
      pvd->GetVTKDataTclName(), current->GetVTKDataTclName());
    // What A pain.  we need this until we remove that drat FieldDataToAttributeDataFilter.
    pvApp->BroadcastScript(
      "[%s GetInput] SetExtentTranslator [%s GetExtentTranslator]",
      pvs->GetVTKSourceTclName(), current->GetVTKDataTclName());
    }
  
  // Hack here specifically for the POP reader.  Initialize the clip extent variable.
  if (s->IsA("vtkPOPReader"))
    {
    // Call update information first to set the clip extent.
    this->Script("%s SetFileName %s", pvs->GetVTKSourceTclName(), this->DataFileName);
    this->Script("%s UpdateInformation", pvd->GetVTKDataTclName());
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
      this->Script("eval %s %s [%s GetWholeExtent]",
		   pvs->GetVTKSourceTclName(), mInt->GetSetCommand(),
		   pvs->GetNthPVInput(0)->GetVTKDataTclName());
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
  pvd->Delete();

  ++this->InstanceCount;

  this->PVWindow->DisableMenus();
  
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
void vtkPVSourceInterface::SaveInTclScript(ofstream *file, const char *sourceName)
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
