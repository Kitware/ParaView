/*=========================================================================

  Program:   ParaView
  Module:    vtkPVFileEntry.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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
#include "vtkPVFileEntryProperty.h"
#include "vtkPVProcessModule.h"
#include "vtkPVReaderModule.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkString.h"
#include "vtkStringList.h"
#include "vtkKWListSelectOrder.h"
#include "vtkCommand.h"
#include "vtkKWPopupButton.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWEvent.h"

#define MAX_FILES_ON_THE_LIST 100

//===========================================================================
//***************************************************************************
class vtkPVFileEntryObserver: public vtkCommand
{
public:
  static vtkPVFileEntryObserver *New() 
    {return new vtkPVFileEntryObserver;};

  vtkPVFileEntryObserver()
    {
    this->FileEntry= 0;
    }

  virtual void Execute(vtkObject* wdg, unsigned long event,  
    void* calldata)
    {
    if ( this->FileEntry)
      {
      this->FileEntry->ExecuteEvent(wdg, event, calldata);
      }
    }

  vtkPVFileEntry* FileEntry;
};

//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVFileEntry);
vtkCxxRevisionMacro(vtkPVFileEntry, "1.86.2.1");

//----------------------------------------------------------------------------
vtkPVFileEntry::vtkPVFileEntry()
{
  this->Observer = vtkPVFileEntryObserver::New();
  this->Observer->FileEntry = this;
  this->LabelWidget = vtkKWLabel::New();
  this->Entry = vtkKWEntry::New();
  //this->Entry->PullDownOn();
  this->BrowseButton = vtkKWPushButton::New();
  this->Extension = NULL;
  this->InSetValue = 0;

  this->TimestepFrame = vtkKWFrame::New();
  this->Timestep = vtkKWScale::New();
  this->TimeStep = 0;
  
  this->Property = NULL;

  this->Path = 0;

  this->FileListPopup = vtkKWPopupButton::New();

  this->FileListSelect = vtkKWListSelectOrder::New();
  this->ListObserverTag = 0;
  this->IgnoreFileListEvents = 0;

  this->Initialized = 0;
}

//----------------------------------------------------------------------------
vtkPVFileEntry::~vtkPVFileEntry()
{
  if ( this->ListObserverTag )
    {
    this->FileListSelect->RemoveObserver(this->ListObserverTag);
    }
  this->Observer->FileEntry = 0;
  this->Observer->Delete();
  this->Observer = 0;
  this->BrowseButton->Delete();
  this->BrowseButton = NULL;
  this->Entry->Delete();
  this->Entry = NULL;
  this->LabelWidget->Delete();
  this->LabelWidget = NULL;
  this->SetExtension(NULL);

  this->Timestep->Delete();
  this->TimestepFrame->Delete();
  this->FileListPopup->Delete();
  this->FileListPopup = 0;
  this->FileListSelect->Delete();
  this->FileListSelect = 0;
  
  this->SetProperty(NULL);

  this->SetPath(0);
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

//----------------------------------------------------------------------------
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
  
  if ( this->GetApplication() && !this->BalloonHelpInitialized )
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
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(pvApp, "frame", "-bd 0 -relief flat"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

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
               this->Entry->GetWidgetName(), this->GetTclName());
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

  this->FileListPopup->SetParent(frame);
  this->FileListPopup->Create(pvApp, 0);
  this->FileListPopup->SetLabel("Timesteps");
  this->FileListPopup->SetPopupTitle("Select Files For Time Series");
  this->FileListPopup->SetCommand(this, "UpdateAvailableFiles");

  this->FileListSelect->SetParent(this->FileListPopup->GetPopupFrame());
  this->FileListSelect->Create(pvApp, 0);
  this->Script("pack %s -fill both -expand 1", this->FileListSelect->GetWidgetName());
  this->Script("pack %s -fill x", this->FileListPopup->GetWidgetName());

  this->ListObserverTag = this->FileListSelect->AddObserver(
    vtkKWEvent::WidgetModifiedEvent, 
    this->Observer);
  frame->Delete();

  this->FileListSelect->SetEllipsisCommand(this, "UpdateAvailableFiles 1");
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
  if ( ts >= this->Property->GetNumberOfFiles() && ts < 0 )
    {
    return;
    }
  this->TimeStep = ts;
  const char* fname = this->Property->GetFile(this->TimeStep);
  if ( fname )
    {
    if ( fname[0] == '/' || 
      (fname[1] == ':' && (fname[2] == '/' || fname[2] == '\\')) ||
      (fname[0] == '\\' && fname[1] == '\\') ||
      !this->Path || !*this->Path)
      {
      this->SetValue(fname);
      }
    else
      {
      ostrstream str;
      str << this->Path << "/" << fname << ends;
      this->SetValue(str.str());
      str.rdbuf()->freeze(0);
      }
    }
  this->Timestep->SetValue(this->TimeStep);
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
  vtkKWLoadSaveDialog* loadDialog = this->GetPVApplication()->NewLoadSaveDialog();
  const char* fname = this->Entry->GetValue();

  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVWindow* win = 0;
  if (pvApp)
    {
    win = pvApp->GetMainWindow();
    }
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
    if (win)
      {
      win->RetrieveLastPath(loadDialog, "OpenPath");
      }
    }
  loadDialog->Create(this->GetPVApplication(), 0);
  if (win) 
    { 
    loadDialog->SetParent(this); 
    }
  loadDialog->SetTitle(this->GetLabel()?this->GetLabel():"Select File");
  if(this->Extension)
    {
    loadDialog->SetDefaultExtension(this->Extension);
    str << "{{} {." << this->Extension << "}} ";
    }
  str << "{{All files} {*}}" << ends;  
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

  char* prefix = 0;
  char* format = 0;
  already_set = this->Property->GetNumberOfFiles();

  char* path   = new char [ strlen(fileName) + 1];
  vtkKWDirectoryUtilities::GetFilenamePath(fileName, path);

  if ( !this->Path || strcmp(this->Path, path) != 0 )
    {
    already_set = 0;
    this->SetPath(path);
    }

  if ( already_set )
    {
    // Already set, so just return
    this->InSetValue = 0;
    this->ModifiedCallback();
    delete [] path;
    return;
    }

  this->IgnoreFileListEvents = 1;

  this->FileListSelect->RemoveItemsFromFinalList();

  // Have to regenerate prefix, pattern...

  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  vtkStringList* files = vtkStringList::New();
  char* file   = new char [ strlen(fileName) + 1];
  char* ext    = new char [ strlen(fileName) + 1];
  char* number = new char [ strlen(fileName) + 1];
  vtkKWDirectoryUtilities::GetFilenameName(fileName, file);
  vtkKWDirectoryUtilities::GetFilenameExtension(fileName, ext);

  int in_ext = 1;
  int in_num = 0;

  int fnameLength = 0;

  if (strcmp(ext, "h5") == 0)
    {
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

  if ( path[0] )
    {
    prefix = file;
    number[ncnt] = 0;
    for ( cc = 0; cc < ncnt/2; cc ++ )
      {
      char tmp = number[cc];
      number[cc] = number[ncnt-cc-1];
      number[ncnt-cc-1] = tmp;
      }
    char firstformat[100];
    char secondformat[100];
    sprintf(firstformat, "%%s/%%s%%0%dd.%%s", ncnt);
    sprintf(secondformat, "%%s/%%s%%d.%%s");
    this->Entry->DeleteAllValues();
    pm->GetDirectoryListing(path, 0, files, 0);
    int cnt = 0;
    for ( cc = 0; cc < files->GetLength(); cc ++ )
      {
      if ( files->GetLength() < MAX_FILES_ON_THE_LIST)
        {
        this->FileListSelect->AddSourceElement(files->GetString(cc));
        }
      if ( vtkString::StartsWith(files->GetString(cc), file ) &&
        vtkString::EndsWith(files->GetString(cc), ext) )
        {
        cnt ++;
        }
      }
    int med = atoi(number);
    fnameLength = (int)(strlen(fileName)) * 2;
    char* rfname = new char[ fnameLength ];
    int min = med+cnt;
    int max = med-cnt;
    int foundone = 0;
    for ( cc = med-cnt; cc < med+cnt; cc ++ )
      {
      sprintf(rfname, firstformat, path, file, cc, ext);
      if ( files->GetIndex(rfname+strlen(path)+1) >= 0 )
        {
        this->Entry->AddValue(rfname);
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
      format = secondformat;
      min = smin;
      max = smax;
      }
    else
      {
      format = firstformat;
      }
    if ( max - min < MAX_FILES_ON_THE_LIST )
      {
      char* name = new char [ fnameLength ];
      char* shname = new char [ fnameLength ];
      for ( cc = min; cc <= max; cc ++ )
        {
        sprintf(name, format, path, prefix, cc, ext);
        vtkKWDirectoryUtilities::GetFilenameName(name, shname);
        if ( files->GetIndex(shname) >= 0 )
          {
          this->FileListSelect->AddFinalElement(shname, 1);
          }
        }
      delete [] name;
      delete [] shname;
      }
    }

  if ( !this->FileListSelect->GetNumberOfElementsOnFinalList() )
    {
    vtkKWDirectoryUtilities::GetFilenameName(fileName, file);
    this->FileListSelect->AddFinalElement(file, 1);
    }

  if ( !this->Initialized )
    {
    this->Property->RemoveAllFiles();
    int kk;
    for ( kk = 0; kk < this->FileListSelect->GetNumberOfElementsOnFinalList(); kk ++ )
      {
      this->Property->AddFile(this->FileListSelect->GetElementFromFinalList(kk));
      }
    char* cfile = new char[ strlen(fileName) + 1];
    vtkKWDirectoryUtilities::GetFilenamePath(fileName, cfile);
    this->Property->SetDirectoryName(cfile);
    vtkKWDirectoryUtilities::GetFilenameName(fileName, cfile);
    for ( cc = 0; cc < this->Property->GetNumberOfFiles(); cc ++ )
      {

      if ( strcmp(cfile, this->Property->GetFile(cc)) == 0 )
        {
        this->Property->SetTimeStep(cc);
        this->SetTimeStep(cc);
        break;
        }
      }
    delete [] cfile;
    this->Initialized = 1;
    }

  files->Delete();
  delete [] path;
  delete [] file;
  delete [] ext;
  delete [] number;

  this->UpdateTimeStep();

  this->IgnoreFileListEvents = 0;
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
void vtkPVFileEntry::AcceptInternal(vtkClientServerID sourceID)
{
  const char* fname = this->Entry->GetValue();
  char *cmd = new char[strlen(this->VariableName)+4];
  sprintf(cmd, "Set%s", this->VariableName);
  
  this->Property->SetString(fname);
  this->Property->SetVTKSourceID(sourceID);
  this->Property->SetTimeStep(this->TimeStep);
  this->Property->SetVTKCommand(cmd);

  delete[] cmd;
  
  vtkPVReaderModule* rm = vtkPVReaderModule::SafeDownCast(this->PVSource);
  if (rm && fname && fname[0])
    {
    const char* desc = rm->RemovePath(fname);
    if (desc)
      {
      rm->SetLabelOnce(desc);
      }
    }
  this->Property->RemoveAllFiles();
  int cc;
  for ( cc = 0; cc < this->FileListSelect->GetNumberOfElementsOnFinalList(); cc ++ )
    {
    this->Property->AddFile(this->FileListSelect->GetElementFromFinalList(cc));
    }

  if ( this->Property->GetNumberOfFiles() > 0 )
    {
    this->WidgetRange[0] = 0;
    this->WidgetRange[1] = this->FileListSelect->GetNumberOfElementsOnFinalList();
    this->UseWidgetRange = 1;
    }
  
  this->Property->AcceptInternal();
  this->UpdateAvailableFiles();
  
  this->ModifiedFlag = 0;
}


//----------------------------------------------------------------------------
void vtkPVFileEntry::ResetInternal()
{
  this->SetValue(this->Property->GetString());
  this->SetTimeStep(this->Property->GetTimeStep());

  this->IgnoreFileListEvents = 1;
  this->FileListSelect->RemoveItemsFromFinalList();
  int cc;
  for ( cc = 0; cc < this->Property->GetNumberOfFiles(); cc ++ )
    {
    this->FileListSelect->AddFinalElement(this->Property->GetFile(cc), 1);
    }
  const char* fileName = this->Entry->GetValue();
  if ( fileName && fileName[0] )
    {
    char *file = new char[ strlen(fileName) + 1 ];
    vtkKWDirectoryUtilities::GetFilenameName(fileName, file);
    this->FileListSelect->AddFinalElement(file, 1);
    delete [] file;
    }

  this->IgnoreFileListEvents = 0;

  this->UpdateAvailableFiles();

  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
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
int vtkPVFileEntry::GetNumberOfFiles()
{
  if ( !this->Property )
    {
    return 0;
    }
  return this->Property->GetNumberOfFiles();
}

//-----------------------------------------------------------------------------
void vtkPVFileEntry::SaveInBatchScript(ofstream* file)
{
  if ( this->Property->GetNumberOfFiles() > 1 )
    {
    *file << "set " 
          << "pvTemp" << this->PVSource->GetVTKSourceID(0) << "_files {";
    int cc;
    for ( cc = 0; cc < this->Property->GetNumberOfFiles(); cc ++ )
      {
      *file << "\"" << this->Property->GetDirectoryName() << "/" << this->Property->GetFile(cc) << "\" ";
      }
    *file << "}" << endl;

    *file << "  [$pvTemp" << this->PVSource->GetVTKSourceID(0) 
          <<  " GetProperty " << this->VariableName << "] SetElement 0 "
          << " [ lindex $" << "pvTemp" << this->PVSource->GetVTKSourceID(0)  
          << "_files " << this->TimeStep << "]" << endl;

    }
  else
    {
    *file << "  [$pvTemp" << this->PVSource->GetVTKSourceID(0) 
          <<  " GetProperty " << this->VariableName << "] SetElement 0 {"
          << this->Entry->GetValue() << "}" << endl;
    }
}


//-----------------------------------------------------------------------------
void vtkPVFileEntry::AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                               vtkPVAnimationInterfaceEntry *ai)
{
  if ( this->Property->GetNumberOfFiles() > 0 )
    {
    char methodAndArgs[500];

    sprintf(methodAndArgs, "AnimationMenuCallback %s", ai->GetTclName()); 
    menu->AddCommand(this->GetTraceName(), this, methodAndArgs, 0,"");
    }
}

//-----------------------------------------------------------------------------
void vtkPVFileEntry::AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai)
{
  if (ai->InitializeTrace(NULL))
    {
    this->AddTraceEntry("$kw(%s) AnimationMenuCallback $kw(%s)", 
      this->GetTclName(), ai->GetTclName());
    }

  this->Superclass::AnimationMenuCallback(ai);

  ai->SetLabelAndScript(this->GetTraceName(), NULL, this->GetTraceName());
  ai->SetCurrentProperty(this->Property);
  ai->SetTimeStart(0);
  ai->SetTimeEnd(this->Property->GetNumberOfFiles()-1);
  ai->SetTypeToInt();
  ai->Update();
}

//----------------------------------------------------------------------------
void vtkPVFileEntry::SetProperty(vtkPVWidgetProperty *prop)
{
  this->Property = vtkPVFileEntryProperty::SafeDownCast(prop);
  if (this->Property)
    {
    char *cmd = new char[strlen(this->VariableName)+4];
    sprintf(cmd, "Set%s", this->VariableName);
    this->Property->SetVTKCommand(cmd);
    delete[] cmd;
    }
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVFileEntry::GetProperty()
{
  return this->Property;
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVFileEntry::CreateAppropriateProperty()
{
  return vtkPVFileEntryProperty::New();
}

//----------------------------------------------------------------------------
void vtkPVFileEntry::ExecuteEvent(vtkObject*, unsigned long event, void*)
{
  if ( event == vtkKWEvent::WidgetModifiedEvent && !this->IgnoreFileListEvents )
    {
    this->UpdateTimeStep();
    this->ModifiedCallback();
    }
}

//----------------------------------------------------------------------------
void vtkPVFileEntry::UpdateTimeStep()
{
  const char* fileName = this->Entry->GetValue();
  if ( !fileName || !fileName[0] )
    {
    return;
    }

  this->IgnoreFileListEvents = 1;
  char* file = new char[ strlen(fileName) + 1 ];
  vtkKWDirectoryUtilities::GetFilenameName(fileName, file);
  this->FileListSelect->AddFinalElement(file, 1);
  this->TimeStep = this->FileListSelect->GetElementIndexFromFinalList(file);
  if ( this->TimeStep < 0 )
    {
    cout << "This should not have happended" << endl;
    cout << "Cannot find \"" << file << "\" on the list" << endl;
    int cc;
    for ( cc = 0; cc < this->FileListSelect->GetNumberOfElementsOnFinalList(); cc ++ )
      {
      cout << "Element: " << this->FileListSelect->GetElementFromFinalList(cc) << endl;
      }
    vtkPVApplication::Abort();
    }
  delete [] file;
  this->Timestep->SetValue(this->TimeStep);
  if ( this->FileListSelect->GetNumberOfElementsOnFinalList() > 1 )
    {
    this->Script("pack %s -side bottom -expand 1 -fill x", 
      this->TimestepFrame->GetWidgetName());
    this->Timestep->SetRange(0, 
      this->FileListSelect->GetNumberOfElementsOnFinalList()-1);
    }
  else
    {
    this->Script("pack forget %s", 
      this->TimestepFrame->GetWidgetName());
    }
  this->IgnoreFileListEvents = 0;
}

//----------------------------------------------------------------------------
void vtkPVFileEntry::UpdateAvailableFiles( int force )
{
  if ( !this->Path )
    {
    return;
    }

  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  vtkStringList* files = vtkStringList::New();
  pm->GetDirectoryListing(this->Path, 0, files, 0);

  if ( files->GetLength() < MAX_FILES_ON_THE_LIST || force )
    {
    this->IgnoreFileListEvents = 1;
    this->FileListSelect->RemoveItemsFromSourceList();
    int cc;
    for ( cc = 0; cc < files->GetLength(); cc ++ )
      {
      this->FileListSelect->AddSourceElement(files->GetString(cc));
      }
    this->IgnoreFileListEvents = 0;
    }
  files->Delete();
  this->UpdateTimeStep();
}

//-----------------------------------------------------------------------------
void vtkPVFileEntry::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->LabelWidget);
  this->PropagateEnableState(this->BrowseButton);
  this->PropagateEnableState(this->Entry);
  this->PropagateEnableState(this->TimestepFrame);
  this->PropagateEnableState(this->Timestep);
  this->PropagateEnableState(this->FileListSelect);
  this->PropagateEnableState(this->FileListPopup);
}

//----------------------------------------------------------------------------
void vtkPVFileEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Extension: " << (this->Extension?this->Extension:"none") << endl;
}
