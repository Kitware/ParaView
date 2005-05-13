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
#include "vtkSMDoubleVectorProperty.h"
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

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVLookmark );
vtkCxxRevisionMacro(vtkPVLookmark, "1.4");

//----------------------------------------------------------------------------
vtkPVLookmark::vtkPVLookmark()
{
  this->ImageData = NULL;
  this->StateScript= NULL;
  this->CenterOfRotation = new float[3];
  this->Sources = vtkPVSourceCollection::New();
}

//----------------------------------------------------------------------------
vtkPVLookmark::~vtkPVLookmark()
{

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

  if(this->IsLockedToDataset()==0)
    {
    this->ViewLookmarkWithCurrentDataset();
    this->SetLookmarkIconCommand();
    return;
    }

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

  delete [] decodedImageData;
  decoder->Delete();
  icon->Delete();
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

    this->ParseAndExecuteStateScript(src,temp_script,0);
//    this->CreateLookmarkCallback();

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
  //create and store a new session state file
  this->StoreStateScript();
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
    vtkKWIcon::IMAGE_OPTION_FLIP_V);

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
  int encodedImageSize = imageSize*1.5;
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
  this->LmkIcon->SetBind(this, "<Button-1>", "View");
  this->LmkIcon->SetBind(this, "<Double-1>", "View");
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
    this->LmkIcon->SetImageOption(icon);
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
  float tvalue=0;
  char *ptr2;
  char *field;
  double fval,xval,yval,zval; 
  int i=0;
  char *name = new char[50];
  char *data = new char[50];
  char *tok;
  int val;
  char *readername=NULL; 
  char *ptr1;
  char *ptr;
  char cmd[200];

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
            tok = strstr(ptr,"SetValue");
            tok+=9;
            tvalue = atof(tok);
            scale->SetValue(tvalue);
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
            val = this->GetIntegerScalarWidgetValue(ptr);
            labeledToggle->SetState(val);
//            labeledToggle->Accept();
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
            vectorEntry->SetValue(this->GetVectorEntryValue(ptr));
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
            val = this->GetIntegerScalarWidgetValue(ptr);
            selectionList->SetCurrentValue(val);
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
            minMaxWidget->SetMaxValue(this->GetIntegerScalarWidgetValue(ptr));
            ptr = strtok(NULL,"\r\n");
            minMaxWidget->SetMinValue(this->GetIntegerScalarWidgetValue(ptr));
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

      // next line will either contain "ColorByProperty" "ColorByPointField" or "ColorByCellField"
      vtkPVDisplayGUI *pvData = reader->GetPVOutput(); 
      ptr = strtok(NULL,"\r\n");

      if(strstr(ptr,"ColorByCellField"))
        {
        field = this->GetFieldNameAndValue(ptr,&val);
        //pvData->ColorByCellField(field,val);
        pvData->ColorByArray(field, vtkSMDisplayProxy::CELL_FIELD_DATA);
        }
      else if(strstr(ptr,"ColorByPointField"))
        {
        field = this->GetFieldNameAndValue(ptr,&val);
        //pvData->ColorByPointField(field,val);
        pvData->ColorByArray(field, vtkSMDisplayProxy::POINT_FIELD_DATA);
        
        }
      else if(strstr(ptr,"ColorByProperty"))
        {
        pvData->ColorByProperty();
        }

      ptr = strtok(NULL,"\r\n");

      if(strstr(ptr,"DrawVolume"))
        {
        pvData->DrawVolume();
        ptr = strtok(NULL,"\r\n");
        if(strstr(ptr,"VolumeRenderPointField"))
          {
          field = this->GetFieldNameAndValue(ptr,&val);
          //pvData->VolumeRenderPointField(field,val);
          pvData->VolumeRenderByArray(field, vtkSMDisplayProxy::POINT_FIELD_DATA);
          }
        else if(strstr(ptr,"VolumeRenderCellField"))
          {
          field = this->GetFieldNameAndValue(ptr,&val);
          //pvData->VolumeRenderCellField(field,val);
          pvData->VolumeRenderByArray(field, vtkSMDisplayProxy::CELL_FIELD_DATA);
          }
        ptr = strtok(NULL,"\r\n");
        }
      // this line sets the partdisplay variable
      ptr+=4;
      ptr2 = ptr;
      while(*ptr2!=' ')
        ptr2++;
      *ptr2='\0';
      strcpy(data,ptr);

      vtkSMDisplayProxy *display = reader->GetDisplayProxy();
      ptr = strtok(NULL,"\r\n");
      while(strstr(ptr,data)) 
        {
        if(strstr(ptr,"SetColor"))
          {
          this->GetDoubleVectorWidgetValue(ptr,&xval,&yval,&zval);
          display->SetColorCM(xval,yval,zval);
          }
        else if(strstr(ptr,"SetRepresentation"))
          {
          val = this->GetIntegerScalarWidgetValue(ptr);
          display->SetRepresentationCM(val); 
          }
        else if(strstr(ptr,"SetUseImmediateMode"))
          {
          val = this->GetIntegerScalarWidgetValue(ptr);
          display->SetImmediateModeRenderingCM(val); 
          }
        else if(strstr(ptr,"SetScalarVisibility"))
          {
          val = this->GetIntegerScalarWidgetValue(ptr);
          display->SetScalarVisibilityCM(val); 
          }
        else if(strstr(ptr,"SetDirectColorFlag"))
          {
          val = this->GetIntegerScalarWidgetValue(ptr);
          // when DirectColorFlag = 0,
          // color mode is Default (=1).
          // when DirectColorFlag = 1
          // color mode is MapScalars (=0).
          display->SetColorModeCM(!val); 
          }
        else if(strstr(ptr,"SetInterpolateColorsFlag"))
          {
          // This is "InterpolateColors" while property
          // "InterpolateColorsBeforeMapping". 
          // These are opposite concepts.
          val = this->GetIntegerScalarWidgetValue(ptr);
          display->SetInterpolateScalarsBeforeMappingCM(val); 
          }
        else if(strstr(ptr,"SetInterpolation"))
          {
          val = this->GetIntegerScalarWidgetValue(ptr);
          display->SetInterpolationCM(val); 
          }
        else if(strstr(ptr,"SetPointSize"))
          {
          val = this->GetIntegerScalarWidgetValue(ptr);
          display->SetPointSizeCM(val); 
          }
        else if(strstr(ptr,"SetLineWidth"))
          {
          val = this->GetIntegerScalarWidgetValue(ptr);
          display->SetLineWidthCM(val); 
          }
        else if(strstr(ptr,"SetOpacity"))
          {
          fval = this->GetDoubleScalarWidgetValue(ptr);
          display->SetOpacityCM(fval); 
          }
        else if(strstr(ptr,"SetTranslate"))
          {
          this->GetDoubleVectorWidgetValue(ptr,&xval,&yval,&zval);
          display->SetPositionCM(xval,yval,zval); 
          }
        else if(strstr(ptr,"SetScale"))
          {
          this->GetDoubleVectorWidgetValue(ptr,&xval,&yval,&zval);
          display->SetScaleCM(xval,yval,zval); 
          }
        else if(strstr(ptr,"SetOrigin"))
          {
          this->GetDoubleVectorWidgetValue(ptr,&xval,&yval,&zval);
          display->SetOriginCM(xval,yval,zval); 
          }
        else if(strstr(ptr,"SetOrientation"))
          {
          this->GetDoubleVectorWidgetValue(ptr,&xval,&yval,&zval);
          display->SetOrientationCM(xval,yval,zval); 
          }   
        ptr = strtok(NULL,"\r\n");
        }
      break;
      }
    else
      {
      ptr = strtok(NULL,"\r\n");
      }
    }

  if(strstr(ptr,readername) && strstr(ptr,"SetCubeAxesVisibility"))
    {
    val = this->GetIntegerScalarWidgetValue(ptr);
    reader->SetCubeAxesVisibility(val);
    ptr = strtok(NULL,"\r\n");
    }
  if(strstr(ptr,readername) && strstr(ptr,"SetPointLabelVisibility"))
    {
    val = this->GetIntegerScalarWidgetValue(ptr);
    reader->SetPointLabelVisibility(val);
    ptr = strtok(NULL,"\r\n");
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
//      src->SetPVLookmark(this);
      delete [] srcLabel;
      }

    if(strstr(ptr,"SetVisibility") && strstr(ptr,readername))
      {
      val = this->GetIntegerScalarWidgetValue(ptr);
      reader->SetVisibility(val);
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
  delete [] data;
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
  char *ptr2 = ptr;
  while(*ptr2!=' ')
    ptr2++;
  ptr2++;
  while(*ptr2!=' ')
    ptr2++;
  ptr2++;
  return atoi(ptr2);
}

//----------------------------------------------------------------------------
double vtkPVLookmark::GetDoubleScalarWidgetValue(char *ptr)
{
  char *ptr2 = ptr;
  while(*ptr2!=' ')
    ptr2++;
  ptr2++;
  while(*ptr2!=' ')
    ptr2++;
  ptr2++;
  return atof(ptr2);
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
}
