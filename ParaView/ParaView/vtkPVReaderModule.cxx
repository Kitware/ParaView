/*=========================================================================

  Program:   ParaView
  Module:    vtkPVReaderModule.cxx
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
#include "vtkPVReaderModule.h"

#include "vtkKWFrame.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVFileEntry.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkVector.txx"
#include "vtkVectorIterator.txx"
#include "vtkPVWidgetCollection.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVReaderModule);
vtkCxxRevisionMacro(vtkPVReaderModule, "1.29");

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
  this->FileEntry->SetParent(this->GetParameterFrame()->GetFrame());
  this->FileEntry->SetModifiedCommand(this->GetTclName(), 
                                      "SetAcceptButtonColorToRed");
  this->FileEntry->SetObjectVariable(this->GetVTKSourceTclName(), "FileName");
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
                                          vtkPVReaderModule*& clone )
{
  clone = 0;

  vtkPVSource* pvs = 0;
  int retVal = this->Superclass::CloneAndInitialize(makeCurrent, pvs);
  if (retVal == VTK_OK)
    {
    clone = vtkPVReaderModule::SafeDownCast(pvs);
    }
  return retVal;
}

//----------------------------------------------------------------------------
int vtkPVReaderModule::CanReadFile(const char* fname)
{
  const char* ext = this->ExtractExtension(fname);
  const char* val = 0;
  this->Iterator->GoToFirstItem();
  while(!this->Iterator->IsDoneWithTraversal())
    {
    this->Iterator->GetData(val);
    if (ext && strcmp(ext, val) == 0)
      {
      // Trouble reading PLOT3D files, so disable Canread feature,
      // and just go by extension.

      // The extension matches, see if the reader can read the file.
      //this->Script("%s vtkPVReaderModuleCanReadFileTemp",
      //             this->SourceClassName);

      // First see if the reader has a CanRead method.
      //this->Script("catch {vtkPVReaderModuleCanReadFileTemp CanReadFile {%s}}",
      //             fname);
      //int result = this->GetIntegerResult(this->Application);
      //if (result == 1)
      //  {
        // The reader does not have a CanReadFile method.
        // Assume it can read the file because it has the correct extension.
      //  return 1;
      //  }

      //this->Script("vtkPVReaderModuleCanReadFileTemp CanReadFile {%s}",
      //             fname);
      //result = this->GetIntegerResult(this->Application);
      //this->Script("vtkPVReaderModuleCanReadFileTemp Delete");
      //if(result)
      //  {
        return 1;
      //  }
      }
    this->Iterator->GoToNextItem();
    }
  return 0;
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
  if (this->FileEntry)
    {
    this->FileEntry->SetValue(fname);
    vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
    pm->ServerScript("%s Set%s {%s}", this->GetVTKSourceTclName(), 
                     this->FileEntry->GetVariableName(), fname);
    
    const char* ext = this->ExtractExtension(fname);
    if (ext)
      {
      this->FileEntry->SetExtension(ext+1);
      }
    }

  const char* desc = this->RemovePath(fname);
  if (desc)
    {
    this->SetLabelNoTrace(desc);
    }

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
  *file << "$kw(" << this->GetPVWindow()->GetTclName() << ") OpenCustom {" 
        << this->GetModuleName() << "} {" 
        << this->FileEntry->GetValue() << "}\n"; 
  *file << "set kw(" << this->GetTclName() << ") [$kw("
        << this->GetPVWindow()->GetTclName() << ") GetCurrentPVSource]\n";
  this->VisitedFlag = 1;
}

//----------------------------------------------------------------------------
void vtkPVReaderModule::AddPVFileEntry(vtkPVFileEntry* fileEntry)
{
  vtkPVWidget* pvw;
  // How to add to the begining of a collection?
  // Just make a new one.
  vtkPVWidgetCollection *newWidgets = vtkPVWidgetCollection::New();
  newWidgets->AddItem(fileEntry);
  this->Widgets->InitTraversal();
  while ( (pvw = this->Widgets->GetNextPVWidget()) )
    {
    newWidgets->AddItem(pvw);
    }
  this->Widgets->Delete();
  this->Widgets = newWidgets;

  // Just doing what is in vtkPVSource::AddPVWidget(pvw);
  char str[512];
  if (fileEntry->GetTraceName() == NULL)
    {
    vtkWarningMacro("TraceName not set. Widget class: " 
                    << pvw->GetClassName());
    return;
    }

  fileEntry->SetTraceReferenceObject(this);
  sprintf(str, "GetPVWidget {%s}", fileEntry->GetTraceName());
  fileEntry->SetTraceReferenceCommand(str);
  fileEntry->Select();
}

//----------------------------------------------------------------------------
void vtkPVReaderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AcceptAfterRead: " << this->AcceptAfterRead << endl;
  os << indent << "PackFileEntry: " << this->PackFileEntry << endl;
  os << indent << "FileEntry: " << this->FileEntry << endl;
}
