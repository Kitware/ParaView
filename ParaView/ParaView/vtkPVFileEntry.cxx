/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVFileEntry.cxx
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
#include "vtkPVFileEntry.h"

#include "vtkArrayMap.txx"
#include "vtkKWDirectoryUtilities.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWScale.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationInterfaceEntry.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVReaderModule.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkString.h"
#include "vtkStringList.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVFileEntry);
vtkCxxRevisionMacro(vtkPVFileEntry, "1.52.2.2");

//----------------------------------------------------------------------------
vtkPVFileEntry::vtkPVFileEntry()
{
  this->LabelWidget = vtkKWLabel::New();
  this->Entry = vtkKWEntry::New();
  //this->Entry->PullDownOn();
  this->BrowseButton = vtkKWPushButton::New();
  this->Extension = NULL;
  this->SuppressReset = 1;
  this->InSetValue = 0;

  this->TimestepFrame = vtkKWFrame::New();
  this->Timestep = vtkKWScale::New();
  this->TimeStep = 0;
  this->Format = 0;
  this->Prefix = 0;
  this->Ext = 0;
  this->Path = 0;
  this->Range[0] = this->Range[1] = 0;
  
  this->LastAcceptedFileName = 0;
}

//----------------------------------------------------------------------------
vtkPVFileEntry::~vtkPVFileEntry()
{
  this->BrowseButton->Delete();
  this->BrowseButton = NULL;
  this->Entry->Delete();
  this->Entry = NULL;
  this->LabelWidget->Delete();
  this->LabelWidget = NULL;
  this->SetExtension(NULL);

  this->Timestep->Delete();
  this->TimestepFrame->Delete();
  this->SetFormat(0);
  this->SetPrefix(0);
  this->SetExt(0);
  this->SetPath(0);
  
  this->SetLastAcceptedFileName(0);
}

//----------------------------------------------------------------------------
void vtkPVFileEntry::SetLabel(const char* label)
{
  // For getting the widget in a script.
  this->LabelWidget->SetLabel(label);

  if (label && label[0] &&
      (this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName(label);
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }
}

//----------------------------------------------------------------------------
const char* vtkPVFileEntry::GetLabel()
{
  return this->LabelWidget->GetLabel();
}

void vtkPVFileEntry::SetBalloonHelpString(const char *str)
{

  // A little overkill.
  if (this->BalloonHelpString == NULL && str == NULL)
    {
    return;
    }

  // This check is needed to prevent errors when using
  // this->SetBalloonHelpString(this->BalloonHelpString)
  if (str != this->BalloonHelpString)
    {
    // Normal string stuff.
    if (this->BalloonHelpString)
      {
      delete [] this->BalloonHelpString;
      this->BalloonHelpString = NULL;
      }
    if (str != NULL)
      {
      this->BalloonHelpString = new char[strlen(str)+1];
      strcpy(this->BalloonHelpString, str);
      }
    }
  
  if ( this->Application && !this->BalloonHelpInitialized )
    {
    this->LabelWidget->SetBalloonHelpString(this->BalloonHelpString);
    this->Entry->SetBalloonHelpString(this->BalloonHelpString);
    this->BrowseButton->SetBalloonHelpString(this->BalloonHelpString);
    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVFileEntry::Create(vtkKWApplication *pvApp)
{
  const char* wname;
  
  if (this->Application)
    {
    vtkErrorMacro("FileEntry already created");
    return;
    }
  
  this->SetApplication(pvApp);

  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);
  vtkKWFrame* frame = vtkKWFrame::New();
  frame->SetParent(this);
  frame->Create(pvApp, 0);

  this->LabelWidget->SetParent(frame);
  this->Entry->SetParent(frame);
  this->BrowseButton->SetParent(frame);
  
  // Now a label
  this->LabelWidget->Create(pvApp, "-width 18 -justify right");
  this->Script("pack %s -side left", this->LabelWidget->GetWidgetName());
  
  // Now the entry
  this->Entry->Create(pvApp, "");
  this->Script("bind %s <KeyPress> {%s ModifiedCallback}",
               this->Entry->GetEntry()->GetWidgetName(), this->GetTclName());
  this->Entry->BindCommand(this, "EntryChangedCallback");
  // Change the order of the bindings so that the
  // modified command gets called after the entry changes.
  this->Script("bindtags %s [concat Entry [lreplace [bindtags %s] 1 1]]", 
               this->Entry->GetWidgetName(), this->Entry->GetWidgetName());
  this->Script("pack %s -side left -fill x -expand t",
               this->Entry->GetWidgetName());
  
  // Now the push button
  this->BrowseButton->Create(pvApp, "");
  this->BrowseButton->SetLabel("Browse");
  this->BrowseButton->SetCommand(this, "BrowseCallback");

  if (this->BalloonHelpString)
    {
    this->SetBalloonHelpString(this->BalloonHelpString);
    }
  this->Script("pack %s -side left", this->BrowseButton->GetWidgetName());
  this->Script("pack %s -fill both -expand 1", frame->GetWidgetName());
  frame->Delete();

  this->TimestepFrame->SetParent(this);
  this->TimestepFrame->Create(pvApp, 0);
  this->Timestep->SetParent(this->TimestepFrame);
  this->Timestep->Create(pvApp, 0);
  this->Script("pack %s -expand 1 -fill both", this->Timestep->GetWidgetName());
  this->Script("pack %s -side bottom -expand 1 -fill x", this->TimestepFrame->GetWidgetName());
  this->Script("pack forget %s", this->TimestepFrame->GetWidgetName());
  this->Timestep->DisplayLabel("Timestep");
  this->Timestep->DisplayRangeOn();
  this->Timestep->DisplayEntryAndLabelOnTopOff();
  this->Timestep->DisplayEntry();
  this->Timestep->SetEndCommand(this, "TimestepChangedCallback");
  this->Timestep->SetEntryCommand(this, "TimestepChangedCallback");
}


//----------------------------------------------------------------------------
void vtkPVFileEntry::EntryChangedCallback()
{
  const char* val = this->Entry->GetValue();
  this->SetValue(val);
}

//-----------------------------------------------------------------------------
void vtkPVFileEntry::SetTimeStep(int ts)
{
  if ( ts < this->Range[0] || ts > this->Range[1] )
    {
    return;
    }
  this->TimeStep = ts;
  if ( !(this->FileNameLength && this->Format && this->Path && this->Prefix && this->Ext) )
    {
    return;
    }
  char* name = new char [ this->FileNameLength ];
  sprintf(name, this->Format, this->Path, this->Prefix, ts, this->Ext);
  this->SetValue(name);
  delete [] name;
}

//----------------------------------------------------------------------------
void vtkPVFileEntry::TimestepChangedCallback()
{
  int ts = static_cast<int>(this->Timestep->GetValue());
  this->SetTimeStep(ts);
}

//----------------------------------------------------------------------------
void vtkPVFileEntry::BrowseCallback()
{
  ostrstream str;
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  vtkKWLoadSaveDialog* loadDialog = pm->NewLoadSaveDialog();
  const char* fname = this->Entry->GetValue();
  if (fname && fname[0])
    {
    char* path   = new char [ strlen(fname) + 1];
    vtkKWDirectoryUtilities::GetFilenamePath(fname, path);
    if (path[0])
      {
      loadDialog->SetLastPath( path );
      }
    delete[] path;
    }
  else
    {
    vtkPVApplication* pvApp = pm->GetPVApplication();
    if (pvApp)
      {
      vtkPVWindow* win = pvApp->GetMainWindow();
      if (win)
        {
        win->RetrieveLastPath(loadDialog, "OpenPath");
        }
      }
    }
  loadDialog->Create(this->GetPVApplication(), 0);
  loadDialog->SetTitle(this->GetLabel()?this->GetLabel():"Select File");
  if(this->Extension)
    {
    loadDialog->SetDefaultExtension(this->Extension);
    str << "{{} {." << this->Extension << "}} ";
    }
  str << "{{All files} {*.*}}" << ends;  
  loadDialog->SetFileTypes(str.str());
  str.rdbuf()->freeze(0);  
  if(loadDialog->Invoke())
    {
    this->Script("%s SetValue {%s}", this->GetTclName(),
                 loadDialog->GetFileName());
    }
  loadDialog->Delete();
}

//----------------------------------------------------------------------------
void vtkPVFileEntry::SetValue(const char* fileName)
{
  if ( this->InSetValue )
    {
    return;
    }

  const char *old;
  
  if (fileName == NULL || fileName[0] == 0)
    {
    return;
    }

  old = this->Entry->GetValue();
  if (strcmp(old, fileName) == 0)
    {
    return;
    }
  this->InSetValue = 1;

  this->Entry->SetValue(fileName); 

  int already_set = 0;
  int cc;

  if ( this->Prefix && this->Format && this->Path && this->Ext )
    {
    char* name = new char[ this->FileNameLength + 2 ];
    // Things are already set. Let us check if this file uses same pattern

    for ( cc = this->Range[0]; cc <= this->Range[1]; cc ++ )
      {
      sprintf(name, this->Format, this->Path, this->Prefix, cc, this->Ext);
      if ( vtkString::Equals(name, fileName) )
        {
        already_set = 1;
        break;
        }
      }
    delete [] name;
    }

  if ( already_set )
    {
    // Already set, so just return
    this->InSetValue = 0;
    this->ModifiedCallback();
    return;
    }
  this->Range[0] = this->Range[1] = 0;

  // Have to regenerate prefix, pattern...

  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  vtkStringList* files = vtkStringList::New();
  vtkStringList* dirs = vtkStringList::New();
  char* path   = new char [ strlen(fileName) + 1];
  char* file   = new char [ strlen(fileName) + 1];
  char* ext    = new char [ strlen(fileName) + 1];
  char* number = new char [ strlen(fileName) + 1];
  vtkKWDirectoryUtilities::GetFilenamePath(fileName, path);
  vtkKWDirectoryUtilities::GetFilenameName(fileName, file);
  vtkKWDirectoryUtilities::GetFilenameExtension(fileName, ext);

  int in_ext = 1;
  int in_num = 0;

  int h5Flag = 0;
  if (strcmp(ext, "h5") == 0)
    {
    h5Flag = 1;
    file[strlen(file)-1] = 'f';
    }

  int ncnt = 0;
  for ( cc = (int)(strlen(file))-1; cc >= 0; cc -- )
    {
    if ( file[cc] >= '0' && file[cc] <= '9' )
      {
      in_num = 1;
      number[ncnt] = file[cc];
      ncnt ++;
      }
    else if ( in_ext && file[cc] == '.' )
      {
      in_ext = 0;
      in_num = 1;
      ncnt = 0;
      }
    else if ( in_num )
      {
      break;
      }
    file[cc] = 0;
    }
  if ( path[0] && file[0] )
    {
    this->SetPath(path);
    this->SetPrefix(file);
    this->SetExt(ext);
    number[ncnt] = 0;
    for ( cc = 0; cc < ncnt/2; cc ++ )
      {
      char tmp = number[cc];
      number[cc] = number[ncnt-cc-1];
      number[ncnt-cc-1] = tmp;
      }
    char format[100];
    char secondformat[100];
    sprintf(format, "%%s/%%s%%0%dd.%%s", ncnt);
    sprintf(secondformat, "%%s/%%s%%d.%%s");
    this->Entry->DeleteAllValues();
    pm->GetDirectoryListing(path, dirs, files, "readable");
    int cnt = 0;
    for ( cc = 0; cc < files->GetLength(); cc ++ )
      {
      if ( vtkString::StartsWith(files->GetString(cc), file ) &&
        vtkString::EndsWith(files->GetString(cc), ext) )
        {
        cnt ++;
        }
      }
    int med = atoi(number);
    this->FileNameLength = (int)(strlen(fileName)) * 2;
    char* rfname = new char[ this->FileNameLength ];
    int min = med+cnt;
    int max = med-cnt;
    int foundone = 0;
    for ( cc = med-cnt; cc < med+cnt; cc ++ )
      {
      sprintf(rfname, format, path, file, cc, ext);
      if ( files->GetIndex(rfname+strlen(path)+1) >= 0 )
        {
        this->Entry->AddValue(rfname);
        //cout << "File: " << rfname << endl;
        if ( max < cc )
          {
          max = cc;
          }
        if ( min > cc )
          {
          min = cc;
          }
        foundone = 1;
        }
      else if ( foundone )
        {
        if ( min > max || med < min || med > max )
          {
          min = cc;
          }
        else
          {
          break;
          }
        }
      }
    foundone = 0;
    int smin = med+cnt;
    int smax = med-cnt;
    for ( cc = med-cnt; cc < med+cnt; cc ++ )
      {
      sprintf(rfname, secondformat, path, file, cc, ext);
      if ( files->GetIndex(rfname+strlen(path)+1) >= 0 )
        {
        this->Entry->AddValue(rfname);
        //cout << "File: " << rfname << endl;
        if ( smax < cc )
          {
          smax = cc;
          }
        if ( smin > cc )
          {
          smin = cc;
          }
        foundone = 1;
        }
      else if ( foundone )
        {
        if ( smin > smax || med < smin || med > smax )
          {
          smin = cc;
          }
        else
          {
          break;
          }
        }
      }
    delete [] rfname;
    // If second range is bigger than first range, use second format
    if ( (smax - smin) >= (max - min) )
      {
      this->SetFormat(secondformat);
      min = smin;
      max = smax;
      // cout << "Use second format" << endl;
      }
    else
      {
      this->SetFormat(format);
      // cout << "Use first format" << endl;
      }
    if ( this->Entry->GetNumberOfValues() > 1 )
      {
      this->Script("pack %s -side bottom -expand 1 -fill x", this->TimestepFrame->GetWidgetName());
      this->Timestep->SetRange(min, max);
      this->SetRange(min, max);
      }
    this->Timestep->SetValue(med);
    this->TimeStep = med;
    ostrstream str;
    str << "set " << this->GetPVSource()->GetVTKSourceTclName() << "_files {";
    char* name = new char [ this->FileNameLength ];
    for ( cc = min; cc <= max; cc ++ )
      {
      sprintf(name, this->Format, this->Path, this->Prefix, cc, this->Ext);
      if ( cc > min )
        {
        str << " ";
        }
      str << name;
      }
    delete [] name;
    str << "}" << ends;
    //cout << str.str() << endl;
    this->GetPVApplication()->GetProcessModule()->ServerScript(str.str());
    str.rdbuf()->freeze(0);
    }

  dirs->Delete();
  files->Delete();
  delete [] path;
  delete [] file;
  delete [] ext;
  delete [] number;
  
  this->InSetValue = 0;
  this->ModifiedCallback();
}

//---------------------------------------------------------------------------
void vtkPVFileEntry::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  // I assume the quotes are for eveluating an output tcl variable.
  *file << "$kw(" << this->GetTclName() << ") SetValue \""
        << this->GetValue() << "\"" << endl;
}


//----------------------------------------------------------------------------
void vtkPVFileEntry::AcceptInternal(const char* sourceTclName)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  const char* fname = this->Entry->GetValue();

  pvApp->GetProcessModule()->ServerScript(
    "%s Set%s {%s}", sourceTclName, this->VariableName, fname);
  this->SetLastAcceptedFileName(fname);  

  vtkPVReaderModule* rm = vtkPVReaderModule::SafeDownCast(this->PVSource);
  if (rm && fname && fname[0])
    {
    const char* desc = rm->RemovePath(fname);
    if (desc)
      {
      rm->SetLabelOnce(desc);
      }
    }

  this->ModifiedFlag = 0;
}


//----------------------------------------------------------------------------
void vtkPVFileEntry::ResetInternal()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

//  pvApp->Script("%s SetValue [%s Get%s]", this->Entry->GetTclName(),
//                sourceTclName, this->VariableName); 
  this->SetValue(this->LastAcceptedFileName);

  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
vtkPVFileEntry* vtkPVFileEntry::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVFileEntry::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVFileEntry::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVFileEntry* pvfe = vtkPVFileEntry::SafeDownCast(clone);
  if (pvfe)
    {
    pvfe->LabelWidget->SetLabel(this->LabelWidget->GetLabel());
    pvfe->SetExtension(this->GetExtension());
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVFileEntry.");
    }
}

//----------------------------------------------------------------------------
int vtkPVFileEntry::ReadXMLAttributes(vtkPVXMLElement* element,
                                      vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(label)
    {
    this->SetLabel(label);
    }
  else
    {
    this->SetLabel(this->VariableName);
    }
  
  // Setup the Extension.
  const char* extension = element->GetAttribute("extension");
  if(!extension)
    {
    vtkErrorMacro("No extension attribute.");
    return 0;
    }
  this->SetExtension(extension);
  
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkPVFileEntry::GetValue() 
{
  return this->Entry->GetValue();
}

//-----------------------------------------------------------------------------
void vtkPVFileEntry::SaveInBatchScriptForPart(ofstream* file, const char* sourceTclName)
{
  if (this->Range[0] < this->Range[1])
    {
    *file << "\tset " << sourceTclName << "_files {";
    *file << this->Script("concat $%s_files", sourceTclName);
    *file << "}" << endl;

    *file << "\t" << sourceTclName << " Set" << this->VariableName 
          << " [ lindex $" << sourceTclName << "_files " << this->TimeStep 
          << "]\n";
    }
  else
    {
    *file << "\t" << sourceTclName << " Set" << this->VariableName << " {" 
          << this->Entry->GetValue() << "}\n";
    }
}


//-----------------------------------------------------------------------------
void vtkPVFileEntry::AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                               vtkPVAnimationInterfaceEntry *ai)
{
  if ( !(this->FileNameLength && this->Format && this->Path && this->Prefix && this->Ext) )
    {
    return;
    }
  if ( (this->Range[1] - this->Range[0]) > 0 )
    {
    char methodAndArgs[500];

    sprintf(methodAndArgs, "AnimationMenuCallback %s", ai->GetTclName()); 
    menu->AddCommand(this->GetTraceName(), this, methodAndArgs, 0,"");
    }
}

//-----------------------------------------------------------------------------
void vtkPVFileEntry::AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai)
{
  char script[5000];

  if (ai->InitializeTrace(NULL))
    {
    this->AddTraceEntry("$kw(%s) AnimationMenuCallback $kw(%s)", 
      this->GetTclName(), ai->GetTclName());
    }

  sprintf(script, "%s SetFileName [ lindex $%s_files [expr round($pvTime)-%d] ]",
    this->GetPVSource()->GetVTKSourceTclName(),
    this->GetPVSource()->GetVTKSourceTclName(),
    this->Range[0]);
  ai->SetLabelAndScript(this->GetTraceName(), script);
  ai->SetTimeStart(this->Range[0]);

  //int ts = static_cast<int>(this->Timestep->GetValue());
  //ai->SetCurrentTime(ts);
  ai->SetTimeEnd(this->Range[1]);
  ai->SetTypeToInt();
  //cout << "Set time to: " << ai->GetTimeStart() << " - " << ai->GetTimeEnd() << endl;
  sprintf(script, "AnimationMenuCallback $kw(%s)", 
    ai->GetTclName());
  ai->SetSaveStateScript(script);
  ai->SetSaveStateObject(this);
  ai->Update();
}

//----------------------------------------------------------------------------
void vtkPVFileEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Extension: " << (this->Extension?this->Extension:"none") << endl;
  os << "Range: " << this->Range[0] << " " << this->Range[1] << endl;
}
