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
#include "vtkPVTraceHelper.h"

#include "vtkSMXYPlotDisplayProxy.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkPVArraySelection.h"
#include "vtkSMDomain.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMXYPlotActorProxy.h"
#include <vtkstd/string>
#include <kwsys/ios/sstream>
 
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVProbe);
vtkCxxRevisionMacro(vtkPVProbe, "1.142");

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
  this->CanShowPlot = 0;
  this->ArraySelection = vtkPVArraySelection::New();
}

//----------------------------------------------------------------------------
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

  this->ArraySelection->Delete();
  this->ArraySelection = NULL;
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

    kwsys_ios::ostringstream str;
    // SourceListName.SourceProxyName.XYPlotDisplay == name for the 
    // Display proxy.
    str << this->GetSourceList() << "."
      << this->GetName() << "."
      << "XYPlotDisplay";
    this->SetPlotDisplayProxyName(str.str().c_str());
    pxm->RegisterProxy("displays", this->PlotDisplayProxyName, this->PlotDisplayProxy);

    // We cannot set the input here itself,
    // since the Probe proxy is not yet created and setting it as input
    // calls a CreateParts() on it, which leads to errors.
    // Hence, we defer it until after initializaiton.
    }

  this->ArraySelection->SetParent(this->ProbeFrame);
  this->ArraySelection->SetPVSource(this);
  this->ArraySelection->SetLabelText("Point Scalars");
  this->ArraySelection->SetModifiedCommand(this->GetTclName(), "ArraySelectionInternalCallback");
}

//----------------------------------------------------------------------------
void vtkPVProbe::SetVisibilityNoTrace(int val)
{
  if (this->PlotDisplayProxy && this->CanShowPlot)
    {
    this->PlotDisplayProxy->SetVisibilityCM(val);
    }
  this->Superclass::SetVisibilityNoTrace(val);
}


//----------------------------------------------------------------------------
void vtkPVProbe::AcceptCallbackInternal()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  this->GetTraceHelper()->AddEntry("[$kw(%s) GetShowXYPlotToggle] SetState %d",
                      this->GetTclName(),
                      this->ShowXYPlotToggle->GetState());
 
  int initialized = this->GetInitialized();
  
  // call the superclass's method
  this->vtkPVSource::AcceptCallbackInternal();
   
  if (!initialized)
    {
    // This should be done on first accept or initialization
    vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("Input"));
    if (!ip)
      {
      vtkErrorMacro("Failed to find property Input on PlotDisplayProxy.");
      return;
      }
    ip->RemoveAllProxies();
    ip->AddProxy(this->GetProxy());
    this->PlotDisplayProxy->SetVisibilityCM(0); // also calls UpdateVTKObjects().
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
          kwsys_ios::ostringstream arrayStrm;
          arrayStrm << array->GetName() << ": ( ";
          arrayData = arrayStrm.str();

          for (j = 0; j < numComponents; j++)
            {
            // make sure we fill buffer from the beginning
            kwsys_ios::ostringstream tempStrm;
            tempStrm << array->GetComponent( 0, j );
            tempArray = tempStrm.str();

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
          kwsys_ios::ostringstream arrayStrm;
          arrayStrm << array->GetName() << ": " << array->GetComponent( 0, 0 ) << endl;

          label += arrayStrm.str();
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

  // Fill up the ArrayNames of the XYPlotActorProxy (subproxy of XYPlotDisplayProxy) from the
  // input of PVProbe
  if (!initialized)
    {
    int numArrays = this->GetDataInformation()->GetPointDataInformation()->GetNumberOfArrays();
    vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
      this->PlotDisplayProxy->GetProperty("ArrayNames"));
    if (svp)
      {
      for(int i=0; i<numArrays; i++)
        {
        vtkPVArrayInformation* arrayInfo = 
          this->GetDataInformation()->GetPointDataInformation()->GetArrayInformation(i);
        if( arrayInfo->GetNumberOfComponents() == 1 )
          {
          svp->SetElement(i, arrayInfo->GetName());
          }
        }
      // Trick to force a domain of the sub-proxy to depend to the parent proxy one
      // This need to be done after the accept
      vtkSMDomain *arrayList = svp->GetDomain( "array_list" );
      vtkSMProperty* inputProp = this->GetProxy()->GetProperty("Input");
      arrayList->AddRequiredProperty(inputProp, "SubInput");
      svp->UpdateDependentDomains(); // Now forcing to update the domain

      this->ArraySelection->SetSMProperty(svp);
      this->ArraySelection->Create(pvApp);
      }
    else
      {
      vtkErrorMacro("Failed to find property ArrayNames.");
      }
    }

  this->CanShowPlot = numPts > 1 ? 1: 0;

  if (this->ShowXYPlotToggle->GetState() && numPts > 1)
    {
    this->PlotDisplayProxy->SetVisibilityCM(1);
    this->Script("pack %s", this->ArraySelection->GetWidgetName());  
    }
  else
    {
    this->PlotDisplayProxy->SetVisibilityCM(0);
    this->Script("pack forget %s", this->ArraySelection->GetWidgetName());  
    }

}
 
//----------------------------------------------------------------------------
void vtkPVProbe::ArraySelectionInternalCallback()
{
  this->ArraySelection->Accept();
  this->PlotDisplayProxy->UpdateVTKObjects();
  this->GetPVRenderView()->EventuallyRender();
}

//----------------------------------------------------------------------------
void vtkPVProbe::SaveInBatchScript(ofstream* file)
{
  if( this->VisitedFlag )
    {
    return;
    }

  this->Superclass::SaveInBatchScript(file);
  if (this->CanShowPlot)
    {
    *file << "  # Save the XY Plot" << endl;
    this->PlotDisplayProxy->SaveInBatchScript(file);
    }
}

//----------------------------------------------------------------------------
void vtkPVProbe::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PlotDisplayProxy: " << this->PlotDisplayProxy << endl;
  os << indent << "ShowXYPlotToggle: " << this->GetShowXYPlotToggle() << endl;
}

