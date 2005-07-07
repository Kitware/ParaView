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
#include "vtkKWFrameLabeled.h"
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

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVLookmark );
vtkCxxRevisionMacro(vtkPVLookmark, "1.15");


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
  vtkPVSource *reader;
  vtkPVSource *currentSource = win->GetCurrentPVSource();

  this->UnsetLookmarkIconCommand();

  //try to delete preexisting filters belonging to this lookmark - for when lmk has been visited before in this session
  this->DeletePVSources();

  // this prevents other filters' visibility from disturbing the lookmark view
  this->TurnFiltersOff();

  //  If the lookmark is clicked and this checkbox is checked
  //    and the dataset is currently loaded - same behavior as now
  //    and the dataset is not loaded but exists on disk at the stored path - load the dataset into paraview without prompting the user
  //    and the dataset is not loaded and does not exist on disk at the stored path - prompt the user for dataset; perhaps have an "update dataset path" option
  //    ignores that fact that there might be other datasets loaded in paraview at the moment

  if(!(reader = this->SearchForDefaultDatasetInSourceList()))
    {
    FILE *file;
    if( (file = fopen(this->Dataset,"r")) != NULL)
      {
      fclose(file);
      //look for dataset at stored path and open automatically if found
      if(win->Open(this->Dataset) == VTK_OK)
        {
        reader = win->GetCurrentPVSource();
        reader->AcceptCallback();
        }
      else
        {
        this->SetLookmarkIconCommand();
        return;
        }
      }
    else
      {
      //ask user for dataset
      vtkKWMessageDialog::PopupMessage(
        this->GetPVApplication(), win, "Could Not Find Default Data Set", 
        "You will now be asked to select a different data set.", 
        vtkKWMessageDialog::ErrorIcon);

      win->OpenCallback();
      reader = win->GetCurrentPVSource();
      if(reader==currentSource || !reader->IsA("vtkPVReaderModule"))
        {
        this->SetLookmarkIconCommand();
        return;
        }
      reader->AcceptCallback();

      this->Script("focus %s",this->GetWidgetName());

      vtkPVReaderModule *rm = vtkPVReaderModule::SafeDownCast(reader);
      this->SetDataset((char *)rm->GetFileEntry()->GetValue());
      }
    }

  // needed? since done in Parse method?
  //set the reader to the current pv source so that the input will automatically be set to the reader
  win->SetCurrentPVSource(reader);

  char *temp_script = new char[strlen(this->StateScript)+1];
  strcpy(temp_script,this->GetStateScript());

  this->GetTraceHelper()->AddEntry("$kw(%s) View",
                      this->GetTclName());

  this->ParseAndExecuteStateScript(reader,temp_script,1);

  // this is needed to update the eyeballs based on recent changes to visibility of filters
  // handle case where there is no source in the source window
  pvs = win->GetCurrentPVSource();
  if(pvs && pvs->GetNotebook())
    this->GetPVRenderView()->UpdateNavigationWindow(pvs,0);

  this->SetLookmarkIconCommand();
//  win->SetCenterOfRotation(this->CenterOfRotation);

  delete [] temp_script;
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
  int imageSize = this->Width*this->Height*this->PixelSize;
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
  char *buf = new char[300];
  char *stateScript;
  ostrstream state;
  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();

  win->SetSaveVisibleSourcesOnlyFlag(1);
  win->SaveState("tempLookmarkState.pvs");
  win->SetSaveVisibleSourcesOnlyFlag(0);

  //read the session state file in to a new vtkPVLookmark
  if((lookmarkScript = fopen("tempLookmarkState.pvs","r")) != NULL)
    {
    while(fgets(buf,300,lookmarkScript))
      state << buf;
    }
  state << ends;
  fclose(lookmarkScript);
  delete [] buf;
  stateScript = new char[strlen(state.str())+1];
  strcpy(stateScript,state.str());
  this->SetStateScript(stateScript);
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

  // execute state script stored with this lookmark except
  // don't use reader assigned to lookmark, use the current one
  vtkPVSource *src, *temp;
  src = win->GetCurrentPVSource();
  while((temp = src->GetPVInput(0)))
    src = temp;
  if(src->IsA("vtkPVReaderModule"))
    {
    char *temp_script = new char[strlen(this->GetStateScript())+1];
    strcpy(temp_script,this->GetStateScript());
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
      this->ErrorEventTag = this->GetPVApplication()->GetMainWindow()->AddObserver(
        vtkKWEvent::ErrorMessageEvent, this->Observer);
      }

    this->ParseAndExecuteStateScript(src,temp_script,0);
//    this->CreateLookmarkCallback();

    if (this->ErrorEventTag)
      {
      this->GetPVApplication()->GetMainWindow()->RemoveObserver(this->ErrorEventTag);
      this->ErrorEventTag = 0;
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
    this->LmkIcon->SetBind(this, "<Button-1>", "ViewLookmarkWithCurrentDataset");
    this->LmkIcon->SetBind(this, "<Double-1>", "ViewLookmarkWithCurrentDataset");
    }
  else
    {
    this->LmkIcon->SetBind(this, "<Button-1>", "View");
    this->LmkIcon->SetBind(this, "<Double-1>", "View");
    }
}

//----------------------------------------------------------------------------
void vtkPVLookmark::UnsetLookmarkIconCommand()
{
  this->LmkIcon->UnsetBind("<Button-1>");
  this->LmkIcon->UnsetBind("<Double-1>");
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

vtkPVSource *vtkPVLookmark::SearchForDefaultDatasetInSourceList()
{
  vtkPVSource *pvs;
  vtkPVSource *reader = NULL;
  char *targetName;
  vtkPVReaderModule *mod;

  vtkPVSourceCollection *col = this->GetPVApplication()->GetMainWindow()->GetSourceList("Sources");
  if (col == NULL)
    {
    return NULL;
    }
  vtkCollectionIterator *it = col->NewIterator();
  it->InitTraversal();
  while ( !it->IsDoneWithTraversal() )
    {
    pvs = static_cast<vtkPVSource*>( it->GetCurrentObject() );
    pvs->SetVisibility(0);
    if(pvs->IsA("vtkPVReaderModule"))
      {
      mod = vtkPVReaderModule::SafeDownCast(pvs);
      targetName = (char *)mod->GetFileEntry()->GetValue();
      if(!strcmp(targetName,this->GetDataset()))
        {
        reader = pvs;
        }
      }
    it->GoToNextItem();
    }
  it->Delete();

  return reader;
}


//----------------------------------------------------------------------------
void vtkPVLookmark::ParseAndExecuteStateScript(vtkPVSource *reader,char *script, int useDatasetFlag)
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

  vtkPVSource *src;
  vtkPVWidget *pvWidget;
  char *ptr;
  char *ptr1;
  char *ptr2;
  char *field;
  double fval; 
  int i=0;
  char *name = new char[50];
  char data[50];
  char *tok;
  int val;
  char *readername=NULL; 
  char cmd[200];
  int ival;
  char sval[100];
  char str[100];

  vtkPVWindow *win = this->GetPVApplication()->GetMainWindow();
  vtkPVDisplayGUI *pvData = reader->GetPVOutput();

  this->Script("[winfo toplevel %s] config -cursor watch", 
                this->GetWidgetName());

  this->GetPVRenderView()->StartBlockingRender();

  ptr = strtok(script,"\r\n");

  while(ptr)
    {
    if(!readername && !strstr(ptr,"InitializeReadCustom"))
      {
      this->Script(ptr);
      ptr = strtok(NULL,"\r\n");
      }
    else if(strstr(ptr,"InitializeReadCustom"))
      {
      ptr1 = this->GetReaderTclName(ptr);
      readername = new char[25];
      strcpy(readername,ptr1);
      ptr = strtok(NULL,"\r\n");
      }
    else if(strstr(ptr,"GetPVWidget"))
      {
      //loop through collection till found, operate accordingly leaving else statement with ptr one line past 
      vtkCollectionIterator *it = reader->GetWidgets()->NewIterator();
      it->InitTraversal();
 
      for (i = 0; i < reader->GetWidgets()->GetNumberOfItems(); i++)
        {
        pvWidget = static_cast<vtkPVWidget*>(it->GetCurrentObject());
        if(strstr(ptr,pvWidget->GetTraceHelper()->GetObjectName()))
          {
          if((scale = vtkPVScale::SafeDownCast(pvWidget)) && useDatasetFlag==1)
            {
            ptr = strtok(NULL,"\r\n");
            sscanf(ptr,"%*s %*s %lf",&fval);
            scale->SetValue(fval);
            ptr = strtok(NULL,"\r\n");
            }
          else if((arraySelection = vtkPVArraySelection::SafeDownCast(pvWidget)))
            { 
            ptr = strtok(NULL,"\r\n");
            while(strstr(ptr,"SetArrayStatus"))
              {
              val = this->GetArrayStatus(name,ptr);
              //only turn the variable on, not off, because some other filter might be using the variable
              if(val)
                arraySelection->SetArrayStatus(name,val);
              ptr = strtok(NULL,"\r\n");  
              }
            arraySelection->Accept();
            arraySelection->ModifiedCallback();
            }
          else if((labeledToggle = vtkPVLabeledToggle::SafeDownCast(pvWidget)))
            {
            ptr = strtok(NULL,"\r\n");
            sscanf(ptr,"%*s %*s %d",&ival);
            labeledToggle->SetState(ival);
            labeledToggle->ModifiedCallback();
            ptr = strtok(NULL,"\r\n");
            }
          else if((selectWidget = vtkPVSelectWidget::SafeDownCast(pvWidget)))
            {
            //get the third token of the next line and take off brackets which will give you the value of select widget:
            ptr = strtok(NULL,"\r\n");
            tok = this->GetFieldName(ptr);
            selectWidget->SetCurrentValue(tok); 
            //ignore next line
            ptr = strtok(NULL,"\r\n");
            ptr = strtok(NULL,"\r\n");
            tok = this->GetFieldName(ptr);
            arraySelection = vtkPVArraySelection::SafeDownCast(selectWidget->GetPVWidget(tok));
            ptr = strtok(NULL,"\r\n");
            while(strstr(ptr,"SetArrayStatus"))
              {
              val = this->GetArrayStatus(name,ptr);
              arraySelection->SetArrayStatus(name,val);
              ptr = strtok(NULL,"\r\n"); 
              }
            arraySelection->Accept();
            arraySelection->ModifiedCallback();
            } 
          else if((selectTimeSet = vtkPVSelectTimeSet::SafeDownCast(pvWidget)))
            {
            ptr = strtok(NULL,"\r\n");
            selectTimeSet->SetTimeValueCallback(this->GetStringEntryValue(ptr));
            selectTimeSet->ModifiedCallback();
            ptr = strtok(NULL,"\r\n");
            }
          else if((vectorEntry = vtkPVVectorEntry::SafeDownCast(pvWidget)))
            {
            ptr = strtok(NULL,"\r\n");
            sscanf(ptr,"%*s %*s %s",sval);
            vectorEntry->SetValue(sval);
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
            sscanf(ptr,"%*s %*s %d",&ival);
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
            sscanf(ptr,"%*s %*s %d",&ival);
            minMaxWidget->SetMaxValue(ival);
            ptr = strtok(NULL,"\r\n");
            minMaxWidget->SetMinValue(ival);
            minMaxWidget->ModifiedCallback();
            ptr = strtok(NULL,"\r\n");
            }
#ifdef PARAVIEW_USE_EXODUS
          else if((dspWidget = vtkPVBasicDSPFilterWidget::SafeDownCast(pvWidget)))
            {
            ptr = strtok(NULL,"\r\n");
            dspWidget->ChangeDSPFilterMode(this->GetStringValue(ptr));
            ptr = strtok(NULL,"\r\n");
            dspWidget->ChangeCutoffFreq(this->GetStringValue(ptr));
            ptr = strtok(NULL,"\r\n");
            dspWidget->SetFilterLength(atoi(this->GetStringValue(ptr)));
            ptr = strtok(NULL,"\r\n");
            }
#endif
          else   //if we do not support this widget yet, advance and break loop
            {
            ptr = strtok(NULL,"\r\n");
            }
          break;
          }
        it->GoToNextItem();
        }
      //widget in state file is not in widget collection
      if(i==reader->GetWidgets()->GetNumberOfItems())
        {
        ptr = strtok(NULL,"\r\n");
        }
      it->Delete();
      }
    else if(strstr(ptr,"AcceptCallback"))
      {
      //update Display page
      reader->AcceptCallback();

      ptr = strtok(NULL,"\r\n");
      // ignore comments
      while(ptr[0]=='#')
        {
        ptr = strtok(NULL,"\r\n");
        }

      // this line sets the partdisplay variable
      sscanf(ptr,"%*s %s %*s %*s",data);

      ptr = strtok(NULL,"\r\n");

      vtkSMDisplayProxy *display = reader->GetDisplayProxy();
      ptr = strtok(NULL,"\r\n");

      while(strstr(ptr,data)) 
        {
        if(strstr(ptr,"UpdateVTKObjects"))
          {
          display->UpdateVTKObjects();
          ptr = strtok(NULL,"\r\n");
          continue;
          }

        sscanf(ptr,"%*s %*s %s %*s %*s %*s",str);
        ptr1 = strstr(str,"]");
        *ptr1 = '\0';

        // Borrowed the following code to loop through properties from vtkPVSource::SaveState()

        vtkSMPropertyIterator* iter = reader->GetDisplayProxy()->NewPropertyIterator();

        // Even in state we have to use ServerManager API for displays.
        for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
          {
          vtkSMProperty* p = iter->GetProperty();
          if(strcmp(str,p->GetXMLName()))
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
            sscanf(ptr,"%*s %*s %*s %*s %d %d",&val,&ival);
            ivp->SetElement(val,ival);
            }
          else if (dvp)
            {
            sscanf(ptr,"%*s %*s %*s %*s %d %lf",&val,&fval);
            dvp->SetElement(val,fval);
            }
          else if (svp)
            {
            sscanf(ptr,"%*s %*s %*s %*s %d %s",&val,sval);
            ptr1 = sval;
            if(strstr(ptr1,"{"))
              {
              ptr1++;
              ptr2 = strstr(sval,"}");
              *ptr2 = '\0';
              }
            if(ptr1)
              {
              svp->SetElement(val,ptr1);
              }
            }

          break;
          }

        iter->Delete();

        ptr = strtok(NULL,"\r\n");

        }
      win->SetCurrentPVSource(reader);

      break;
      }
    else
      {
      ptr = strtok(NULL,"\r\n");
      }
    }

  if(strstr(ptr,"ColorByArray"))
    {
    field = this->GetFieldNameAndValue(ptr,&val);
    pvData->ColorByArray(field, val);
    }
  else if(strstr(ptr,"VolumeRenderByArray"))
    {
    field = this->GetFieldNameAndValue(ptr,&val);
    pvData->VolumeRenderByArray(field, val);
    }
  else if(strstr(ptr,"ColorByProperty"))
    {
    pvData->ColorByProperty();
    }

  reader->GetPVOutput()->Update();

  while(ptr)
    {

    // want to set the reader as the current source before this line executes (this way if the reader is the input source in the script 
    // it will be set as the input of this newly created source by default without explicitly setting it as input - which we can't do since we don't want to use its Tcl name)
    if(strstr(ptr,"CreatePVSource"))
      {
      this->GetPVApplication()->GetMainWindow()->SetCurrentPVSource(reader);
      }

    if(strstr(ptr,"AcceptCallback"))
      {
      // the current source would be the filter just created
      src = this->GetPVApplication()->GetMainWindow()->GetCurrentPVSource();

      // append the lookmark name to its filter name : strip off any characters after '-' because a lookmark name could already have been appended to this filter name
      char *srcLabel = new char[100];
      strcpy(srcLabel,src->GetLabel());
      if(strstr(srcLabel,"-"))
        {
        ptr1 = srcLabel;
        while(*ptr1!='-')
          ptr1++;
        ptr1++;
        *ptr1 = '\0';
        }
      else
        {
        strcat(srcLabel,"-");
        }    
    
      strcat(srcLabel,this->GetName());
      src->SetLabel(srcLabel);

      //add all pvsources created by this lmk to its collection
      this->AddPVSource(src);
      src->SetLookmark(this);
      delete [] srcLabel;
      }

    if(strstr(ptr,"SetVisibility") && strstr(ptr,readername))
      {
      sscanf(ptr,"%*s %*s %d",&ival);
      reader->SetVisibility(ival);
      }

    if(strstr(ptr,readername) || (useDatasetFlag==0 && strstr(ptr,"SetCameraState")))
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

    this->Script(ptr);

    ptr = strtok(NULL,"\r\n");
    }

  this->Script("[winfo toplevel %s] config -cursor {}", 
                this->GetWidgetName());

  this->GetPVRenderView()->EndBlockingRender();

  delete [] name;
  delete [] readername;
}


//----------------------------------------------------------------------------
char* vtkPVLookmark::GetVectorEntryValue(char *line)
{
  char *tok;
  
  if((tok = strstr(line,"SetValue")))
    {
    tok+=9;
    }
  
  return tok; 
}

//----------------------------------------------------------------------------
char* vtkPVLookmark::GetStringEntryValue(char *line)
{
  char *tok;
  char *ptr;

  
  if((tok = strstr(line,"{")))
    {
    tok++;
    ptr = tok;
    while(*ptr!='}')
      {
      ptr++;
      }
    *ptr = '\0';
    }
  else if((tok = strstr(line,"\"")))
    {
    tok++;
    ptr = tok;
    while(*ptr!='"')
      {
      ptr++;
      }
    *ptr = '\0';
    }
  
  return tok; 
}

//----------------------------------------------------------------------------
char* vtkPVLookmark::GetStringValue(char *line)
{
  char *ptr;
  
  ptr = line;
  ptr+=strlen(line);
  while(*ptr!=' ')
    {
    ptr--;
    }
  ptr++;

  return ptr;
}

//----------------------------------------------------------------------------
int vtkPVLookmark::GetArrayStatus(char *name, char *line)
{
  char *tok;
  char *ptr;

  tok = strstr(line,"{");
  tok++;
  ptr = tok;
  int i=0;
  while(*ptr!='}')
    {
    name[i++] = *ptr;
    ptr++;
    }
  name[i] = '\0';
  ptr+=2;
  
  return atoi(ptr); 
}

//----------------------------------------------------------------------------
int vtkPVLookmark::GetIntegerScalarWidgetValue(char *ptr)
{
  int ret;
  sscanf(ptr,"%*s %*s %*s %*s %*s %d",&ret);

  return ret;
}

//----------------------------------------------------------------------------
double vtkPVLookmark::GetDoubleScalarWidgetValue(char *ptr)
{
  double retd;
  sscanf(ptr,"%*s %*s %*s %*s %*s %lf",&retd);

  return retd;
}

//----------------------------------------------------------------------------
void vtkPVLookmark::GetDoubleVectorWidgetValue(char *ptr,double *xval,double *yval,double *zval)
{
  char *ptr2 = ptr;
  while(*ptr2!=' ')
    ptr2++;
  ptr2++;
  while(*ptr2!=' ')
    ptr2++;
  ptr2++;
  *xval = atof(ptr2);

  while(*ptr2!=' ')
    ptr2++;
  ptr2++;
  *yval = atof(ptr2);

  while(*ptr2!=' ')
    ptr2++;
  ptr2++;
  *zval = atof(ptr2);
}

//----------------------------------------------------------------------------
char* vtkPVLookmark::GetReaderTclName(char *ptr)
{
  char *ptr1;
  char *ptr2;
  ptr1 = ptr;
  ptr1+=4;
  ptr2 = ptr1;
  while(*ptr2!=' ')
    ptr2++;
  *ptr2 = '\0';
  return ptr1;
}

//----------------------------------------------------------------------------
char *vtkPVLookmark::GetFieldName(char *ptr)
{
  char *field;
  char *ptr2;
  ptr2 = ptr;
  while(*ptr2!='{')
    ptr2++;
  ptr2++;
  field = ptr2;
  while(*ptr2!='}')
    ptr2++;
  *ptr2 = '\0';
  return field;
}

//----------------------------------------------------------------------------
char *vtkPVLookmark::GetFieldNameAndValue(char *ptr, int *val)
{
  char *field;
  char *ptr2;
  ptr2 = ptr;
  while(*ptr2!='{')
    ptr2++;
  ptr2++;
  field = ptr2;
  while(*ptr2!='}')
    ptr2++;
  *ptr2 = '\0';
  ptr2+=2;
  *val = atoi(ptr2);
  return field;
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
  this->LmkMainFrame->PerformShowHideFrame();
  this->LmkMainFrame->PerformShowHideFrame();
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
