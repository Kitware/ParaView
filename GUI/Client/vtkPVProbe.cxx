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
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"

#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkXYPlotActor.h"
#include "vtkXYPlotWidget.h"
#include "vtkSMPlotDisplay.h"

#include "vtkSMXYPlotDisplayProxy.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include <vtkstd/string>
 
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVProbe);
vtkCxxRevisionMacro(vtkPVProbe, "1.138.2.3");

int vtkPVProbeCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

#define PV_TAG_PROBE_OUTPUT 759362



//----------------------------------------------------------------------------
vtkPVProbe::vtkPVProbe()
{
  this->CommandFunction = vtkPVProbeCommand;

  this->SelectedPointFrame = vtkKWWidget::New();
  this->SelectedPointLabel = vtkKWLabel::New();
  this->PointDataLabel = vtkKWLabel::New();
  
  this->ShowXYPlotToggle = vtkKWCheckButton::New();

  
  this->ProbeFrame = vtkKWWidget::New();
  
  
  this->ReplaceInputOff();

  // Special ivar in vtkPVSource just for this subclass.
  // We cannot process inputs that have more than one part.
  this->RequiredNumberOfInputParts = 1;
  
  this->PlotDisplayProxy = 0; 
  this->PlotDisplayProxyName = 0;
}

vtkPVProbe::~vtkPVProbe()
{  
  if (this->PlotDisplayProxy)
    {
    if (this->GetPVApplication() && this->GetPVApplication()->GetRenderModuleProxy())
      {
      this->RemoveDisplayFromRenderModule(this->PlotDisplayProxy);
      }

    if  (this->PlotDisplayProxyName)
      {
      vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
      pxm->UnRegisterProxy("displays", this->PlotDisplayProxyName);
      this->SetPlotDisplayProxyName(0);
      }

    this->PlotDisplayProxy->Delete();
    this->PlotDisplayProxy = NULL;
    }

  this->SelectedPointLabel->Delete();
  this->SelectedPointLabel = NULL;
  this->SelectedPointFrame->Delete();
  this->SelectedPointFrame = NULL;
  this->PointDataLabel->Delete();
  this->PointDataLabel = NULL;  
  
  this->ShowXYPlotToggle->Delete();
  this->ShowXYPlotToggle = NULL;
  
  this->ProbeFrame->Delete();
  this->ProbeFrame = NULL;  
}

//----------------------------------------------------------------------------
void vtkPVProbe::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  vtkClientServerStream stream;
    
  this->Superclass::CreateProperties();
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

  // widgets for points
  this->SelectedPointFrame->SetParent(this->ProbeFrame);
  this->SelectedPointFrame->Create(pvApp, "frame", "");
  
  
  
  this->SelectedPointLabel->SetParent(this->SelectedPointFrame);
  this->SelectedPointLabel->Create(pvApp, "");
  this->SelectedPointLabel->SetText("Point");

  this->Script("pack %s -side left",
               this->SelectedPointLabel->GetWidgetName());
  
  this->PointDataLabel->SetParent(this->ProbeFrame);
  this->PointDataLabel->Create(pvApp, "");

  this->ShowXYPlotToggle->SetParent(this->ProbeFrame);
  this->ShowXYPlotToggle->Create(pvApp, "-text \"Show XY-Plot\"");
  this->ShowXYPlotToggle->SetState(1);
  this->Script("%s configure -command {%s SetAcceptButtonColorToModified}",
               this->ShowXYPlotToggle->GetWidgetName(), this->GetTclName());

  this->Script("pack %s",
               this->ShowXYPlotToggle->GetWidgetName());

}

//----------------------------------------------------------------------------
void vtkPVProbe::SetVisibilityNoTrace(int val)
{
  if (this->PlotDisplayProxy)
    {
    this->PlotDisplayProxy->cmSetVisibility(val);
    }
  this->Superclass::SetVisibilityNoTrace(val);
}


//----------------------------------------------------------------------------
void vtkPVProbe::AcceptCallbackInternal()
{
  this->AddTraceEntry("[$kw(%s) GetShowXYPlotToggle] SetState %d",
                      this->GetTclName(),
                      this->ShowXYPlotToggle->GetState());
  
  // call the superclass's method
  this->vtkPVSource::AcceptCallbackInternal();
   
  if (!this->PlotDisplayProxy)
    {
    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();

    this->PlotDisplayProxy = vtkSMXYPlotDisplayProxy::SafeDownCast(
      pxm->NewProxy("displays", "XYPlotDisplay"));
    if (!this->PlotDisplayProxy)
      {
      vtkErrorMacro("Failed to create Plot Display Proxy!");
      return;
      }

    ostrstream str;
    // SourceListName.SourceProxyName.XYPlotDisplay == name for the 
    // Display proxy.
    str << this->GetSourceList() << "."
      << this->GetName() << "."
      << "XYPlotDisplay" << ends;
    this->SetPlotDisplayProxyName(str.str());
    str.rdbuf()->freeze(0);
    pxm->RegisterProxy("displays", this->PlotDisplayProxyName, this->PlotDisplayProxy);
    vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("Input"));
    if (!ip)
      {
      vtkErrorMacro("Failed to find property Input on PlotDisplayProxy.");
      return;
      }
    ip->RemoveAllProxies();
    ip->AddProxy(this->GetProxy());
    this->PlotDisplayProxy->UpdateVTKObjects();
    this->AddDisplayToRenderModule(this->PlotDisplayProxy);
    }
  // We need to update manually for the case we are probing one point.
  int numPts = this->GetDataInformation()->GetNumberOfPoints();

  if (numPts == 1)
    { // Put the array information in the UI. 
    // Get the collected data from the display.
    this->PlotDisplayProxy->Update();
    vtkPolyData* d = this->PlotDisplayProxy->GetCollectedData();
    vtkPointData* pd = d->GetPointData();
  
    // update the ui to see the point data for the probed point
    vtkIdType j, numComponents;

    // use vtkstd::string since 'label' can grow in length arbitrarily
    vtkstd::string label;
    vtkstd::string arrayData;
    vtkstd::string tempArray;

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
    this->PointDataLabel->SetText( label.c_str() );
    this->Script("pack %s", this->PointDataLabel->GetWidgetName());
    }
  else
    {
    this->PointDataLabel->SetText("");
    this->Script("pack forget %s", this->PointDataLabel->GetWidgetName());
    }

  if (this->ShowXYPlotToggle->GetState() && numPts > 1)
    {
    vtkPVArrayInformation* arrayInfo = 
      this->GetDataInformation()->GetPointDataInformation()->GetArrayInformation(0);
    vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("ArrayNames"));
    if (svp)
      {
      svp->SetNumberOfElements(1);
      svp->SetElement(0, arrayInfo->GetName());
      this->PlotDisplayProxy->UpdateVTKObjects();
      }
      
    this->PlotDisplayProxy->cmSetVisibility(1);
    }
  else
    {
    this->PlotDisplayProxy->cmSetVisibility(0);
    }
    
}
 
//----------------------------------------------------------------------------
void vtkPVProbe::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PlotDisplayProxy: " << this->PlotDisplayProxy << endl;
  os << indent << "ShowXYPlotToggle: " << this->GetShowXYPlotToggle() << endl;
}
