/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLookmark.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkPVLookmark.h"

#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWIcon.h"
#include "vtkKWTclInteractor.h"
#include "vtkPVLookmarkManager.h"
#include "vtkPVRenderView.h"
#include "vtkPVReaderModule.h"
#include "vtkBase64Utilities.h"
#include "vtkCollectionIterator.h"
#include "vtkKWLabel.h"
#include "vtkWindowToImageFilter.h"
#include "vtkImageData.h"
#include "vtkImageResample.h"
#include "vtkImageClip.h"
#include "vtkPVApplication.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSMProxy.h"
#include "vtkPVArraySelection.h"
#include "vtkCamera.h"
#include "vtkPVScale.h"
#include "vtkPVLabeledToggle.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVFileEntry.h"
#include "vtkPVTraceHelper.h"
#include "vtkPVWidgetCollection.h"
#include "vtkSMDisplayProxy.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVSelectionList.h"
#include "vtkPVSelectTimeSet.h"
#include "vtkPVStringEntry.h"
#include "vtkPVSelectWidget.h"
#include "vtkPVMinMax.h"
#ifdef PARAVIEW_USE_EXODUS
#include "vtkPVBasicDSPFilterWidget.h"
#endif
#include "vtkKWFrameWithLabel.h"
#include "vtkPVInteractorStyleCenterOfRotation.h"
#include "vtkKWText.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkKWPushButton.h"
#include "vtkKWToolbar.h"
#include "vtkKWFrame.h"
#include "vtkImageReader2.h"
#include "vtkKWEvent.h"
#include "vtkCommand.h"
#include <vtkstd/map>
#include <vtkstd/string>
#include "vtkStdString.h"
#include "vtkKWMenuButton.h"
#include "vtkPVVolumeAppearanceEditor.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVLookmark );
vtkCxxRevisionMacro(vtkPVLookmark, "1.33");


//*****************************************************************************
class vtkPVLookmarkObserver : public vtkCommand
{
public:
  static vtkPVLookmarkObserver* New()
    {
    return new vtkPVLookmarkObserver;
    }
  void SetLookmark(vtkPVLookmark* lmk)
    {
    this->Lookmark = lmk;
    }
  virtual void Execute(vtkObject* obj, unsigned long event, void* calldata)
    {
    if (this->Lookmark)
      {
      this->Lookmark->ExecuteEvent(obj, event, calldata);
      }
    }
protected:
  vtkPVLookmarkObserver()
    {
    this->Lookmark = 0;
    }
  vtkPVLookmark* Lookmark;
};


//----------------------------------------------------------------------------
vtkPVLookmark::vtkPVLookmark()
{
  this->ImageData = NULL;
  this->StateScript= NULL;
  this->CenterOfRotation = new float[3];
  this->CenterOfRotation[0] = this->CenterOfRotation[1] = this->CenterOfRotation[2] = 0;
  this->Sources = vtkPVSourceCollection::New();
  this->TraceHelper = vtkPVTraceHelper::New();
  this->TraceHelper->SetObject(this);
  this->ToolbarButton = 0;
  this->Observer = vtkPVLookmarkObserver::New();
  this->Observer->SetLookmark(this);
  this->ErrorEventTag = 0;
}

//----------------------------------------------------------------------------
vtkPVLookmark::~vtkPVLookmark()
{
  this->TraceHelper->Delete();
  this->TraceHelper = 0;

  this->Observer->Delete();

  if(this->CenterOfRotation)
    {
    delete [] this->CenterOfRotation;
    this->CenterOfRotation = NULL;
    }
  if(this->StateScript)
    {
    delete [] this->StateScript;
    this->StateScript = NULL;
    }
  if(this->ImageData)
    {
    delete [] this->ImageData;
    this->ImageData = NULL;
    }
  if(this->Sources)
    {
    this->Sources->Delete();
    this->Sources = 0;
    }
  if(this->ToolbarButton)
    {
    this->ToolbarButton->Delete();
    this->ToolbarButton = 0;
    }
}


//-----------------------------------------------------------------------------
void vtkPVLookmark::ExecuteEvent(vtkObject* vtkNotUsed(obj), unsigned long event, void* vtkNotUsed(callData))
{
  if ( event == vtkKWEvent::ErrorMessageEvent || event == vtkKWEvent::WarningMessageEvent )
    {
    this->GetPVApplication()->GetMainWindow()->ShowErrorLog();
    }
}


//----------------------------------------------------------------------------
void vtkPVLookmark::View()
{
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();
  vtkPVSource *pvs;

  this->UnsetLookmarkIconCommand();

  //try to delete preexisting filters belonging to this lookmark - for when lmk has been visited before in this session
  this->DeletePVSources();

  // this prevents other filters' visibility from disturbing the lookmark view
  this->TurnFiltersOff();

//ds
  // for every dataset or source belonging to the lookmark,
  //  If the dataset is currently loaded, do nothing
  //  if the dataset is not loaded but exists on disk at the stored path load the dataset into paraview without prompting the user OR it is a source create it
  //  if the dataset is not loaded and does not exist on disk at the stored path - prompt the user for dataset, give the user the option to cancel the operation or continue visiting lookmark without this dataset, also ask if they want to update the path for this dataset

  char *temp_script = new char[strlen(this->StateScript)+1];
  strcpy(temp_script,this->GetStateScript());

  this->GetTraceHelper()->AddEntry("$kw(%s) View",
                      this->GetTclName());

  this->ParseAndExecuteStateScript(temp_script,0);

  // this is needed to update the eyeballs based on recent changes to visibility of filters
  // handle case where there is no source in the source window
  pvs = win->GetCurrentPVSource();
  if(pvs && pvs->GetNotebook())
    this->GetPVRenderView()->UpdateNavigationWindow(pvs,0);

  this->SetLookmarkIconCommand();
//  win->SetCenterOfRotation(this->CenterOfRotation);
  delete [] temp_script;
}

//----------------------------------------------------------------------------
vtkPVSource* vtkPVLookmark::GetSourceForMacro(vtkPVSourceCollection *sources,char *name)
{
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();
  vtkPVSource *pvs1;
  vtkPVSource *pvs2;
  vtkPVSource *source = NULL;
  int i = 0;
  char mesg[400];

  // check if this lookmark has a single source of the same module type first
  // FIXME: better test for number of source items
  while(this->DatasetList[i]){ i++; }
  if(i==1)
    {
    pvs1 = win->GetCurrentPVSource();
    while((pvs2 = pvs1->GetPVInput(0)))
      {
      pvs1 = pvs2;
      }
    return pvs1;
    }

  // add all the sources into collection
//  vtkPVSourceCollection *choices = vtkPVSourceCollection::New();
  vtkCollectionIterator *itChoices;
/*
  vtkPVSourceCollection *col = this->GetPVApplication()->GetMainWindow()->GetSourceList("Sources");
  if (col == NULL)
    {
    return 0;
    }
  vtkCollectionIterator *it = col->NewIterator();
  it->InitTraversal();
  while ( !it->IsDoneWithTraversal() )
    {
    pvs1 = static_cast<vtkPVSource*>( it->GetCurrentObject() );
    if( !pvs1->IsA("vtkPVReaderModule") )
      {
      choices->AddItem(pvs1);
      }
    it->GoToNextItem();
    } 
  it->Delete();
*/
  // ask user to specify
  vtkKWMessageDialog *dialog = vtkKWMessageDialog::New();
  dialog->SetMasterWindow(win);
  dialog->SetOptions(
    vtkKWMessageDialog::WarningIcon | vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault );
  dialog->SetModal(0);
  dialog->SetStyleToOkCancel();
  dialog->Create(this->GetPVApplication());
  vtkKWMenuButton *menu = vtkKWMenuButton::New();
  menu->SetParent(dialog->GetBottomFrame());
  menu->Create(this->GetPVApplication());
  this->Script("pack %s",menu->GetWidgetName());
//  itChoices = choices->NewIterator();
  itChoices = sources->NewIterator();
  itChoices->InitTraversal();
  char *defaultValue = NULL;
  while(!itChoices->IsDoneWithTraversal())
    {
    pvs2 = static_cast<vtkPVSource*>( itChoices->GetCurrentObject() );
    menu->AddRadioButton(pvs2->GetModuleName());
    if(!strcmp(name,pvs2->GetModuleName()))
      {
      defaultValue = name;
      }
    itChoices->GoToNextItem();
    }
  if(defaultValue)
    {
    menu->SetValue(defaultValue);
    }
  else if(pvs2)
    {
    menu->SetValue(pvs2->GetModuleName());
    }
  sprintf(mesg,"Multiple open sources match the data type of the file path \"%s\" stored with this lookmark. Please select which source to use, then press OK.",name);
  dialog->SetText( mesg );
  dialog->SetTitle( "Multiple Matching Sources" );
  dialog->SetIcon();
  dialog->BeepOn();
  if(dialog->Invoke())
    {
    itChoices->InitTraversal();
    while(!itChoices->IsDoneWithTraversal())
      {
      pvs2 = static_cast<vtkPVSource*>( itChoices->GetCurrentObject() );
      if(!strcmp(menu->GetValue(),pvs2->GetModuleName()))
        {
        source = pvs2;
        break;
        }
      itChoices->GoToNextItem();
      }
    }
  dialog->Delete();
//  it->Delete();
//  choices->Delete();
  itChoices->Delete();

  return source;
}

//----------------------------------------------------------------------------
vtkPVSource* vtkPVLookmark::GetSourceForLookmark(vtkPVSourceCollection *sources,char *name)
{
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();
  vtkPVSource *pvs1;
  vtkPVSource *source = NULL;

  // If there is an open source that matches this one, use it, otherwise, create a new one
/*
  vtkPVSourceCollection *col = this->GetPVApplication()->GetMainWindow()->GetSourceList("Sources");
  if (col == NULL)
    {
    return 0;
    }
  vtkCollectionIterator *it = col->NewIterator();
*/
  vtkCollectionIterator *it = sources->NewIterator();
  it->InitTraversal();
  while ( !it->IsDoneWithTraversal() )
    {
    pvs1 = static_cast<vtkPVSource*>( it->GetCurrentObject() );
    if( !pvs1->IsA("vtkPVReaderModule") && !strcmp(pvs1->GetModuleName(),name))
      {
      source = pvs1;
      break;
      }
    it->GoToNextItem();
    } 
  it->Delete();

  if(!source)
    {
    win->CreatePVSource(name,"Sources",1,1);
    source = win->GetCurrentPVSource();
    source->AcceptCallback();
    }

  return source;
}


//----------------------------------------------------------------------------
vtkPVSource* vtkPVLookmark::GetReaderForMacro(vtkPVSourceCollection *readers,char *moduleName, char *name)
{
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();
  vtkPVSource *pvs;
  vtkPVSource *pvs1;
  vtkPVSource *pvs2;
  vtkPVSource *source = NULL;
  vtkPVReaderModule *mod;
  const char *ptr1;
  const char *ptr2;
  char mesg[400];
  char *defaultValue = NULL;

  // check if this lookmark has a single source  first
  // if so, use the currently viewed reader
  int i = 0;
  while(this->DatasetList[i]){ i++; }
  if(i==1)
    {
    pvs1 = win->GetCurrentPVSource();
    while((pvs2 = pvs1->GetPVInput(0)))
      {
      pvs1 = pvs2;
      }
//    if(!strcmp(pvs1->GetModuleName(),moduleName))
//      {
    if(pvs1->IsA("vtkPVReaderModule"))
      {
      return pvs1;
      }
    else
      {
      vtkKWMessageDialog::PopupMessage(
        this->GetPVApplication(), win, "Attempt to use a macro created from a reader on a source", 
        "Please select a reader or one of a reader's filters in the Selection Window and try again.", 
        vtkKWMessageDialog::ErrorIcon);
      return 0;
      }
//      }
    }

  // If there is an open reader of the same type and filename, use it, 
  // 
/*
  vtkPVSourceCollection *choices = vtkPVSourceCollection::New();
  vtkPVSourceCollection *col = this->GetPVApplication()->GetMainWindow()->GetSourceList("Sources");
  if (col == NULL)
    {
    source = NULL;
    }
  vtkCollectionIterator *itOuter = col->NewIterator();
  vtkCollectionIterator *itInner = col->NewIterator();
  vtkCollectionIterator *itChoices = NULL;
  itOuter->InitTraversal();
*/

  // This lookmark is made up of multiple readers
  // compare each reader
/*
  vtkCollectionIterator *itOuter = readers->NewIterator();
  vtkCollectionIterator *itInner = readers->NewIterator();
  itOuter->InitTraversal();
  while ( !itOuter->IsDoneWithTraversal() )
    {
    pvs1 = static_cast<vtkPVSource*>( itOuter->GetCurrentObject() );
    pvs1->SetVisibility(0);
    //if(!pvs1->GetPVInput(0))  && !strcmp(pvs1->GetModuleName(),moduleName))
    //  {
    // search sources again and if this is the only one of this type, use it
    // if find a multiple, check its filename for a match, if so use it
    // else, populate a kwoptionmenu in a popup dialog and prompt user, use their selection

    mod = vtkPVReaderModule::SafeDownCast(pvs1);
    ptr1 = mod->RemovePath(mod->GetFileEntry()->GetValue());
    ptr2 = mod->RemovePath(name);
    if(!strcmp(ptr1,ptr2))
      {
      source = pvs1;
      }

    if(!source)
      {
//        choices->AddItem(pvs1);
      itInner->InitTraversal();
      while ( !itInner->IsDoneWithTraversal() )
        {
        pvs2 = static_cast<vtkPVSource*>( itInner->GetCurrentObject() );
        if(pvs2 != pvs1) // && !strcmp(pvs2->GetModuleName(),moduleName))
          {
          mod = vtkPVReaderModule::SafeDownCast(pvs2);
          if(mod)
            {
            ptr1 = mod->RemovePath(mod->GetFileEntry()->GetValue());
            ptr2 = mod->RemovePath(name);
            // if this source has a matching filename, use it
            if(!strcmp(ptr1,ptr2))
              {
              source = pvs2;
              }
            }
          if(!source)
            {
//             choices->AddItem(pvs2);
            }
          }
        itInner->GoToNextItem();
        }

      if(!source) // && choices->GetNumberOfItems()>1)
        {
*/
  vtkCollectionIterator *itChoices = readers->NewIterator();
  // found multiple and none of their filenames match the given one
  vtkKWMessageDialog *dialog = vtkKWMessageDialog::New();
  dialog->SetMasterWindow(win);
  dialog->SetOptions(
    vtkKWMessageDialog::WarningIcon | vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault );
  dialog->SetModal(0);
  dialog->SetStyleToOkCancel();
  dialog->Create(this->GetPVApplication());
  vtkKWMenuButton *menu = vtkKWMenuButton::New();
  menu->SetParent(dialog->GetBottomFrame());
  menu->Create(this->GetPVApplication());
  this->Script("pack %s",menu->GetWidgetName());
  itChoices->InitTraversal();
  while(!itChoices->IsDoneWithTraversal())
    {
    pvs = static_cast<vtkPVSource*>( itChoices->GetCurrentObject() );
    mod = vtkPVReaderModule::SafeDownCast(pvs);
    menu->AddRadioButton(mod->RemovePath(mod->GetFileEntry()->GetValue()));
    if(!strcmp(name,mod->RemovePath(mod->GetFileEntry()->GetValue())))
      {
      defaultValue = name;
      }
    itChoices->GoToNextItem();
    }
  // if there is an exact filename match, set it as default entry, otherwise, use last one added
  if(defaultValue)
    {
    menu->SetValue(defaultValue);
    }
  else if(mod)
    {
    menu->SetValue(mod->RemovePath(mod->GetFileEntry()->GetValue()));
    }
  sprintf(mesg,"Multiple open sources match the data type of the file path \"%s\" stored with this lookmark. Please select which source to use, then press OK.",name);
  dialog->SetText( mesg );
  dialog->SetTitle( "Multiple Matching Sources" );
  dialog->SetIcon();
  dialog->BeepOn();
  if(dialog->Invoke())
    {
    itChoices->InitTraversal();
    while(!itChoices->IsDoneWithTraversal())
      {
      pvs = static_cast<vtkPVSource*>( itChoices->GetCurrentObject() );
      mod = vtkPVReaderModule::SafeDownCast(pvs);
      if(!strcmp(menu->GetValue(),mod->RemovePath(mod->GetFileEntry()->GetValue())))
        {
        source = pvs;
        break;
        }
      itChoices->GoToNextItem();
      }
    }
  itChoices->Delete();
  dialog->Delete();
/*
        }
      choices->RemoveAllItems();
      }
      }
    if(source)
      {
      break;
      } 
    itOuter->GoToNextItem();
    }
  itOuter->Delete();
  itInner->Delete();
  choices->Delete();
*/
  return source;
}


//----------------------------------------------------------------------------
vtkPVSource* vtkPVLookmark::GetReaderForLookmark(vtkPVSourceCollection *readers,char *moduleName, char *name)
{
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();
  vtkPVSource *pvs;
  vtkPVSource *source = NULL;
  vtkPVSource *currentSource = win->GetCurrentPVSource();
  char *targetName;
  vtkPVReaderModule *mod;
  FILE *file;

  // If there is an open dataset of the same type and path, return it
/*
  vtkPVSourceCollection *col = this->GetPVApplication()->GetMainWindow()->GetSourceList("Sources");
  if (col == NULL)
    {
    source = NULL;
    }
*/
  vtkCollectionIterator *it = readers->NewIterator();
  it->InitTraversal();
  while ( !it->IsDoneWithTraversal() )
    {
    pvs = static_cast<vtkPVSource*>( it->GetCurrentObject() );
    pvs->SetVisibility(0);
//    if(!pvs->GetPVInput(0))
//      {
      // this is either a Source or Reader
//      if(pvs->IsA("vtkPVReaderModule"))
//        {
        mod = vtkPVReaderModule::SafeDownCast(pvs);
        targetName = (char *)mod->GetFileEntry()->GetValue();
        if(!strcmp(targetName,name) && !strcmp(pvs->GetModuleName(),moduleName))
          {
          source = pvs;
          }
 //       }
//      }
    it->GoToNextItem();
    }
  it->Delete();

  // If there is no match among the open datasets, try to open it automatically, otherwise ask the user
  if(!source)
    {
    if( (file = fopen(name,"r")) != NULL)
      {
      fclose(file);
      if(win->Open(name) == VTK_OK)
        {
        source = win->GetCurrentPVSource();
        source->AcceptCallback();
        }
      }
    else
      {
      vtkKWMessageDialog::PopupMessage(
        this->GetPVApplication(), win, "Could Not Find Default Data Set", 
        "You will now be asked to select a different data set.", 
        vtkKWMessageDialog::ErrorIcon);

      win->OpenCallback();
      source = win->GetCurrentPVSource();
      if(source==currentSource || !source->IsA("vtkPVReaderModule"))
        {
        return 0;
        }
      source->AcceptCallback();

      this->Script("focus %s",this->GetWidgetName());
      }
    }

  return source;

}

void vtkPVLookmark::CreateIconFromMainView()
{
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();

  // withdraw the pane so that the lookmark will be added corrrectly
  this->Script("wm withdraw %s", this->GetPVLookmarkManager()->GetWidgetName());
  if(win->GetTclInteractor())
    this->Script("wm withdraw %s", win->GetTclInteractor()->GetWidgetName());
  this->Script("focus %s",win->GetWidgetName());
  for(int i=0;i<4;i++)
    {
    this->GetPVLookmarkManager()->Script("update");
    this->GetPVRenderView()->ForceRender();
    }

  vtkKWIcon *lmkIcon = this->GetIconOfRenderWindow(this->GetPVRenderView()->GetRenderWindow());
  this->GetPVRenderView()->ForceRender();
  this->GetPVLookmarkManager()->Display();
  this->SetLookmarkImage(lmkIcon);
  this->SetImageData(this->GetEncodedImageData(lmkIcon));
  this->SetLookmarkIconCommand();

  if(this->MacroFlag)
    {
    this->AddLookmarkToolbarButton(lmkIcon);
    }

  lmkIcon->Delete();
}

void vtkPVLookmark::CreateIconFromImageData()
{
  if(!this->ImageData)
    {
    return;
    }
  unsigned long imageSize = this->Width*this->Height*this->PixelSize;
  unsigned char *decodedImageData = new unsigned char[imageSize];
  vtkBase64Utilities *decoder = vtkBase64Utilities::New();
  decoder->Decode((unsigned char*)this->ImageData,imageSize,decodedImageData);
  vtkKWIcon *icon = vtkKWIcon::New();
  icon->SetImage(decodedImageData,this->Width,this->Height,this->PixelSize,imageSize);
  this->SetLookmarkImage(icon);
  this->SetLookmarkIconCommand();

  if(this->MacroFlag)
    {
    this->AddLookmarkToolbarButton(icon);
    }

  delete [] decodedImageData;
  decoder->Delete();
  icon->Delete();
}

//----------------------------------------------------------------------------
void vtkPVLookmark::AddLookmarkToolbarButton(vtkKWIcon *icon)
{
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();

  if(!this->ToolbarButton)
    {
    this->ToolbarButton = vtkKWPushButton::New();
    this->ToolbarButton->SetParent(win->GetLookmarkToolbar()->GetFrame());
    this->ToolbarButton->Create(this->GetPVApplication());
    this->ToolbarButton->SetImageToIcon(icon);
    this->ToolbarButton->SetBalloonHelpString(this->GetName());
    this->ToolbarButton->SetCommand(this, "ViewLookmarkWithCurrentDataset");
    win->GetLookmarkToolbar()->AddWidget(this->ToolbarButton);
    }
}

//----------------------------------------------------------------------------
void vtkPVLookmark::StoreStateScript()
{
  FILE *lookmarkScript;
  char buf[300];
  char filter[50];
  char *cmd;
  char *stateScript;
  ostrstream state;
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();

  win->SetSaveVisibleSourcesOnlyFlag(1);
  win->SaveState("tempLookmarkState.pvs");
  win->SetSaveVisibleSourcesOnlyFlag(0);
//ds
  cmd = new char[200];
  if(strstr(this->Dataset,"/") && !strstr(this->Dataset,"\\"))
    {
    char *ptr = this->Dataset;
    ptr+=strlen(ptr)-1;
    while(*ptr!='/' && *ptr!='\\')
      ptr--;
    ptr++;
    sprintf(cmd,"Operations: %s", ptr);
    }
  else
    {
    sprintf(cmd,"Operations: %s", this->Dataset);
    }

  //read the session state file in to a new vtkPVLookmark
  if((lookmarkScript = fopen("tempLookmarkState.pvs","r")) != NULL)
    {
    while(fgets(buf,300,lookmarkScript))
      {
      if(strstr(buf,"CreatePVSource") && !strstr(buf,this->Dataset))
        {
        sscanf(buf,"%*s %*s %*s %*s %[^]]",filter);
        cmd = (char *)realloc(cmd,strlen(cmd)+strlen(filter)+5);
        sprintf(cmd,"%s, %s ",cmd,filter);
        }
      state << buf;
      }
    }
  state << ends;
  fclose(lookmarkScript);
  stateScript = new char[strlen(state.str())+1];
  strcpy(stateScript,state.str());
  this->SetStateScript(stateScript);
  delete [] stateScript;

  if(!this->GetComments())
    {
    this->SetComments(cmd);
    }

  delete [] cmd;
  remove("tempLookmarkState.pvs");
}

//----------------------------------------------------------------------------
void vtkPVLookmark::ViewLookmarkWithCurrentDataset()
{
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();

  // if the pipeline is empty, don't add
  if(win->GetSourceList("Sources")->GetNumberOfItems()==0)
    {
    vtkKWMessageDialog::PopupMessage(
      this->GetPVApplication(), win, "No Data Loaded", 
      "You must first open your data to execute a lookmark macro", 
      vtkKWMessageDialog::ErrorIcon);
    return;
    }


  // get the camera props to restore after
  vtkCamera *cam = this->GetPVRenderView()->GetRenderer()->GetActiveCamera();
  vtkCamera *camera = cam->NewInstance();
  camera->SetParallelScale(cam->GetParallelScale());
  camera->SetViewAngle(cam->GetViewAngle());
  camera->SetClippingRange(cam->GetClippingRange());
  camera->SetFocalPoint(cam->GetFocalPoint());
  camera->SetPosition(cam->GetPosition());
  camera->SetViewUp(cam->GetViewUp());

  this->GetTraceHelper()->AddEntry("$kw(%s) ViewLookmarkWithCurrentDataset",
                      this->GetTclName());

  if(!this->ErrorEventTag)
    {
 //   this->ErrorEventTag = this->GetPVApplication()->GetMainWindow()->AddObserver(
//      vtkKWEvent::ErrorMessageEvent, this->Observer);
    }

  char *temp_script = new char[strlen(this->GetStateScript())+1];
  strcpy(temp_script,this->GetStateScript());

  // this prevents other filters' visibility from disturbing the lookmark view
  this->TurnFiltersOff();

  this->ParseAndExecuteStateScript(temp_script,1);

  delete [] temp_script;

  vtkPVSource *pvs = win->GetCurrentPVSource();
  if(pvs && pvs->GetNotebook())
    this->GetPVRenderView()->UpdateNavigationWindow(pvs,0);

  if (this->ErrorEventTag)
    {
//    this->GetPVApplication()->GetMainWindow()->RemoveObserver(this->ErrorEventTag);
//    this->ErrorEventTag = 0;
    }

  // copy the parameters of the current camera for this class
  // into the active camera on the client and server
  vtkSMProxy* renderModuleProxy = this->GetPVApplication()->
    GetRenderModuleProxy();
  vtkSMDoubleVectorProperty* dvp;

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    renderModuleProxy->GetProperty("CameraPosition"));
  if (dvp)
    {
    dvp->SetElements(camera->GetPosition());
    }
  else
    {
    vtkErrorMacro("Failed to find property CameraPosition.");
    }

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    renderModuleProxy->GetProperty("CameraFocalPoint"));
  if (dvp)
    {
    dvp->SetElements(camera->GetFocalPoint());
    }
  else
    {
    vtkErrorMacro("Failed to find property CameraFocalPoint.");
    }

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    renderModuleProxy->GetProperty("CameraViewUp"));
  if (dvp)
    {
    dvp->SetElements(camera->GetViewUp());
    }
  else
    {
    vtkErrorMacro("Failed to find property CameraFocalPoint.");
    }

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    renderModuleProxy->GetProperty("CameraViewAngle"));
  if (dvp)
    {
    dvp->SetElement(0, camera->GetViewAngle());
    }
  else
    {
    vtkErrorMacro("Failed to find property CameraViewAngle.");
    }

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    renderModuleProxy->GetProperty("CameraClippingRange"));
  if (dvp)
    {
    dvp->SetElements(camera->GetClippingRange());
    }
  else
    {
    vtkErrorMacro("Failed to find property CameraClippingRange.");
    }

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    renderModuleProxy->GetProperty("CameraParallelScale"));
  if (dvp)
    {
    dvp->SetElement(0, camera->GetParallelScale());
    }
  else
    {
    vtkErrorMacro("Failed to find property CameraParallelScale.");
    }
  renderModuleProxy->UpdateVTKObjects(); 
  this->GetPVRenderView()->EventuallyRender();
  camera->Delete();
}

void vtkPVLookmark::Update()
{
  this->GetTraceHelper()->AddEntry("$kw(%s) Update",
                      this->GetTclName());

  //create and store a new session state file
  this->StoreStateScript();
  if(this->MacroFlag)
    {
    this->GetPVApplication()->GetMainWindow()->GetLookmarkToolbar()->RemoveWidget(this->ToolbarButton);
    this->ToolbarButton->Delete();
    this->ToolbarButton = 0;
    }
  this->CreateIconFromMainView();
  this->UpdateWidgetValues();

//  this->SetCenterOfRotation(this->PVApplication->GetMainWindow()->GetCenterOfRotationStyle()->GetCenter());
}



//----------------------------------------------------------------------------
vtkKWIcon *vtkPVLookmark::GetIconOfRenderWindow(vtkRenderWindow *renderWindow)
{

  vtkWindowToImageFilter *w2i = vtkWindowToImageFilter::New();
  w2i->SetInput(renderWindow);
  w2i->ShouldRerenderOff();
  w2i->Update();

  this->GetPVRenderView()->GetRenderWindow()->SwapBuffersOn();
  this->GetPVRenderView()->GetRenderWindow()->Frame();

  int* dim = w2i->GetOutput()->GetDimensions();
  float width = dim[0];
  float height = dim[1];

  int *extent = w2i->GetOutput()->GetExtent();
  int extentW = extent[1] - extent[0] + 1;
  int extentH = extent[3] - extent[2] + 1;
  float extentN = 0;

  vtkImageClip *iclip = vtkImageClip::New();
  if(width>height)
    {
    int extentD = extentW - extentH;
    extentN = extentH;
    int arg1 = extent[0]+ extentD/2;
    int arg2 = extent[1]-extentD/2;
    iclip->SetOutputWholeExtent(arg1,arg2,extent[2],extent[3],extent[4],extent[5]);
    }
  else if(width<height)
    {
    int extentD = extentH - extentW;
    extentN = extentW;
    int arg1 = extent[2]+extentD/2;
    int arg2 = extent[3]-extentD/2;
    iclip->SetOutputWholeExtent(extent[0],extent[1],arg1,arg2,extent[4],extent[5]);
    }
  else
    {
    extentN = extentW;
    iclip->SetOutputWholeExtent(extent[0],extent[1],extent[2],extent[3],extent[4],extent[5]);
    }
  iclip->SetInput(w2i->GetOutput());
  iclip->Update();

//  int scaledW = width/20;
//  int scaledH = height/20;

  vtkImageResample *resample = vtkImageResample::New();
  resample->SetAxisMagnificationFactor(0,this->Width/extentN);
  resample->SetAxisMagnificationFactor(1,this->Height/extentN);
  resample->SetInput(iclip->GetOutput());
  resample->Update();

  vtkImageData *img_data = resample->GetOutput();
  int *wext = img_data->GetWholeExtent();

  this->PixelSize = img_data->GetNumberOfScalarComponents();

  vtkKWIcon* icon = vtkKWIcon::New();
  icon->SetImage(
    static_cast<unsigned char*>(img_data->GetScalarPointer()), 
    wext[1] - wext[0] + 1,
    wext[3] - wext[2] + 1,
    this->PixelSize,
    0,
    vtkKWIcon::ImageOptionFlipVertical);

  w2i->Delete();
  resample->Delete();
  iclip->Delete();

  return icon;
}

//----------------------------------------------------------------------------
char *vtkPVLookmark::GetEncodedImageData(vtkKWIcon *icon)
{
  const unsigned char *imageData = icon->GetData();
  int imageSize = this->Width*this->Height*this->PixelSize;
  int encodedImageSize = imageSize*2;
  unsigned char *encodedImageData = new unsigned char[encodedImageSize];
  vtkBase64Utilities *encoder = vtkBase64Utilities::New();
  unsigned long size = encoder->Encode(imageData,imageSize,encodedImageData);
  encodedImageData[size] = '\0';
  encoder->Delete();

  return (char *)encodedImageData;
}


//----------------------------------------------------------------------------
void vtkPVLookmark::SetLookmarkIconCommand()
{
  if(this->MacroFlag)
    {
    this->LmkIcon->SetBinding(
      "<Button-1>", this, "ViewLookmarkWithCurrentDataset");
    this->LmkIcon->SetBinding(
      "<Double-1>", this, "ViewLookmarkWithCurrentDataset");
    }
  else
    {
    this->LmkIcon->SetBinding("<Button-1>", this, "View");
    this->LmkIcon->SetBinding("<Double-1>", this, "View");
    }
}

//----------------------------------------------------------------------------
void vtkPVLookmark::UnsetLookmarkIconCommand()
{
  this->LmkIcon->RemoveBinding("<Button-1>");
  this->LmkIcon->RemoveBinding("<Double-1>");
}

//----------------------------------------------------------------------------
void vtkPVLookmark::SetLookmarkImage(vtkKWIcon *icon)
{
  if(this->LmkIcon)
    {
    this->LmkIcon->SetImageToIcon(icon);
    }
}

//----------------------------------------------------------------------------
void vtkPVLookmark::TurnFiltersOff()
{
  //turn all sources invisible
  vtkPVSource *pvs;
  vtkPVSourceCollection *col = this->GetPVApplication()->GetMainWindow()->GetSourceList("Sources");
  if (col == NULL)
    {
    return;
    }
  vtkCollectionIterator *it = col->NewIterator();
  it->InitTraversal();
  while ( !it->IsDoneWithTraversal() )
    {
    pvs = static_cast<vtkPVSource*>( it->GetCurrentObject() );
    pvs->SetVisibility(0);
    it->GoToNextItem();
    }
  it->Delete();
}


//----------------------------------------------------------------------------
void vtkPVLookmark::ParseAndExecuteStateScript(char *script, int macroFlag)
{
  vtkPVSource *src;
  char *ptr;
  char *ptr1;
  char *ptr2;
  int ival;
  int i=0;
  char srcTclName[20]; 
  char *tclName;
  char cmd[200];
  bool foundSource = false;
  char srcLabel[100];
  vtkStdString string1,string2;
  vtkstd::string::size_type beg;
  //vtkstd::string::size_type end;
  bool executeCmd = true;
  char **buf = new char*[100];
  char moduleName[50];

  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();

  vtkPVSourceCollection *col = this->GetPVApplication()->GetMainWindow()->GetSourceList("Sources");
  vtkPVSourceCollection *readers = vtkPVSourceCollection::New();
  vtkPVSourceCollection *sources = vtkPVSourceCollection::New();
  if (col == NULL)
    {
    return;
    }
  vtkCollectionIterator *it = col->NewIterator();
  it->InitTraversal();
  while ( !it->IsDoneWithTraversal() )
    {
    src = static_cast<vtkPVSource*>( it->GetCurrentObject() );
    if(!src->GetPVInput(0))
      {
      if(src->IsA("vtkPVReaderModule"))
        {
        readers->AddItem(src);
        }
      else
        {
        sources->AddItem(src);
        }
      }
    it->GoToNextItem();
    }
  it->Delete();


  this->Script("[winfo toplevel %s] config -cursor watch", 
                this->GetWidgetName());

  this->GetPVRenderView()->StartBlockingRender();

  vtkstd::map<vtkPVSource*, char*> tclNameMap;
  vtkstd::map<vtkPVSource*, char*>::iterator iter = tclNameMap.begin();

  // parse script for "createpvsource" or "initialize read custom" commands
  // if filename (not including extension) matches one and only one source from collection, initialize, store tclname, and continue
  // if they do not, prompt user to match open sources to ones in state script, continue parsing and initializing, store tclname
  // if more sources in script than in collection or vice versa, fail for now
  // if "createpvsource" and its tcl name does not match any of our stored ones, check for an input widget on a subsequent line, if none, prompt user, else treat as filter
  // if it is a filter, execute commands, looking for input pvwidget (input menu or groupinputwidget) when you get to one, set the appropriate source tied to the tcl name as the currentpvsource
  // when all sources in collection have been initialized, assume we are done and parse rest of script, if we come across "createpvsource" or "Init read custom" prompt user and ignore secion

  ptr = strtok(script,"\r\n");

  while(ptr)
    {
    if(!foundSource && !strstr(ptr,"InitializeReadCustom") && !strstr(ptr,"CreatePVSource"))
      {
      this->Script(ptr);
      ptr = strtok(NULL,"\r\n");
      }
    else if(strstr(ptr,"InitializeReadCustom") )
      {
      // get tcl name for source
      sscanf(ptr,"%*s %s %*s %*s \"%[^\"]",srcTclName,moduleName);
      foundSource = true;
      // next line should be readfileinformation with path in quotes
      ptr = strtok(NULL,"\r\n");
      // get at the file name without path or extension
      ptr1 = strstr(ptr,"\"");
      ptr1++;
      ptr2 = ptr1;
      ptr2+=strlen(ptr1);
      while(*ptr2!='\"')
        ptr2--;
      *ptr2 = '\0';

      if(macroFlag)
        {
        src = this->GetReaderForMacro(readers,moduleName,ptr1);
        }
      else
        {
        // look for an open dataset of the same path and reader type
        // if none is open, try to open the one at the specified path automatically for user
        // if that fails, prompt user for a new data file
        src = this->GetReaderForLookmark(readers,moduleName,ptr1);
        }

      if(src)
        {
        tclName = new char[20];
        strcpy(tclName,srcTclName);
        tclNameMap[src] = tclName;
        i = 0;
        buf[i] = strtok(NULL,"\r\n"); 
        while(!strstr(buf[i],"GetPVWidget"))    //!strstr(buf[i],"inputs") && !strstr(buf[i],"Input")))
          {
          buf[++i] = strtok(NULL,"\r\n"); 
          }
        this->InitializeSourceFromScript(src,buf[i],macroFlag);
        // remove this source from the available list
        readers->RemoveItem(src);
        }
      else
        {
        // this really shouldn't fail, but if it does, quit
        break;
        }

      ptr = strtok(NULL,"\r\n");

/*
      sscanf(ptr,SecondToken_String,srcTclName);
      string1 = ptr;
      beg = string1.find_first_of('\"',0);
      end = string1.find_first_of('\"',beg+1);
      string1 = string1.substr(beg+1,end-beg-1);
      strcpy(moduleName,string1.c_str());
      string1 = ptr;
      beg = string1.find_first_of("\"",end+1);
      end = string1.find_first_of('\"',beg+1);
      string1 = string1.substr(beg+1,end-beg-1);
      strcpy(path,string1.c_str());
*/

      }
    else if(strstr(ptr,"CreatePVSource"))
      {
      strcpy(cmd,ptr);
      sscanf(cmd,"%*s %s",srcTclName);
 //     ptr1 = ptr;
 //     const char *tclName = ptr1+=4;
/*
      string1 = ptr;
      end = string1.find_last_of("]",string1.size());
      string2 = string1.substr(0,end);
      beg = string2.find_first_of("CreatePVSource",0);
      string1 = string2.substr(beg,end-beg);
      beg = string1.find_first_of(" ",0);
      string2 = string1.substr(beg,end-beg);
*/
      ptr1 = strstr(cmd,"]");
      *ptr1 = '\0';
      while(*ptr1 != ' ')
        {
        ptr1--;
        }
      ptr1++;

      foundSource = true;

      // ASSUMPTION: the first pvwidget listed in script will be an input if it is a filter
      // this is a filter, set its input
      buf[0] = ptr;
      i = 1;
      buf[i] = strtok(NULL,"\r\n"); 
      while(!strstr(buf[i],"GetPVWidget"))    //!strstr(buf[i],"inputs") && !strstr(buf[i],"Input")))
        {
        buf[++i] = strtok(NULL,"\r\n"); 
        }
      if(strstr(buf[i],"inputs"))
        {
        // group input widget
        // execute the createpvsource command
        this->Script(buf[0]);
        this->GetPVLookmarkManager()->Withdraw();
        vtkKWMessageDialog *dlg2 = vtkKWMessageDialog::New();
        dlg2->SetMasterWindow(win);
        dlg2->SetOptions(
          vtkKWMessageDialog::WarningIcon | vtkKWMessageDialog::Beep | vtkKWMessageDialog::YesDefault );
        dlg2->SetModal(0);
        dlg2->Create(this->GetPVApplication());
        dlg2->SetText( "Please use the Append filter panel to select the inputs to the filter. Then press OK." );
        dlg2->SetTitle( "Group Input Widget Detected" );
        dlg2->SetIcon();
        dlg2->BeepOn();
        dlg2->Invoke();
        dlg2->Delete();
        this->GetPVLookmarkManager()->Display();
        ptr = strtok(NULL,"\r\n");
        while(!strstr(ptr,"AcceptCallback"))
          {
          ptr = strtok(NULL,"\r\n");
          }
        this->Script(ptr);
        }
      else if(strstr(buf[i],"Input"))
        {
        // input menu widget
        ptr = strtok(NULL,"\r\n");
        sscanf(ptr,"%*s %*s %s",srcTclName);
        iter = tclNameMap.begin();
        while(iter != tclNameMap.end())
          {
          ptr1 = iter->second;
          if(strstr(srcTclName,iter->second))
            {
            win->SetCurrentPVSource(iter->first);
            }
          ++iter;
          }
        for(ival=0;ival<i;ival++)
          {
          this->Script(buf[ival]);
          }
        }
      else
        {
        // no input widget means this is a source
        if(macroFlag)
          {
          // if multiple sources are open and the lookmark creates multiple source, ask user
          // else if the lookmark is single sources, use the currently selected one
          src = this->GetSourceForMacro(sources,ptr1);
          }
        else
          {
          src = this->GetSourceForLookmark(sources,ptr1);
          }

        if(src)
          {
          tclName = new char[20]; 
          strcpy(tclName,srcTclName);
          tclNameMap[src] = tclName;
          this->InitializeSourceFromScript(src,buf[i],macroFlag);
          // remove this source from the available list
          sources->RemoveItem(src);
          }
        else
          {
          break;
          }
        }

      ptr = strtok(NULL,"\r\n");

      }
    else
      {
      if(strstr(ptr,"AcceptCallback"))
        {
        // the current source would be the filter just created
        src = this->GetPVApplication()->GetMainWindow()->GetCurrentPVSource();

        // append the lookmark name to its filter name : strip off any characters after '-' because a lookmark name could already have been appended to this filter name
        strcpy(srcLabel,src->GetLabel());
        if((ptr1 = strstr(srcLabel,"-")))
          {
          *ptr1 = '\0';
          }
        strcat(srcLabel,"-");
        strcat(srcLabel,this->GetName());
        src->SetLabel(srcLabel);

        //add all pvsources created by this lmk to its collection
        this->AddPVSource(src);
        src->SetLookmark(this);
        }

      if(macroFlag==1 && strstr(ptr,"SetCameraState"))
        {
        ptr = strtok(NULL,"\r\n");
        continue;
        }
    
      // encode special '%' character with a preceeding '%' for printf cmd in call to Script()
      if(strstr(ptr,"%"))
        {
        ptr1 = ptr;
        i = 0;
        while(*ptr1)
          {
          if(*ptr1=='%')
            {
            cmd[i] = *ptr1;
            i++;
            }
          cmd[i] = *ptr1;
          ptr1++;
          i++;
          }      
        cmd[i] = '\0';
        ptr = cmd;
        }

      iter = tclNameMap.begin();
      while(iter != tclNameMap.end())
        {
        if(strstr(ptr,iter->second))
          {
          if(strstr(ptr,"SetVisibility"))
            {
            sscanf(ptr,"%*s %*s %d",&ival);
            iter->first->SetVisibility(ival);
            }
          if(strstr(ptr,"ShowVolumeAppearanceEditor"))
            {
            this->InitializeVolumeAppearanceEditor(iter->first,ptr);
            }
          executeCmd = false;

/*
          string1 = ptr;
          beg = string1.rfind(iter->second,string1.size());
          ival = strlen(iter->second);
          //val = strlen(iter->first->GetTclName());
          sprintf(srcTclName,"kw(%s)",iter->first->GetTclName());
          string1.replace(beg,ival,srcTclName);
          this->Script(string1.c_str());
          executeCmd = false;
*/
          }
        ++iter;
        }

      if(executeCmd)
        {
        string1 = ptr;

        // the command does not contain a source tcl name
        if(strstr(ptr,"ColorByArray"))
          {
          string1.erase(string1.find_first_of('[',0),1);
          beg = string1.rfind("GetPVOutput]",string1.size());
          string1.erase(beg,13);
          }

        this->Script(string1.c_str());
        }
      executeCmd = true;

      ptr = strtok(NULL,"\r\n");

      }
    }

  this->Script("[winfo toplevel %s] config -cursor {}", 
                this->GetWidgetName());
  this->GetPVRenderView()->EndBlockingRender();
  // delete tclNameMa
  iter = tclNameMap.begin();
  while(iter != tclNameMap.end())
    {
    delete [] iter->second;
    ++iter;
    }
  delete [] buf;
  readers->Delete();
  sources->Delete();
}

void vtkPVLookmark::InitializeVolumeAppearanceEditor(vtkPVSource *src, char *firstLine)
{
  double fval1,fval2,fval3,fval4;
  int ival;

  vtkPVVolumeAppearanceEditor *vol = this->GetPVApplication()->GetMainWindow()->GetVolumeAppearanceEditor();

  char *ptr = firstLine;
  // needed to display edit volume appearance setting button:
  src->GetPVOutput()->DrawVolume();
  src->GetPVOutput()->ShowVolumeAppearanceEditor();

  ptr = strtok(NULL,"\r\n");
  vol->RemoveAllScalarOpacityPoints();

  ptr = strtok(NULL,"\r\n");
  while(strstr(ptr,"AppendScalarOpacityPoint"))
    {
    sscanf(ptr,"%*s %*s %lf %lf",&fval1,&fval2);
    vol->AppendScalarOpacityPoint(fval1,fval2);
    ptr = strtok(NULL,"\r\n");
    }

  sscanf(ptr,"%*s %*s %lf ",&fval1);
  vol->SetScalarOpacityUnitDistance(fval1);

  ptr = strtok(NULL,"\r\n");
  vol->RemoveAllColorPoints();

  ptr = strtok(NULL,"\r\n");
  while(strstr(ptr,"AppendColorPoint"))
    {
    sscanf(ptr,"%*s %*s %lf %lf",&fval1,&fval2,&fval3,&fval4);
    vol->AppendColorPoint(fval1,fval2,fval3,fval4);
    ptr = strtok(NULL,"\r\n");
    }

  sscanf(ptr,"%*s %*s %d",&ival);
  vol->SetHSVWrap(ival);

  ptr = strtok(NULL,"\r\n");
  sscanf(ptr,"%*s %*s %d",&ival);
  vol->SetColorSpace(ival);
}


void vtkPVLookmark::InitializeSourceFromScript(vtkPVSource *source, char *firstLine, int macroFlag)
{
  vtkPVScale *scale;
  vtkPVArraySelection *arraySelection;
  vtkPVLabeledToggle *labeledToggle;
  vtkPVFileEntry *fileEntry;
  vtkPVSelectTimeSet *selectTimeSet;
  vtkPVVectorEntry *vectorEntry;
  vtkPVSelectionList *selectionList;
  vtkPVStringEntry *stringEntry;
  vtkPVSelectWidget *selectWidget;
  vtkPVMinMax *minMaxWidget;
#ifdef PARAVIEW_USE_EXODUS
  vtkPVBasicDSPFilterWidget *dspWidget;
#endif

  vtkPVWidget *pvWidget;
  char *ptr;
  char sval[100];
  double fval; 
  int ival;
  int i=0;
  char displayName[50];
  int val;
  char ve[6][10];
  char SecondToken_String[] = "%*s %s";
  char FifthToken_WrappedString[] = "%*s %*s %*s %*s {%[^}]";
  char ThirdToken_Float[] = "%*s %*s %lf";
  char ThirdAndFourthTokens_WrappedStringAndInt[] = "%*s %*s {%[^}]} %d";
  char ThirdToken_Int[] = "%*s %*s %d";
  char ThirdToken_WrappedString[] = "%*s %*s {%[^}]";
  char ThirdToken_OptionalWrappedString[] = "%*s %*s %*[{\"]%[^}\"]";
  char ThirdThruEighthToken_String[] = "%*s %*s %s %s %s %s %s %s";
#ifdef PARAVIEW_USE_EXODUS
  char ThirdToken_String[] = "%*s %*s %s";
#endif
  char ThirdToken_RightBracketedString[] = "%*s %*s %[^]] %*s %*s %*s";
  char FifthAndSixthToken_IntAndInt[] = "%*s %*s %*s %*s %d %d";
  char FifthAndSixthToken_IntAndFloat[] = "%*s %*s %*s %*s %d %lf";
  char FifthAndSixthToken_IntAndWrappedString[] = "%*s %*s %*s %*s %d {%[^}]";
  char FourthAndFifthToken_WrappedStringAndInt[] = "%*s %*s %*s {%[^}]} %d";
  vtkStdString string1,string2;
/*
  ptr = strtok(NULL,"\r\n");

  while(!strstr(ptr,"GetPVWidget"))
    {
    ptr = strtok(NULL,"\r\n");
    }
*/
  ptr = firstLine;

  //loop through collection till found, operate accordingly leaving else statement with ptr one line past 
  vtkCollectionIterator *it = source->GetWidgets()->NewIterator();
  it->InitTraversal();
  
  while( !it->IsDoneWithTraversal() )
    {
    pvWidget = static_cast<vtkPVWidget*>(it->GetCurrentObject());
    sscanf(ptr,FifthToken_WrappedString,sval);
    if(!strcmp(sval,pvWidget->GetTraceHelper()->GetObjectName()))
      {
      if((scale = vtkPVScale::SafeDownCast(pvWidget)))
        {
        ptr = strtok(NULL,"\r\n");
        if(!macroFlag)
          {
          sscanf(ptr,ThirdToken_Float,&fval);
          scale->SetValue(fval);
          }
        ptr = strtok(NULL,"\r\n");
        }
      else if((arraySelection = vtkPVArraySelection::SafeDownCast(pvWidget)))
        { 
        ptr = strtok(NULL,"\r\n");
        while(strstr(ptr,"SetArrayStatus"))
          {
          sscanf(ptr,ThirdAndFourthTokens_WrappedStringAndInt,sval,&ival);
          //val = this->GetArrayStatus(name,ptr);
          //only turn the variable on, not off, because some other filter might be using the variable
          if(ival)
            arraySelection->SetArrayStatus(sval,ival);
          ptr = strtok(NULL,"\r\n");  
          }
        arraySelection->ModifiedCallback();
        arraySelection->Accept();
        }
      else if((labeledToggle = vtkPVLabeledToggle::SafeDownCast(pvWidget)))
        {
        ptr = strtok(NULL,"\r\n");
        sscanf(ptr,ThirdToken_Int,&ival);
        labeledToggle->SetSelectedState(ival);
        labeledToggle->ModifiedCallback();
        ptr = strtok(NULL,"\r\n");
        }
      else if((selectWidget = vtkPVSelectWidget::SafeDownCast(pvWidget)))
        {
        //get the third token of the next line and take off brackets which will give you the value of select widget:
        ptr = strtok(NULL,"\r\n");
        sscanf(ptr,ThirdToken_WrappedString,sval);
        selectWidget->SetCurrentValue(sval); 
        //ignore next line
        ptr = strtok(NULL,"\r\n");
        ptr = strtok(NULL,"\r\n");
        sscanf(ptr,FifthToken_WrappedString,sval);
        arraySelection = vtkPVArraySelection::SafeDownCast(selectWidget->GetPVWidget(sval));
        ptr = strtok(NULL,"\r\n");
        while(strstr(ptr,"SetArrayStatus"))
          {
          sscanf(ptr,ThirdAndFourthTokens_WrappedStringAndInt,sval,&ival);
          arraySelection->SetArrayStatus(sval,ival);
          ptr = strtok(NULL,"\r\n"); 
          }
        //arraySelection->Accept();
        arraySelection->ModifiedCallback();
        } 
      else if((selectTimeSet = vtkPVSelectTimeSet::SafeDownCast(pvWidget)))
        {
        ptr = strtok(NULL,"\r\n");
        sscanf(ptr,ThirdToken_OptionalWrappedString,sval);
        selectTimeSet->SetTimeValueCallback(sval);
        selectTimeSet->ModifiedCallback();
        ptr = strtok(NULL,"\r\n");
        }
      else if((vectorEntry = vtkPVVectorEntry::SafeDownCast(pvWidget)))
        {
        // could be up to 6 fields
        ptr = strtok(NULL,"\r\n");
        ival = sscanf(ptr,ThirdThruEighthToken_String,ve[0], ve[1], ve[2], ve[3], ve[4], ve[5]);

        switch (ival)
          {
          case 1:
            vectorEntry->SetValue(ve[0]);
            break;
          case 2:
            vectorEntry->SetValue(ve[0],ve[1]);
            break;
          case 3:
            vectorEntry->SetValue(ve[0],ve[1],ve[2]);
            break;
          case 4:
            vectorEntry->SetValue(ve[0],ve[1],ve[2],ve[3]);
            break;
          case 5:
            vectorEntry->SetValue(ve[0],ve[1],ve[2],ve[3],ve[4]);
            break;
          case 6:
            vectorEntry->SetValue(ve[0],ve[1],ve[2],ve[3],ve[4],ve[5]);
            break;
          }

        vectorEntry->ModifiedCallback();
        ptr = strtok(NULL,"\r\n");
        }
      else if((fileEntry = vtkPVFileEntry::SafeDownCast(pvWidget)))
        {
        ptr = strtok(NULL,"\r\n");
        ptr = strtok(NULL,"\r\n");
        }
      else if((selectionList = vtkPVSelectionList::SafeDownCast(pvWidget)))
        {
        ptr = strtok(NULL,"\r\n");
        sscanf(ptr,ThirdToken_Int,&ival);
        selectionList->SetCurrentValue(ival);
        selectionList->ModifiedCallback();
        ptr = strtok(NULL,"\r\n");
        }
      else if((stringEntry = vtkPVStringEntry::SafeDownCast(pvWidget)))
        {
        //  This widget is used
        ptr = strtok(NULL,"\r\n");
        ptr = strtok(NULL,"\r\n");
        }
      else if((minMaxWidget = vtkPVMinMax::SafeDownCast(pvWidget)))
        {
        ptr = strtok(NULL,"\r\n");
        sscanf(ptr,ThirdToken_Int,&ival);
        minMaxWidget->SetMaxValue(ival);
        ptr = strtok(NULL,"\r\n");
        sscanf(ptr,ThirdToken_Int,&ival);
        minMaxWidget->SetMinValue(ival);
        minMaxWidget->ModifiedCallback();
        ptr = strtok(NULL,"\r\n");
        }
#ifdef PARAVIEW_USE_EXODUS
      else if((dspWidget = vtkPVBasicDSPFilterWidget::SafeDownCast(pvWidget)))
        {
        ptr = strtok(NULL,"\r\n");
        sscanf(ptr,ThirdToken_String,sval);
        dspWidget->ChangeDSPFilterMode(sval);
        ptr = strtok(NULL,"\r\n");
        sscanf(ptr,ThirdToken_String,sval);
        dspWidget->ChangeCutoffFreq(sval);
        ptr = strtok(NULL,"\r\n");
        sscanf(ptr,ThirdToken_String,sval);
        dspWidget->SetFilterLength(atoi(sval));
        ptr = strtok(NULL,"\r\n");
        }
#endif
      else   //if we do not support this widget yet, advance and break loop
        {
        ptr = strtok(NULL,"\r\n");
        }
      }
    it->GoToNextItem();
    }
  //widget in state file is not in widget collection
  if(i==source->GetWidgets()->GetNumberOfItems())
    {
    ptr = strtok(NULL,"\r\n");
    }
  it->Delete();

  while(!strstr(ptr,"AcceptCallback"))
    {
    ptr = strtok(NULL,"\r\n");
    }

  source->AcceptCallback();

  ptr = strtok(NULL,"\r\n");

  // ignore comments
  while(ptr[0]=='#')
    {
    ptr = strtok(NULL,"\r\n");
    }

  // this line sets the partdisplay variable
  sscanf(ptr,SecondToken_String,displayName);

  ptr = strtok(NULL,"\r\n");

  vtkSMDisplayProxy *display = source->GetDisplayProxy();

  while(strstr(ptr,displayName)) 
    {
    if(strstr(ptr,"UpdateVTKObjects"))
      {
      display->UpdateVTKObjects();
      ptr = strtok(NULL,"\r\n");
      continue;
      }

    sscanf(ptr,ThirdToken_RightBracketedString,sval);

    // Borrowed the following code to loop through properties from vtkPVSource::SaveState()

    vtkSMPropertyIterator* iter = source->GetDisplayProxy()->NewPropertyIterator();

    // Even in state we have to use ServerManager API for displays.
    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
      {
      vtkSMProperty* p = iter->GetProperty();
      if(strcmp(sval,p->GetXMLName()))
        { 
        continue;
        }

      vtkSMIntVectorProperty* ivp = 
        vtkSMIntVectorProperty::SafeDownCast(p);
      vtkSMDoubleVectorProperty* dvp = 
        vtkSMDoubleVectorProperty::SafeDownCast(p);
      vtkSMStringVectorProperty* svp =
        vtkSMStringVectorProperty::SafeDownCast(p);
      if (ivp)
        {
        sscanf(ptr,FifthAndSixthToken_IntAndInt,&val,&ival);
        ivp->SetElement(val,ival);
        }
      else if (dvp)
        {
        sscanf(ptr,FifthAndSixthToken_IntAndFloat,&val,&fval);
        dvp->SetElement(val,fval);
        }
      else if (svp)
        {
        sscanf(ptr,FifthAndSixthToken_IntAndWrappedString,&val,sval);
        svp->SetElement(val,sval);
        }

      break;
      }

    iter->Delete();

    ptr = strtok(NULL,"\r\n");

    }

  // make sure that the data information is up to date
//  source->UpdateVTKObjects();
//  source->InvalidateDataInformation();

  if(strstr(ptr,"ColorByArray"))
    {
    sscanf(ptr,FourthAndFifthToken_WrappedStringAndInt,sval,&ival);
    source->ColorByArray(sval, ival);
    }
  else if(strstr(ptr,"VolumeRenderByArray"))
    {
    sscanf(ptr,FourthAndFifthToken_WrappedStringAndInt,sval,&ival);
    source->VolumeRenderByArray(sval, ival);
    }
  else if(strstr(ptr,"ColorByProperty"))
    {
    source->GetPVOutput()->ColorByProperty();
    }

  source->GetPVOutput()->Update();
  source->GetPVOutput()->UpdateColorGUI();

}

//----------------------------------------------------------------------------
int vtkPVLookmark::DeletePVSources()
{
  vtkPVSource *lastPVSource;
  
  lastPVSource = this->Sources->GetLastPVSource();
  // loop thru collection backwards, trying to delete sources as we go, removing them either way
  while(lastPVSource)
    {
    // pvs could have been deleted manually by user - is there a better test for whether the source hass been deleeted?
    if(lastPVSource->GetNotebook() && lastPVSource->IsDeletable())
      lastPVSource->DeleteCallback();
    this->Sources->RemoveItem(lastPVSource);
    lastPVSource = this->Sources->GetLastPVSource();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVLookmark::AddPVSource(vtkPVSource *pvs)
{
  this->Sources->AddItem(pvs);
}

//----------------------------------------------------------------------------
void vtkPVLookmark::RemovePVSource(vtkPVSource *pvs)
{
  this->Sources->RemoveItem(pvs);
}


//----------------------------------------------------------------------------
vtkPVRenderView* vtkPVLookmark::GetPVRenderView()
{
  return this->GetPVApplication()->GetMainView();
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVLookmark::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->GetApplication());
}

//----------------------------------------------------------------------------
vtkPVLookmarkManager* vtkPVLookmark::GetPVLookmarkManager()
{
  return this->GetPVApplication()->GetMainWindow()->GetPVLookmarkManager();
}


void vtkPVLookmark::EnableScrollBar()
{
  if (this->LmkMainFrame->IsFrameCollapsed())
    {
    this->LmkMainFrame->ExpandFrame();
    this->LmkMainFrame->CollapseFrame();
    }
  else
    {
    this->LmkMainFrame->CollapseFrame();
    this->LmkMainFrame->ExpandFrame();
    }
}

//----------------------------------------------------------------------------
void vtkPVLookmark::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "StateScript: " << this->GetStateScript() << endl;
  os << indent << "ImageData: " << this->GetImageData() << endl;
  os << indent << "CenterOfRotation: " << this->GetCenterOfRotation() << endl;
  os << indent << "Dataset: " << this->GetDataset() << endl;
  os << indent << "Location: " << this->GetLocation() << endl;
  os << indent << "TraceHelper: " << this->TraceHelper << endl;
  os << indent << "ToolbarButton: " << this->GetToolbarButton() << endl;

}
