/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProbe.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVProbe.h"

#include "vtkObjectFactory.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkPVApplication.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkXYPlotActor.h"
#include "vtkXYPlotWidget.h"
#include "vtkSMPlotDisplay.h"
#include "vtkPVRenderModule.h"
#include "vtkCommand.h"
#include "vtkKWLoadSaveButton.h"
#include "vtkKWLabeledLoadSaveButton.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWListBox.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVWidget.h"
#include "vtkPVSelectWidget.h"

#include <vtkstd/string>
 
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVProbe);
vtkCxxRevisionMacro(vtkPVProbe, "1.134.2.3");

int vtkPVProbeCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

#define PV_TAG_PROBE_OUTPUT 759362

//===========================================================================
//***************************************************************************
class vtkXYPlotWidgetObserver : public vtkCommand
{
public:
  static vtkXYPlotWidgetObserver *New() 
    {return new vtkXYPlotWidgetObserver;};

  vtkXYPlotWidgetObserver()
    {
      this->PVProbe = 0;
    }

  virtual void Execute(vtkObject* wdg, unsigned long event,  
                       void* calldata)
    {
      if ( this->PVProbe )
        {
        this->PVProbe->ExecuteEvent(wdg, event, calldata);
        }
    }

  vtkPVProbe* PVProbe;
};

//----------------------------------------------------------------------------
vtkPVProbe::vtkPVProbe()
{
  this->CommandFunction = vtkPVProbeCommand;

  this->SelectedPointFrame = vtkKWWidget::New();
  this->PointDataLabel = vtkKWLabel::New();
  
  this->ShowXYPlotToggle = vtkKWCheckButton::New();
  this->SaveButton = vtkKWLabeledLoadSaveButton::New(); 

  this->ProbeFrame = vtkKWWidget::New();
  this->FieldsSelection = vtkKWListBox::New();
  
  this->XYPlotWidget = 0;
  this->XYPlotObserver = NULL;
  
  this->ReplaceInputOff();

  // Special ivar in vtkPVSource just for this subclass.
  // We cannot process inputs that have more than one part.
  this->RequiredNumberOfInputParts = 1;
  
  this->PlotDisplay = vtkSMPlotDisplay::New();
  this->DisplayHasBeenAddedToTheRenderModule = 0;
}

//----------------------------------------------------------------------------
vtkPVProbe::~vtkPVProbe()
{  
  if (this->GetPVApplication() 
   && this->GetPVApplication()->GetProcessModule()->GetRenderModule())
    {
    this->GetPVApplication()->GetProcessModule()->
      GetRenderModule()->RemoveDisplay(this->PlotDisplay);
    }

  this->PlotDisplay->Delete();
  this->PlotDisplay = NULL;

  if ( this->XYPlotWidget )
    {
    this->XYPlotWidget->SetEnabled(0);
    this->XYPlotWidget->SetInteractor(NULL);
    this->XYPlotWidget->Delete();
    this->XYPlotWidget = 0;
    }
  if (this->XYPlotObserver)
    {
    this->XYPlotObserver->Delete();
    this->XYPlotObserver = NULL;
    }
    
  this->SelectedPointFrame->Delete();
  this->SelectedPointFrame = NULL;
  this->PointDataLabel->Delete();
  this->PointDataLabel = NULL;  
  
  this->ShowXYPlotToggle->Delete();
  this->ShowXYPlotToggle = NULL;
  this->SaveButton->Delete();
  this->SaveButton =  NULL;
  
  this->ProbeFrame->Delete();
  this->ProbeFrame = NULL;  
  this->FieldsSelection->Delete();
  this->FieldsSelection = NULL;
}

//----------------------------------------------------------------------------
void vtkPVProbe::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;
  
  this->PlotDisplay->SetProcessModule(pm);
  this->vtkPVSource::CreateProperties();
  // We do not support probing groups and multi-block objects. Therefore,
  // we use the first VTKSource id.
  stream << vtkClientServerStream::Invoke 
         <<  this->GetVTKSourceID(0) << "SetSpatialMatch" << 2
         << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::DATA_SERVER, stream);

  this->ProbeFrame->SetParent(this->ParameterFrame->GetFrame());
  this->ProbeFrame->Create(pvApp, "frame", "");
  
  this->Script("pack %s",
               this->ProbeFrame->GetWidgetName());

  this->PointDataLabel->SetParent(this->ProbeFrame);
  this->PointDataLabel->Create(pvApp, "");

  this->ShowXYPlotToggle->SetParent(this->ProbeFrame);
  this->ShowXYPlotToggle->Create(pvApp, "-text \"Show XY-Plot\"");
  this->ShowXYPlotToggle->SetState(1);
  this->Script("%s configure -command {%s SetAcceptButtonColorToModified}",
               this->ShowXYPlotToggle->GetWidgetName(), this->GetTclName());
  this->Script("pack %s",
               this->ShowXYPlotToggle->GetWidgetName());


  // // Display all the possible data arrays
  // List box for selecting which fields to use for probing:
  this->FieldsSelection->SetParent(this->ParameterFrame->GetFrame());
  this->FieldsSelection->Create(pvApp, "");
  this->FieldsSelection->SetSingleClickCallback(this, "FieldsSelectCallback");
  //this->FieldsSelection->SetSelectState(0,1); //By default take first one
  this->FieldsSelection->GetListbox()->ConfigureOptions("-selectmode extended -exportselection 0");
  this->FieldsSelection->ScrollbarOff();

  this->Script("pack %s -expand true -fill both",
    this->FieldsSelection->GetWidgetName());

  // Add a save button to save XYPloatActor as CSV file
  this->SaveButton->SetParent(this->ParameterFrame->GetFrame());
  this->SaveButton->Create(pvApp); //, "foo");
  this->SaveButton->GetLoadSaveButton()->SetCommand(this, "SaveDialogCallback");
  //this->SaveButton->ExpandWidgetOn ();
  //this->SaveButton->SetLabelPositionToLeft ();
  this->SaveButton->SetLabel ("Save as CSV");
  vtkKWLoadSaveDialog *dlg = this->SaveButton->GetLoadSaveButton()->GetLoadSaveDialog();
  dlg->SetDefaultExtension(".csv");
  dlg->SetFileTypes("{{CSV Document} {.csv}}");
  dlg->SaveDialogOn();

  this->Script("pack %s",
               this->SaveButton->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVProbe::SetVisibilityNoTrace(int val)
{
  if (this->PlotDisplay)
    {
    this->PlotDisplay->SetVisibility(val);
    }
  this->Superclass::SetVisibilityNoTrace(val);
}


//----------------------------------------------------------------------------
void vtkPVProbe::ExecuteEvent(vtkObject* vtkNotUsed(wdg), 
                              unsigned long event,  
                              void* vtkNotUsed(calldata))
{
  //law int fixme;  // move this to the server.
  switch ( event )
    {
    case vtkCommand::StartInteractionEvent:
      //this->PVRenderView->GetPVWindow()->InteractiveRenderEnabledOn();
      break;
    case vtkCommand::EndInteractionEvent:
      //this->PVRenderView->GetPVWindow()->InteractiveRenderEnabledOff();
      //this->RenderView();
      vtkXYPlotActor* xypa = this->XYPlotWidget->GetXYPlotActor();
      double *pos1 = xypa->GetPositionCoordinate()->GetValue();
      double *pos2 = xypa->GetPosition2Coordinate()->GetValue();
      //this->AddTraceEntry("$kw(%s) SetScalarBarPosition1 %lf %lf", 
      //                    this->GetTclName(), pos1[0], pos1[1]);
      //this->AddTraceEntry("$kw(%s) SetScalarBarPosition2 %lf %lf", 
      //                    this->GetTclName(), pos2[0], pos2[1]);
      //this->AddTraceEntry("$kw(%s) SetScalarBarOrientation %d",
      //                    this->GetTclName(), sact->GetOrientation());

      // Synchronize the server scalar bar.
      vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
      vtkClientServerStream stream;
      stream << vtkClientServerStream::Invoke 
             << this->PlotDisplay->GetXYPlotActorProxy()->GetID(0) 
             << "GetPositionCoordinate" 
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke 
             << vtkClientServerStream::LastResult 
             << "SetValue" << pos1[0] << pos1[1]
             << vtkClientServerStream::End;

      stream << vtkClientServerStream::Invoke 
             << this->PlotDisplay->GetXYPlotActorProxy()->GetID(0) 
             << "GetPosition2Coordinate" 
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke 
             << vtkClientServerStream::LastResult 
             << "SetValue" << pos2[0] << pos2[1]
             << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::RENDER_SERVER, stream);
      break;
    }
}

//----------------------------------------------------------------------------
void vtkPVProbe::FieldsSelectCallback()
{
  int fastselec[100];
  int n = this->FieldsSelection->GetNumberOfItems();
  int *selec;
  if( n > 100 )
    {
    selec = new int[n];
    }
  else
    {
    selec = fastselec;
    }
    
  for(int i=0; i<n; i++)
    {
    selec[i] = this->FieldsSelection->GetSelectState(i);
    }
  this->PlotDisplay->UpdateInput(this->GetProxy(), selec);
  if( n > 100 )
    {
    delete [] selec;
    }
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVProbe::SaveDialogCallback()
{
  int numPts = this->GetDataInformation()->GetNumberOfPoints();

  // We need to be in the case of a line
  if (numPts != 1)
    {
    vtkXYPlotActor *xy = this->XYPlotWidget->GetXYPlotActor ();

    ofstream f;
    const char *filename = this->SaveButton->GetLoadSaveButton()->GetFileName();
    f.open( filename );
    xy->PrintAsCSV(f);
    f.close();
    }
}

//----------------------------------------------------------------------------
void vtkPVProbe::FillList(vtkKWListBox* list)
{
  vtkPVDataInformation *dataInfo = this->GetDataInformation();

  vtkPVDataSetAttributesInformation* pdInfo = dataInfo->GetPointDataInformation();
  int n = pdInfo->GetNumberOfArrays ();
  for(int i=0; i<n; i++)
    {
    vtkPVArrayInformation *info = pdInfo->GetArrayInformation(i);
    // Only append the array with only one component
    if( info->GetNumberOfComponents () == 1)
      {
      list->AppendUnique( info->GetName());
      list->SetSelectState(i,1);
      }
    else
      {
      list->SetSelectState(i,0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVProbe::AcceptCallbackInternal()
{
  this->AddTraceEntry("[$kw(%s) GetShowXYPlotToggle] SetState %d",
                      this->GetTclName(),
                      this->ShowXYPlotToggle->GetState());
  
  // call the superclass's method
  this->vtkPVSource::AcceptCallbackInternal();
  if (this->PlotDisplay->GetNumberOfIDs() == 0)
    {
    vtkPVWidget *wdg = this->GetPVWidget( "Probe object" );
    vtkPVSelectWidget *swdg = vtkPVSelectWidget::SafeDownCast( wdg );
    if ( strcmp(swdg->GetCurrentValue(), "Point") == 0 )
      {
      this->FieldsSelection->SetEnabled(0);
      this->SaveButton->SetEnabled(0);
      }
    else
      {
      this->FieldsSelection->SetEnabled(1);
      this->SaveButton->SetEnabled(1);
      }

    if( this->DisplayHasBeenAddedToTheRenderModule == 0)
      {
      this->DisplayHasBeenAddedToTheRenderModule = 1;
      // Connect to the display. Do it only once
      // These should be merged.
      this->PlotDisplay->SetInput(this->GetProxy());
      //this->GetProxy()->AddDisplay(this->PlotDisplay);
      this->FieldsSelectCallback();
      this->GetPVApplication()->GetProcessModule()->
        GetRenderModule()->AddDisplay(this->PlotDisplay);

      //Fill scalars fields:
      this->FillList(this->FieldsSelection);
      }
    }

  //law int fixme; // This should be in server.
  if ( !this->XYPlotWidget )
    {
    this->XYPlotWidget = vtkXYPlotWidget::New();
    this->PlotDisplay->ConnectWidgetAndActor(this->XYPlotWidget);

    vtkPVGenericRenderWindowInteractor* iren = 
      this->GetPVWindow()->GetInteractor();
    if ( iren )
      {
      this->XYPlotWidget->SetInteractor(iren);
      }

    // This observer synchronizes all processes when
    // the widget changes the plot.
    this->XYPlotObserver = vtkXYPlotWidgetObserver::New();
    this->XYPlotObserver->PVProbe = this;
    this->XYPlotWidget->AddObserver(vtkCommand::InteractionEvent, 
                                    this->XYPlotObserver);
    this->XYPlotWidget->AddObserver(vtkCommand::StartInteractionEvent, 
                                    this->XYPlotObserver);
    this->XYPlotWidget->AddObserver(vtkCommand::EndInteractionEvent, 
                                    this->XYPlotObserver);
    }

  // We need to update manually for the case we are probing one point.
  this->PlotDisplay->Update();
  int numPts = this->GetDataInformation()->GetNumberOfPoints();

  if (numPts == 1)
    { // Put the array information in the UI. 
    // Get the collected data from the display.
    vtkPolyData* d = this->PlotDisplay->GetCollectedData();
    vtkPointData* pd = d->GetPointData();
 
    // update the ui to see the point data for the probed point
    vtkIdType j, numComponents;

    // use vtkstd::string since 'label' can grow in length arbitrarily
    vtkstd::string label;
    vtkstd::string arrayData;
    vtkstd::string tempArray;

    this->XYPlotWidget->SetEnabled(0);

    int numArrays = pd->GetNumberOfArrays();
    for (int i = 0; i < numArrays; i++)
      {
      vtkDataArray* array = pd->GetArray(i);
      if (array->GetName())
        {
        numComponents = array->GetNumberOfComponents();
        if (numComponents > 1)
          {
          // make sure we fill buffer from the beginning
          ostrstream arrayStrm;
          arrayStrm << array->GetName() << ": ( " << ends;
          arrayData = arrayStrm.str();
          arrayStrm.rdbuf()->freeze(0);

          for (j = 0; j < numComponents; j++)
            {
            // make sure we fill buffer from the beginning
            ostrstream tempStrm;
            tempStrm << array->GetComponent( 0, j ) << ends; 
            tempArray = tempStrm.str();
            tempStrm.rdbuf()->freeze(0);

            if (j < numComponents - 1)
              {
              tempArray += ",";
              if (j % 3 == 2)
                {
                tempArray += "\n\t";
                }
              else
                {
                tempArray += " ";
                }
              }
            else
              {
              tempArray += " )\n";
              }
            arrayData += tempArray;
            }
          label += arrayData;
          }
        else
          {
          // make sure we fill buffer from the beginning
          ostrstream arrayStrm;
          arrayStrm << array->GetName() << ": " << array->GetComponent( 0, 0 ) << endl << ends;

          label += arrayStrm.str();
          arrayStrm.rdbuf()->freeze(0);
          }
        }
      }
    this->PointDataLabel->SetLabel( label.c_str() );
    this->Script("pack %s", this->PointDataLabel->GetWidgetName());
    }
  else
    {
    this->PointDataLabel->SetLabel("");
    this->Script("pack forget %s", this->PointDataLabel->GetWidgetName());
    }

  if (this->ShowXYPlotToggle->GetState() && numPts > 1)
    {
    vtkPVRenderModule* rm = this->GetPVApplication()->GetProcessModule()->GetRenderModule();
    this->XYPlotWidget->SetCurrentRenderer(rm->GetRenderer2D());
    this->GetPVRenderView()->Enable3DWidget(this->XYPlotWidget);

    this->PlotDisplay->SetVisibility(1);
    }
  else
    {
    this->XYPlotWidget->SetEnabled(0);
    vtkPVApplication* pvApp = this->GetPVApplication();
    vtkPVRenderModule* rm = pvApp->GetProcessModule()->GetRenderModule();
    rm->RemoveDisplay(this->PlotDisplay);
    }

}
 
//----------------------------------------------------------------------------
void vtkPVProbe::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ShowXYPlotToggle: " << this->ShowXYPlotToggle << endl;
  os << indent << "XYPlotWidget: " << this->XYPlotWidget << endl;
}
