/*=========================================================================

  Program:   ParaView
  Module:    vtkPVReaderModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVReaderModule.h"

#include "vtkObjectFactory.h"
#include "vtkCollectionIterator.h"
#include "vtkPVApplication.h"
#include "vtkKWFrame.h"
#include "vtkPVFileEntry.h"
#include "vtkPVScale.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVWidgetCollection.h"
#include "vtkPVWindow.h"
#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkVector.txx"
#include "vtkVectorIterator.txx"
#include <vtkstd/string>
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVReaderModule);
vtkCxxRevisionMacro(vtkPVReaderModule, "1.60");

int vtkPVReaderModuleCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVReaderModule::vtkPVReaderModule()
{
  this->CommandFunction = vtkPVReaderModuleCommand;
  this->FileEntry = 0;
  this->AcceptAfterRead = 1;
  this->Extensions = vtkVector<const char*>::New();
  this->Iterator = this->Extensions->NewIterator();
  this->PackFileEntry = 1;
  this->AddFileEntry = 1;
}

//----------------------------------------------------------------------------
vtkPVReaderModule::~vtkPVReaderModule()
{
  if (this->FileEntry)
    {
    this->FileEntry->Delete();
    }
  this->Extensions->Delete();
  this->Iterator->Delete();
}

//----------------------------------------------------------------------------
void vtkPVReaderModule::CreateProperties()
{
  this->Superclass::CreateProperties();

  this->FileEntry = vtkPVFileEntry::New();
  this->FileEntry->SetLabel("Filename");
  this->FileEntry->SetPVSource(this);
  this->FileEntry->SetParent(this->ParameterFrame->GetFrame());
  this->FileEntry->SetModifiedCommand(this->GetTclName(), 
                                      "SetAcceptButtonColorToModified");
  this->FileEntry->SetSMPropertyName("FileName");
  this->FileEntry->Create(this->GetPVApplication());
  if (this->AddFileEntry)
    {
    // This widget has to be the first.
    // Same as AddPVWidget, but puts first in the list.
    this->AddPVFileEntry(this->FileEntry);
    }
  
  if (this->PackFileEntry)
    {
    if ( this->FileEntry->GetParent()->GetNumberOfPackedChildren() > 0 )
      {
      this->Script("pack %s -pady 10 -side top -fill x -expand t -before [lindex [pack slaves %s] 0]", 
                   this->FileEntry->GetWidgetName(), 
                   this->FileEntry->GetParent()->GetWidgetName());
      }
    else
      {
      this->Script("pack %s -side top -fill x -expand t", 
                   this->FileEntry->GetWidgetName());
      }
    }

}

//----------------------------------------------------------------------------
int vtkPVReaderModule::CloneAndInitialize(int makeCurrent, 
                                          vtkPVReaderModule*& clone)
{
  clone = 0;

  vtkPVSource* pvs = 0;
  int retVal = 
    this->Superclass::CloneAndInitialize(makeCurrent, pvs);
  if (retVal == VTK_OK)
    {
    clone = vtkPVReaderModule::SafeDownCast(pvs);
    }
  return retVal;
}

//----------------------------------------------------------------------------
int vtkPVReaderModule::CanReadFile(const char* fname)
{
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  const char* ext = this->ExtractExtension(fname);
  int matches = 0;
  int canRead = 0;

  // Check if the file name matches any of our extensions.
  for(this->Iterator->GoToFirstItem();
      !this->Iterator->IsDoneWithTraversal() && !matches;
      this->Iterator->GoToNextItem())
    {
    const char* val = 0;
    this->Iterator->GetData(val);
    if(ext && strcmp(ext, val) == 0)
      {
      matches = 1;
      }
    }

  // If the extension matches, see if the reader can read the file.
  if(matches)
    {
    vtkClientServerStream stream;
    // Assume that it can read the file (based on extension match)
    // if CanReadFile does not exist.
    canRead = 1;
    vtkClientServerID tmpID = 
      pm->NewStreamObject(this->SourceClassName, stream);
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() << "SetReportInterpreterErrors" << 0
           << vtkClientServerStream::End;
    stream << vtkClientServerStream::Invoke
           << tmpID << "CanReadFile" << fname
           << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
    pm->GetLastResult(
      vtkProcessModule::DATA_SERVER_ROOT).GetArgument(0, 0, &canRead);
    pm->DeleteStreamObject(tmpID, stream);
    stream << vtkClientServerStream::Invoke
           << pm->GetProcessModuleID() << "SetReportInterpreterErrors" << 1
           << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
    }
  return canRead;
}

//----------------------------------------------------------------------------
void vtkPVReaderModule::AddExtension(const char* ext)
{
  this->Extensions->AppendItem(ext);
}

//----------------------------------------------------------------------------
const char* vtkPVReaderModule::RemovePath(const char* fname)
{
  const char* ptr = strrchr(fname, '/');
  if ( ptr )
    {
    if ( ptr[1] != '\0' )
      {
      return ptr+1;
      }
    else
      {
      return ptr;
      }
    }
  return  0;
}

//----------------------------------------------------------------------------
const char* vtkPVReaderModule::ExtractExtension(const char* fname)
{
  return strrchr(fname, '.');
}

//----------------------------------------------------------------------------
int vtkPVReaderModule::Initialize(const char*, vtkPVReaderModule*& clone)
{
  clone = 0;
  if (this->CloneAndInitialize(1, clone) != VTK_OK)
    {
    vtkErrorMacro("Error creating reader " << this->GetClassName()
                  << endl);
    clone = 0;
    return VTK_ERROR;
    }
  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkPVReaderModule::ReadFileInformation(const char* fname)
{
  this->SetReaderFileName(fname);
  
  const char* desc = this->RemovePath(fname);
  if (desc)
    {
    this->SetLabelNoTrace(desc);
    }

  // Update the reader's information.
  this->Proxy->UpdateInformation();

  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkPVReaderModule::FinalizeInternal(const char*, int accept)
{
  vtkPVWindow* window = this->GetPVApplication()->GetMainWindow();
  window->AddPVSource("Sources", this);
  this->Delete();

  if (this->GetTraceInitialized() == 0)
    { 
    this->SetTraceInitialized(1);
    }
  this->GrabFocus();
  if (accept)
    {
    this->GetPVWindow()->GetMainView()->UpdateNavigationWindow(this, 0);
    this->Accept(0);
    }
  else
    {
    this->GetPVWindow()->GetMainView()->UpdateNavigationWindow(this, 1);
    }
  return VTK_OK;
}

//----------------------------------------------------------------------------
int vtkPVReaderModule::Finalize(const char* fname)
{
  return this->FinalizeInternal(fname, 1);
}

//----------------------------------------------------------------------------
vtkIdType vtkPVReaderModule::GetNumberOfExtensions()
{
  return this->Extensions->GetNumberOfItems();
}

//----------------------------------------------------------------------------
const char* vtkPVReaderModule::GetExtension(vtkIdType i)
{
  const char* result = 0;
  if(this->Extensions->GetItem(i, result) != VTK_OK) { result = 0; }
  return result;
}

//----------------------------------------------------------------------------
void vtkPVReaderModule::SaveState(ofstream *file)
{
  if (this->VisitedFlag)
    {
    return;
    }
  
  *file << "set kw(" << this->GetTclName() << ") [$kw("
        << this->GetPVWindow()->GetTclName() << ") InitializeReadCustom \""
        << this->GetModuleName() << "\" \"" << this->FileEntry->GetValue() 
        << "\"]" << endl;
  *file << "$kw(" << this->GetPVWindow()->GetTclName() << ") "
        << "ReadFileInformation $kw(" << this->GetTclName() << ") \""
        << this->FileEntry->GetValue() << "\"" << endl;
  *file << "$kw(" << this->GetPVWindow()->GetTclName() << ") "
        << "FinalizeRead $kw(" << this->GetTclName() << ") \""
        << this->FileEntry->GetValue() << "\"" << endl;

  // Let the PVWidgets set up the object.
  vtkCollectionIterator *it = this->Widgets->NewIterator();
  it->InitTraversal();
  
  int numWidgets = this->Widgets->GetNumberOfItems();
  for (int i = 0; i < numWidgets; i++)
    {
    vtkPVWidget* widget = static_cast<vtkPVWidget*>(it->GetCurrentObject());
    widget->SaveState(file);
    it->GoToNextItem();
    }
  it->Delete();

  // Call accept.
  *file << "$kw(" << this->GetTclName() << ") AcceptCallback" << endl;

  this->VisitedFlag = 1;

  this->SaveStateDisplay(file);
}

//----------------------------------------------------------------------------
void vtkPVReaderModule::AddPVFileEntry(vtkPVFileEntry* fileEntry)
{
  vtkPVWidget* pvw;
  // How to add to the begining of a collection?
  // Just make a new one.
  vtkPVWidgetCollection *newWidgets = vtkPVWidgetCollection::New();
  newWidgets->AddItem(fileEntry);
  
  vtkCollectionIterator *it = this->Widgets->NewIterator();
  it->InitTraversal();
  while ( (pvw = static_cast<vtkPVWidget*>(it->GetCurrentObject())) )
    {
    newWidgets->AddItem(pvw);
    it->GoToNextItem();
    }
  this->Widgets->Delete();
  this->Widgets = newWidgets;
  it->Delete();
  
  // Just doing what is in vtkPVSource::AddPVWidget(pvw);
  char str[512];
  if (fileEntry->GetTraceName() == NULL)
    {
    vtkWarningMacro("TraceName not set. Widget class: " 
                    << fileEntry->GetClassName());
    return;
    }

  fileEntry->SetTraceReferenceObject(this);
  sprintf(str, "GetPVWidget {%s}", fileEntry->GetTraceName());
  fileEntry->SetTraceReferenceCommand(str);
  fileEntry->Select();
}

//----------------------------------------------------------------------------
void vtkPVReaderModule::SetReaderFileName(const char* fname)
{
  if (this->FileEntry)
    {
    vtkSMProperty *prop = this->FileEntry->GetSMProperty();
    this->FileEntry->SetValue(fname);
    vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
    // Since this is a reader, it is ok to assume that there is on
    // VTKSource. Hence, the use of index 0.
    if (prop)
      {
      vtkClientServerStream stream;
      stream << vtkClientServerStream::Invoke 
             << this->GetVTKSourceID(0) << prop->GetCommand() << fname
             << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
      }
    const char* ext = this->ExtractExtension(fname);
    if (ext)
      {
      this->FileEntry->SetExtension(ext+1);
      }
    }
}

//----------------------------------------------------------------------------
int vtkPVReaderModule::GetNumberOfTimeSteps()
{
  vtkPVWidget* pvWidget = this->GetTimeStepWidget();
  if ( !pvWidget)
    {
    return 0;
    }
  vtkPVFileEntry* pvFE = vtkPVFileEntry::SafeDownCast(pvWidget);
  if (pvFE)
    {
    return pvFE->GetNumberOfFiles();
    }
  vtkPVScale* pvScale = vtkPVScale::SafeDownCast(pvWidget);
  if (pvScale)
    {
    return static_cast<int>(pvScale->GetRangeMax() - pvScale->GetRangeMin());
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVReaderModule::SetRequestedTimeStep(int step)
{
  vtkPVWidget* pvWidget = this->GetTimeStepWidget();
  if ( !pvWidget)
    {
    return;
    }
  vtkPVFileEntry* pvFE = vtkPVFileEntry::SafeDownCast(pvWidget);
  vtkPVScale* pvScale = vtkPVScale::SafeDownCast(pvWidget);
  if (pvFE)
    {
    pvFE->SetTimeStep(step);
    }
  else if (pvScale)
    {
    pvScale->SetValue(step);
    }

  this->AcceptCallback();
  this->GetPVApplication()->GetMainView()->EventuallyRender();
  this->Script("update");
}

//----------------------------------------------------------------------------
vtkPVWidget* vtkPVReaderModule::GetTimeStepWidget()
{
  // 1) check if the FileEntry has KeepsTimeStep iVar set. If so, it's the one
  // that gives us the timesteps.
  if (this->FileEntry && this->FileEntry->GetKeepsTimeStep())
    {
    return this->FileEntry;
    }

  // 2) if not, search thru all the widgets and find one with KeepsTimeStep iVar set 
  // and use it to get the timesteps.
  if (this->Widgets)
    {
    vtkPVWidget* toReturn = NULL;
    vtkPVWidget* pvWidget;
    vtkCollectionIterator* it = this->Widgets->NewIterator();
    it->InitTraversal();

    int i;
    for (i=0; i < this->Widgets->GetNumberOfItems(); i++)
      {
      pvWidget = vtkPVWidget::SafeDownCast(it->GetCurrentObject());
      if (pvWidget && pvWidget->GetKeepsTimeStep())
        {
        toReturn = pvWidget;
        break;
        }
      it->GoToNextItem();
      }
    it->Delete();
    if (toReturn) 
      {
      return toReturn;
      }
    }

  // 3) if none such property is found, try to use FileEntry directly.
  if (this->FileEntry)
    {
    return this->FileEntry;
    }
  // Every attempt to determine the widget that controls the timestep has failed!
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVReaderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AcceptAfterRead: " << this->AcceptAfterRead << endl;
  os << indent << "PackFileEntry: " << this->PackFileEntry << endl;
  os << indent << "FileEntry: " << this->FileEntry << endl;
}
