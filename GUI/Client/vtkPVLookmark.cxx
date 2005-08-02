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
#include "vtkSMDataObjectDisplayProxy.h"
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
#include <vtkstd/vector>
#include "vtkStdString.h"
#include "vtkKWMenuButton.h"
#include "vtkPVVolumeAppearanceEditor.h"
#include "vtkPVThumbWheel.h"
//#include "Tokenizer.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVLookmark );
vtkCxxRevisionMacro(vtkPVLookmark, "1.42");


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
vtkPVSource* vtkPVLookmark::GetReaderForMacro(vtkPVSourceCollection *readers,char *, char *name)
{
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();
  vtkPVSource *pvs;
  vtkPVSource *pvs1;
  vtkPVSource *pvs2;
  vtkPVSource *source = NULL;
  vtkPVReaderModule *mod;
  //  const char *ptr1;
  //  const char *ptr2;
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
  char *stateScript;
  ostrstream state;
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();

  win->SetSaveVisibleSourcesOnlyFlag(1);
  win->SaveState("tempLookmarkState.pvs");
  win->SetSaveVisibleSourcesOnlyFlag(0);

  int i=0;
  char *ptr;
  vtkStdString opsList = "Operations: ";
  while(this->DatasetList[i])
    {
    if(strstr(this->DatasetList[i],"/") && !strstr(this->DatasetList[i],"\\"))
      {
      ptr = this->DatasetList[i];
      ptr+=strlen(ptr)-1;
      while(*ptr!='/' && *ptr!='\\')
        ptr--;
      ptr++;
      opsList.append(ptr);
      opsList.append(", ");
      }
    else
      {
      opsList.append(this->DatasetList[i]);
      opsList.append(", ");
      }
    i++;
    }

  //read the session state file in to a new vtkPVLookmark
  if((lookmarkScript = fopen("tempLookmarkState.pvs","r")) != NULL)
    {
    while(fgets(buf,300,lookmarkScript))
      {
      if(strstr(buf,"CreatePVSource") && !strstr(buf,this->Dataset))
        {
        sscanf(buf,"%*s %*s %*s %*s %[^]]",filter);
        opsList.append(filter);
        opsList.append(", ");
        }
      state << buf;
      }
    }
  state << ends;
  unsigned int ret = opsList.find_last_of(',',opsList.size());
  if(ret != vtkstd::string::npos)
    {
    opsList.erase(ret);
    }

  fclose(lookmarkScript);
  stateScript = new char[strlen(state.str())+1];
  strcpy(stateScript,state.str());
  this->SetStateScript(stateScript);
  this->SetComments(opsList.c_str());

  delete [] stateScript;
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

void vtkPVLookmark::Tokenize(const vtkstd::string& str,
                      vtkstd::vector<vtkstd::string>& tokens,
                      const vtkstd::string& delimiters)
{
  // Skip delimiters at beginning.
  vtkstd::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  vtkstd::string::size_type pos = str.find_first_of(delimiters, lastPos);

  while (vtkstd::string::npos != pos || vtkstd::string::npos != lastPos)
    {
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
    }
}

//----------------------------------------------------------------------------
void vtkPVLookmark::ParseAndExecuteStateScript(char *script, int macroFlag)
{
  vtkPVSource *src;
  vtkstd::string ptr;
  char *ptr1;
  int ival;
  int i=0;
  char srcTclName[20]; 
  char *tclName;
  char cmd[200];
  char path[300];
  bool foundSource = false;
  vtkstd::string srcLabel;
  vtkStdString string1,string2;
  vtkstd::string::size_type beg;
  //vtkstd::string::size_type end;
  bool executeCmd = true;
  //char **buf = new char*[100];
  vtkstd::vector<vtkstd::string> buf;
  char moduleName[50];
  vtkstd::string::size_type ret;

  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();

  // Tokenize the script:

  vtkstd::vector<vtkstd::string> tokens;
  vtkstd::vector<vtkstd::string>::iterator tokIter;
  vtkstd::vector<vtkstd::string>::iterator tokMrkr;
  vtkstd::string delimiters = "\r\n";
  vtkstd::string str = script;
  this->Tokenize(str,tokens,delimiters);
/*
  // Skip delimiters at beginning.
  vtkstd::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  vtkstd::string::size_type pos = str.find_first_of(delimiters, lastPos);

  while (vtkstd::string::npos != pos || vtkstd::string::npos != lastPos)
    {
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
    }
//  Tokenizer tokenizer(script);
*/
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

//  ptr = strtok(script,"\r\n");
  tokIter = tokens.begin();
  ptr = *tokIter;

  while(tokIter <= tokens.end())
    {
    if(!foundSource && ptr.rfind("InitializeReadCustom",ptr.size())==vtkstd::string::npos && ptr.rfind("CreatePVSource",ptr.size())==vtkstd::string::npos)
      {
      this->Script(ptr.c_str());
      ptr = *(++tokIter);
      }
    else if(ptr.rfind("InitializeReadCustom",ptr.size())!=vtkstd::string::npos )
      {
      foundSource = true;

      sscanf(ptr.c_str(),"%*s %s %*s %*s \"%[^\"]\" \"%[^\"]",srcTclName,moduleName,path);

      if(macroFlag)
        {
        src = this->GetReaderForMacro(readers,moduleName,path);
        }
      else
        {
        // look for an open dataset of the same path and reader type
        // if none is open, try to open the one at the specified path automatically for user
        // if that fails, prompt user for a new data file
        src = this->GetReaderForLookmark(readers,moduleName,path);
        }

      if(src)
        {
        tclName = new char[20];
        strcpy(tclName,srcTclName);
        tclNameMap[src] = tclName;
        while(ptr.rfind("GetPVWidget",ptr.size())==vtkstd::string::npos)
          {
          ptr = *(++tokIter);
          }
        this->InitializeSourceFromScript(src,tokIter,macroFlag);
        // remove this source from the available list
        readers->RemoveItem(src);
        }
      else
        {
        // this really shouldn't fail, but if it does, quit
        break;
        }

      ptr = *(++tokIter);
      }
    else if(ptr.rfind("CreatePVSource",ptr.size())!=vtkstd::string::npos )
      {
      foundSource = true;
      sscanf(ptr.c_str(),"%*s %*s %*s %*s %[^]]",moduleName);

      // ASSUMPTION: the first pvwidget listed in script will be an input if it is a filter
      
      // mark the current line with second iterator
      tokMrkr = tokIter;
      while(ptr.rfind("GetPVWidget",ptr.size())==vtkstd::string::npos)
        {
        ptr = *(++tokIter);
        }

      if(ptr.rfind("inputs",ptr.size())!=vtkstd::string::npos)
        {
        // group input widget
        // execute the createpvsource command
        ptr = *tokMrkr;
        while(tokMrkr!=tokIter)
          {
          this->Script(ptr.c_str());   
          ptr = *(++tokMrkr);
          }

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
        while(ptr.rfind("AcceptCallback",ptr.size())==vtkstd::string::npos)
          {
          ptr = *(++tokIter);
          }
        this->Script(ptr.c_str());
        }
      else if(ptr.rfind("Input",ptr.size())!=vtkstd::string::npos)
        {
        // input menu widget
        ptr = *(++tokIter);
        sscanf(ptr.c_str(),"%*s %*s %s",srcTclName);
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
        ptr = *tokMrkr;
        while(tokMrkr!=tokIter)
          {
          this->Script(ptr.c_str());
          ptr = *(++tokMrkr);
          }
        }
      else
        {
        // no input widget means this is a source
        if(macroFlag)
          {
          // if multiple sources are open and the lookmark creates multiple source, ask user
          // else if the lookmark is single sources, use the currently selected one
          src = this->GetSourceForMacro(sources,moduleName);
          }
        else
          {
          src = this->GetSourceForLookmark(sources,moduleName);
          }

        if(src)
          {
          tclName = new char[20]; 
          strcpy(tclName,srcTclName);
          tclNameMap[src] = tclName;
          this->InitializeSourceFromScript(src,tokIter,macroFlag);
          // remove this source from the available list
          sources->RemoveItem(src);
          }
        else
          {
          break;
          }
        }

      ptr = *(++tokIter);

      }
    else if(ptr.rfind("AcceptCallback",ptr.size())!=vtkstd::string::npos )
      {
      // the current source would be the filter just created
      src = this->GetPVApplication()->GetMainWindow()->GetCurrentPVSource();

      // append the lookmark name to its filter name : strip off any characters after '-' because a lookmark name could already have been appended to this filter name
      srcLabel = src->GetLabel();
      ret = srcLabel.find_last_of('-',srcLabel.size());
      if(ret!=vtkstd::string::npos)
        {
        srcLabel.erase(ret,srcLabel.size()-ret);
        }
      srcLabel.append("-");
      srcLabel.append(this->GetName());
      src->SetLabel(srcLabel.c_str());

      //add all pvsources created by this lmk to its collection
      this->AddPVSource(src);
      src->SetLookmark(this);

      this->Script(ptr.c_str());
      ptr = *(++tokIter);
      }
    else if(ptr.rfind("SetCameraState",ptr.size())!=vtkstd::string::npos && macroFlag)
      {
      ptr = *(++tokIter);
      }
    else
      {
      // encode special '%' character with a preceeding '%' for printf cmd in call to Script()
      if(strstr(ptr.c_str(),"%"))
        {
        ptr1 = (char*)ptr.c_str();
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
        if(ptr.rfind(iter->second,ptr.size())!=vtkstd::string::npos)
          {
          if(ptr.rfind("SetVisibility",ptr.size())!=vtkstd::string::npos)
            {
            sscanf(ptr.c_str(),"%*s %*s %d",&ival);
            iter->first->SetVisibility(ival);
            }
          if(ptr.rfind("ShowVolumeAppearanceEditor",ptr.size())!=vtkstd::string::npos)
            {
            this->InitializeVolumeAppearanceEditor(iter->first,tokIter);
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
        if(ptr.rfind("ColorByArray",ptr.size())!=vtkstd::string::npos)
          {
          string1.erase(string1.find_first_of('[',0),1);
          beg = string1.rfind("GetPVOutput]",string1.size());
          string1.erase(beg,13);
          }

        this->Script(string1.c_str());
        }
      executeCmd = true;

      tokIter++;
      if(tokIter>=tokens.end())
        break;
      ptr = *tokIter;

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
  readers->Delete();
  sources->Delete();
}

void vtkPVLookmark::InitializeVolumeAppearanceEditor(vtkPVSource *src, vtkstd::vector<vtkstd::string>::iterator &it)
{
  double fval1,fval2,fval3,fval4;
  int ival;

  vtkPVVolumeAppearanceEditor *vol = this->GetPVApplication()->GetMainWindow()->GetVolumeAppearanceEditor();

  vtkstd::string ptr = *it;
  // needed to display edit volume appearance setting button:
  src->GetPVOutput()->DrawVolume();
  src->GetPVOutput()->ShowVolumeAppearanceEditor();

  ptr = *(++it);
  vol->RemoveAllScalarOpacityPoints();

  ptr = *(++it);
  while(ptr.rfind("AppendScalarOpacityPoint",ptr.size())!=vtkstd::string::npos)
    {
    sscanf(ptr.c_str(),"%*s %*s %lf %lf",&fval1,&fval2);
    vol->AppendScalarOpacityPoint(fval1,fval2);
    ptr = *(++it);
    }

  sscanf(ptr.c_str(),"%*s %*s %lf ",&fval1);
  vol->SetScalarOpacityUnitDistance(fval1);

  ptr = *(++it);
  vol->RemoveAllColorPoints();

  ptr = *(++it);
  while(ptr.rfind("AppendColorPoint",ptr.size())!=vtkstd::string::npos)
    {
    sscanf(ptr.c_str(),"%lf %lf %lf %lf",&fval1,&fval2,&fval3,&fval4);
    vol->AppendColorPoint(fval1,fval2,fval3,fval4);
    ptr = *(++it);
    }

  sscanf(ptr.c_str(),"%*s %*s %d",&ival);
  vol->SetHSVWrap(ival);

  ptr = *(++it);
  sscanf(ptr.c_str(),"%*s %*s %d",&ival);
  vol->SetColorSpace(ival);
}


void vtkPVLookmark::InitializeSourceFromScript(vtkPVSource *source, vtkstd::vector<vtkstd::string>::iterator &tokIter, int macroFlag)
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
  vtkPVThumbWheel *thumbWheel;
#ifdef PARAVIEW_USE_EXODUS
  vtkPVBasicDSPFilterWidget *dspWidget;
#endif

  vtkPVWidget *pvWidget;
  vtkstd::string ptr;
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
  vtkstd::string::size_type beg;
  vtkstd::string::size_type end;

/*
  ptr = strtok(NULL,"\r\n");

  while(!strstr(ptr,"GetPVWidget"))
    {
    ptr = strtok(NULL,"\r\n");
    }
*/
  ptr = *tokIter;

  //loop through collection till found, operate accordingly leaving else statement with ptr one line past 
  vtkCollectionIterator *it = source->GetWidgets()->NewIterator();
  it->InitTraversal();
  
  while( !it->IsDoneWithTraversal() )
    {
    pvWidget = static_cast<vtkPVWidget*>(it->GetCurrentObject());
    sscanf(ptr.c_str(),FifthToken_WrappedString,sval);
    if(!strcmp(sval,pvWidget->GetTraceHelper()->GetObjectName()))
      {
      if((scale = vtkPVScale::SafeDownCast(pvWidget)))
        {
        ptr = *(++tokIter);
        if(!macroFlag)
          {
          sscanf(ptr.c_str(),ThirdToken_Float,&fval);
          scale->SetValue(fval);
          }
        ptr = *(++tokIter);
        }
      else if((arraySelection = vtkPVArraySelection::SafeDownCast(pvWidget)))
        { 
        ptr = *(++tokIter);
        while(ptr.rfind("SetArrayStatus",ptr.size())!=vtkstd::string::npos)
          {
          sscanf(ptr.c_str(),ThirdAndFourthTokens_WrappedStringAndInt,sval,&ival);
          //only turn the variable on, not off, because some other filter might be using the variable
          if(ival)
            arraySelection->SetArrayStatus(sval,ival);
          ptr = *(++tokIter);  
          }
        arraySelection->ModifiedCallback();
        arraySelection->Accept();
        }
      else if((labeledToggle = vtkPVLabeledToggle::SafeDownCast(pvWidget)))
        {
        ptr = *(++tokIter);
        sscanf(ptr.c_str(),ThirdToken_Int,&ival);
        labeledToggle->SetSelectedState(ival);
        labeledToggle->ModifiedCallback();
        ptr = *(++tokIter);
        }
      else if((selectWidget = vtkPVSelectWidget::SafeDownCast(pvWidget)))
        {
        //get the third token of the next line and take off brackets which will give you the value of select widget:
        ptr = *(++tokIter);
        sscanf(ptr.c_str(),ThirdToken_WrappedString,sval);
        selectWidget->SetCurrentValue(sval); 
        //ignore next line
        ptr = *(++tokIter);
        ptr = *(++tokIter);
        sscanf(ptr.c_str(),FifthToken_WrappedString,sval);
        arraySelection = vtkPVArraySelection::SafeDownCast(selectWidget->GetPVWidget(sval));
        ptr = *(++tokIter);
        while(ptr.rfind("SetArrayStatus",ptr.size())!=vtkstd::string::npos)
          {
          sscanf(ptr.c_str(),ThirdAndFourthTokens_WrappedStringAndInt,sval,&ival);
          arraySelection->SetArrayStatus(sval,ival);
          ptr = *(++tokIter); 
          }
        //arraySelection->Accept();
        arraySelection->ModifiedCallback();
        } 
      else if((selectTimeSet = vtkPVSelectTimeSet::SafeDownCast(pvWidget)))
        {
        ptr = *(++tokIter);
        sscanf(ptr.c_str(),ThirdToken_OptionalWrappedString,sval);
        selectTimeSet->SetTimeValueCallback(sval);
        selectTimeSet->ModifiedCallback();
        ptr = *(++tokIter);
        }
      else if((vectorEntry = vtkPVVectorEntry::SafeDownCast(pvWidget)))
        {
        // could be up to 6 fields
        ptr = *(++tokIter);
        ival = sscanf(ptr.c_str(),ThirdThruEighthToken_String,ve[0], ve[1], ve[2], ve[3], ve[4], ve[5]);

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
        ptr = *(++tokIter);
        }
      else if((fileEntry = vtkPVFileEntry::SafeDownCast(pvWidget)))
        {
        ptr = *(++tokIter);
        ptr = *(++tokIter);
        }
      else if((selectionList = vtkPVSelectionList::SafeDownCast(pvWidget)))
        {
        ptr = *(++tokIter);
        sscanf(ptr.c_str(),ThirdToken_Int,&ival);
        selectionList->SetCurrentValue(ival);
        selectionList->ModifiedCallback();
        ptr = *(++tokIter);
        }
      else if((thumbWheel = vtkPVThumbWheel::SafeDownCast(pvWidget)))
        {
        ptr = *(++tokIter);
        sscanf(ptr.c_str(),ThirdToken_Float,&fval);
        thumbWheel->SetValue(fval);
        thumbWheel->ModifiedCallback();
        ptr = *(++tokIter);
        }
      else if((stringEntry = vtkPVStringEntry::SafeDownCast(pvWidget)))
        {
        if(macroFlag)
          {
          ptr = *(++tokIter);
          ptr = *(++tokIter);
          }
        else
          {
          ptr = *(++tokIter);
          beg = ptr.find_first_of('{',0);
          if(beg!=vtkstd::string::npos)
            {
            end = ptr.find_last_of('}',ptr.size());
            //sscanf(ptr.c_str(),ThirdToken_WrappedString,sval);
            ptr = ptr.substr(beg+1,end-beg-1);
            stringEntry->SetValue(ptr.c_str());
            stringEntry->ModifiedCallback();
            }
          ptr = *(++tokIter);
          }
        }
      else if((minMaxWidget = vtkPVMinMax::SafeDownCast(pvWidget)))
        {
        // If this is a macro, don't initialize since the two datasets could be made up of a 
        // different range of files. 
        if(macroFlag)
          {
          ptr = *(++tokIter);
          ptr = *(++tokIter);
          ptr = *(++tokIter);
          }
        else
          {
          ptr = *(++tokIter);
          sscanf(ptr.c_str(),ThirdToken_Float,&fval);
          minMaxWidget->SetMaxValue(fval);
          ptr = *(++tokIter);
          sscanf(ptr.c_str(),ThirdToken_Float,&fval);
          minMaxWidget->SetMinValue(fval);
          minMaxWidget->ModifiedCallback();
          ptr = *(++tokIter);
          }
        }
#ifdef PARAVIEW_USE_EXODUS
      else if((dspWidget = vtkPVBasicDSPFilterWidget::SafeDownCast(pvWidget)))
        {
        ptr = *(++tokIter);
        sscanf(ptr.c_str(),ThirdToken_String,sval);
        dspWidget->ChangeDSPFilterMode(sval);
        ptr = *(++tokIter);
        sscanf(ptr.c_str(),ThirdToken_String,sval);
        dspWidget->ChangeCutoffFreq(sval);
        ptr = *(++tokIter);
        sscanf(ptr.c_str(),ThirdToken_String,sval);
        dspWidget->SetFilterLength(atoi(sval));
        ptr = *(++tokIter);
        }
#endif
      else   //if we do not support this widget yet, advance and break loop
        {
        ptr = *(++tokIter);
        }
      }
    it->GoToNextItem();
    }
  //widget in state file is not in widget collection
  if(i==source->GetWidgets()->GetNumberOfItems())
    {
    ptr = *(++tokIter);
    }
  it->Delete();

  while(ptr.rfind("AcceptCallback",ptr.size())==vtkstd::string::npos)
    {
    ptr = *(++tokIter);
    }

  source->AcceptCallback();

  ptr = *(++tokIter);

  // ignore comments
  while(ptr[0]=='#')
    {
    ptr = *(++tokIter);
    }

  // this line sets the partdisplay variable
  sscanf(ptr.c_str(),SecondToken_String,displayName);

  ptr = *(++tokIter);

  vtkSMDisplayProxy *display = source->GetDisplayProxy();

  while(ptr.rfind(displayName,ptr.size())!=vtkstd::string::npos) 
    {
    if(ptr.rfind("UpdateVTKObjects",ptr.size())!=vtkstd::string::npos)
      {
      display->UpdateVTKObjects();
      ptr = *(++tokIter);
      continue;
      }

    sscanf(ptr.c_str(),ThirdToken_RightBracketedString,sval);

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
        sscanf(ptr.c_str(),FifthAndSixthToken_IntAndInt,&val,&ival);
        ivp->SetElement(val,ival);
        }
      else if (dvp)
        {
        sscanf(ptr.c_str(),FifthAndSixthToken_IntAndFloat,&val,&fval);
        dvp->SetElement(val,fval);
        }
      else if (svp)
        {
        sscanf(ptr.c_str(),FifthAndSixthToken_IntAndWrappedString,&val,sval);
        svp->SetElement(val,sval);
        }

      break;
      }

    iter->Delete();

    ptr = *(++tokIter);

    }

  // make sure that the data information is up to date
//  source->UpdateVTKObjects();
//  source->InvalidateDataInformation();

  if(ptr.rfind("ColorByArray",ptr.size())!=vtkstd::string::npos)
    {
    sscanf(ptr.c_str(),FourthAndFifthToken_WrappedStringAndInt,sval,&ival);
    source->ColorByArray(sval, ival);
    }
  else if(ptr.rfind("VolumeRenderByArray",ptr.size())!=vtkstd::string::npos)
    {
    sscanf(ptr.c_str(),FourthAndFifthToken_WrappedStringAndInt,sval,&ival);
    source->VolumeRenderByArray(sval, ival);
    }
  else if(ptr.rfind("ColorByProperty",ptr.size())!=vtkstd::string::npos)
    {
    source->GetPVOutput()->ColorByProperty();
    }

  source->GetPVOutput()->Update();
  source->GetPVOutput()->UpdateColorGUI();
  source->GetPVOutput()->DataColorRangeCallback();
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
