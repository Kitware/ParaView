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

#include "vtkIdTypeArray.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPProbeFilter.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVArrayMenu.h"
#include "vtkPVClientServerModule.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty2D.h"
#include "vtkSocketController.h"
#include "vtkSource.h"
#include "vtkString.h"
#include "vtkSystemIncludes.h"
#include "vtkXYPlotActor.h"
#include "vtkXYPlotWidget.h"

#include <vtkstd/string>
 
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVProbe);
vtkCxxRevisionMacro(vtkPVProbe, "1.110");

int vtkPVProbeCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

#define PV_TAG_PROBE_OUTPUT 759362

//----------------------------------------------------------------------------
vtkPVProbe::vtkPVProbe()
{
  static int instanceCount = 0;
  
  this->CommandFunction = vtkPVProbeCommand;

  this->SelectedPointFrame = vtkKWWidget::New();
  this->SelectedPointLabel = vtkKWLabel::New();
  this->PointDataLabel = vtkKWLabel::New();
  
  this->ShowXYPlotToggle = vtkKWCheckButton::New();

  
  this->ProbeFrame = vtkKWWidget::New();
  
  this->XYPlotWidget = 0;
  
  // Create a unique id for creating tcl names.
  ++instanceCount;
  this->InstanceCount = instanceCount;
  
  this->ReplaceInputOff();

  this->XYPlotTclName = 0;

  char buffer[1024];
  sprintf(buffer, "probeXYPlot%d", this->InstanceCount);
  this->SetXYPlotTclName(buffer);

  // Special ivar in vtkPVSource just for this subclass.
  // We cannot process inputs that have more than one part.
  this->RequiredNumberOfInputParts = 1;
  
  this->ServerSideID.ID = 0;
  this->XYPlotActorID.ID = 0;
}

vtkPVProbe::~vtkPVProbe()
{
  if (this->ServerSideID.ID)
    {
    vtkPVProcessModule *pm = this->GetPVApplication()->GetProcessModule();
    pm->DeleteStreamObject(this->ServerSideID);
    pm->SendStreamToServerRoot();
    }
  
  if (this->XYPlotActorID.ID)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    if ( pvApp )
      {
      vtkPVProcessModule* pm = pvApp->GetProcessModule();
      pm->DeleteStreamObject(this->XYPlotActorID);
      pm->SendStreamToClientAndServer();
      }
    this->XYPlotActorID.ID = 0;
    }

  if ( this->XYPlotWidget )
    {
    this->XYPlotWidget->SetEnabled(0);
    this->XYPlotWidget->SetInteractor(NULL);
    this->XYPlotWidget->Delete();
    this->XYPlotWidget = 0;
    }
  this->SetXYPlotTclName(0);

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
  this->vtkPVSource::CreateProperties();
  // We do not support probing groups and multi-block objects. Therefore,
  // we use the first VTKSource id.
  pm->GetStream() << vtkClientServerStream::Invoke <<  this->GetVTKSourceID(0)
                  << "SetSpatialMatch" << 2
                  << vtkClientServerStream::End;
  pm->SendStreamToServer();

  this->ProbeFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->ProbeFrame->Create(pvApp, "frame", "");
  
  this->Script("pack %s",
               this->ProbeFrame->GetWidgetName());

  // widgets for points
  this->SelectedPointFrame->SetParent(this->ProbeFrame);
  this->SelectedPointFrame->Create(pvApp, "frame", "");
  
  
  
  this->SelectedPointLabel->SetParent(this->SelectedPointFrame);
  this->SelectedPointLabel->Create(pvApp, "");
  this->SelectedPointLabel->SetLabel("Point");

  this->Script("pack %s -side left",
               this->SelectedPointLabel->GetWidgetName());
  
  this->PointDataLabel->SetParent(this->ProbeFrame);
  this->PointDataLabel->Create(pvApp, "");

  this->ShowXYPlotToggle->SetParent(this->ProbeFrame);
  this->ShowXYPlotToggle->Create(pvApp, "-text \"Show XY-Plot\"");
  this->ShowXYPlotToggle->SetState(1);
  this->Script("%s configure -command {%s SetAcceptButtonColorToRed}",
               this->ShowXYPlotToggle->GetWidgetName(), this->GetTclName());

  this->Script("pack %s",
               this->ShowXYPlotToggle->GetWidgetName());

  if ( !this->XYPlotWidget )
    {
    // Actor will be in server manager.  Widget will be in UI.
    this->XYPlotActorID = pm->NewStreamObject("vtkXYPlotActor");
    pm->SendStreamToClientAndServer();

    this->XYPlotWidget = vtkXYPlotWidget::New();
    this->XYPlotWidget->SetXYPlotActor(vtkXYPlotActor::SafeDownCast(
                                    pm->GetObjectFromID(this->XYPlotActorID)));

    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorID 
                    << "GetPositionCoordinate" 
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << vtkClientServerStream::LastResult 
                    << "SetValue" << 0.05 << 0.05 << 0
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorID 
                    << "GetPosition2Coordinate" 
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << vtkClientServerStream::LastResult 
                    << "SetValue" << 0.8 << 0.3 << 0
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorID 
                    << "SetNumberOfLabels" << 5 
                    << vtkClientServerStream::End;
    // This is stupid and has to change!
    int fixme;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorID 
                    << "SetXTitle" << "Line Divisions" 
                    << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();


    pm->GetStream() << vtkClientServerStream::Invoke << pm->GetApplicationID()
                    << "GetController"
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke << this->GetVTKSourceID(0) 
                    << "SetController"
                    << vtkClientServerStream::LastResult
                    << vtkClientServerStream::End; 
    pm->SendStreamToServer();
    
    vtkPVGenericRenderWindowInteractor* iren = 
      this->GetPVWindow()->GetInteractor();
    if ( iren )
      {
      this->XYPlotWidget->SetInteractor(iren);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVProbe::AcceptCallbackInternal()
{
  int i;
  const char *arrayName = NULL;
  
  this->AddTraceEntry("[$kw(%s) GetShowXYPlotToggle] SetState %d",
                      this->GetTclName(),
                      this->ShowXYPlotToggle->GetState());
  
  // call the superclass's method
  this->vtkPVSource::AcceptCallbackInternal();
  
  vtkPVWindow *window = this->GetPVWindow();
  
  // Can't wait for render to update things.
  if (this->GetPVOutput())
    {
    // Update the VTK data.
    this->Update();
    }
  
  // Get the probe filter's output from the root node's process.
  vtkPolyData* probeOutput = NULL;

  vtkPVProcessModule *pm = this->GetPVApplication()->GetProcessModule();
  vtkPVClientServerModule *csm = vtkPVClientServerModule::SafeDownCast(pm);

  if (csm)
    {
    probeOutput = vtkPolyData::New();
    if (!this->ServerSideID.ID)
      {
      this->ServerSideID = pm->NewStreamObject("vtkPVServerProbe");
      pm->GetStream() << vtkClientServerStream::Invoke
                      << this->ServerSideID << "SetProcessModule"
                      << pm->GetProcessModuleID()
                      << vtkClientServerStream::End;
      }
    
    pm->GetStream() << vtkClientServerStream::Invoke
                    << this->ServerSideID << "SendPolyDataOutput"
                    << this->GetVTKSourceID(0)
                    << vtkClientServerStream::End;
    pm->SendStreamToServerRoot();
  
    csm->GetSocketController()->Receive(probeOutput, 1, PV_TAG_PROBE_OUTPUT);
    }
  else
    {
    vtkPProbeFilter *probeFilter = vtkPProbeFilter::SafeDownCast(
      pm->GetObjectFromID(this->GetVTKSourceID(0)));
    probeOutput = probeFilter->GetPolyDataOutput();
    probeOutput->Register(NULL);
    }
  
  vtkPointData *pd = probeOutput->GetPointData();
  
  int arrayCount;
  vtkDataArray *array;
  vtkPVArrayInformation *arrayInfo;

  vtkPVDataInformation *di = this->GetPart()->GetDataInformation();
  vtkPVDataSetAttributesInformation *pdi = di->GetPointDataInformation();
  
  int numPts = this->GetPart()->GetDataInformation()->GetNumberOfPoints();
  int numArrays = pdi->GetNumberOfArrays();
  
  if (numPts == 1)
    {
    // update the ui to see the point data for the probed point
    
    vtkIdType j, numComponents;

    // use vtkstd::string since 'label' can grow in length arbitrarily
    vtkstd::string label;
    vtkstd::string arrayData;
    vtkstd::string tempArray;

    this->XYPlotWidget->SetEnabled(0);

    for (i = 0; i < numArrays; i++)
      {
      array = pd->GetArray(i);
      arrayInfo = pdi->GetArrayInformation(i);
      numComponents = arrayInfo->GetNumberOfComponents();
      if (numComponents > 1)
        {
        // make sure we fill buffer from the beginning
        ostrstream arrayStrm;
        arrayStrm << arrayInfo->GetName() << ": ( " << ends;
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
    this->PointDataLabel->SetLabel( label.c_str() );
    this->Script("pack %s", this->PointDataLabel->GetWidgetName());
    }
  else if (numPts > 1)
    {
    this->Script("pack forget %s", this->PointDataLabel->GetWidgetName());

    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorID 
                    << "RemoveAllInputs" 
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorID 
                    << "SetYTitle" << ""
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorID 
                    << "PlotPointsOn" 
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorID 
                    << "GetProperty" 
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << vtkClientServerStream::LastResult 
                    << "SetColor" << 1 << 0.8 << 0.8
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorID 
                    << "GetProperty" 
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << vtkClientServerStream::LastResult 
                    << "SetPointSize" << 2
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorID 
                    << "SetLegendPosition" << 0.4 << 0.6 
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->XYPlotActorID 
                    << "SetLegendPosition2" << 0.5 << 0.25 
                    << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();

    float cstep = 1.0 / numArrays;
    float ccolor = 0;
    arrayCount = 0;
    vtkXYPlotActor* xyp = this->XYPlotWidget->GetXYPlotActor();
    for ( i = 0; i < numArrays; i++)
      {
      arrayInfo = pdi->GetArrayInformation(i);
      arrayName = arrayInfo->GetName();
      if (arrayInfo->GetNumberOfComponents() == 1)
        {
        // Replacing this wiull tak a little more work.
        xyp->AddInput(probeOutput, arrayName, 0);
        xyp->SetPlotLabel(i, arrayName);
        float r, g, b;
        this->HSVtoRGB(ccolor, 1, 1, &r, &g, &b);
        xyp->SetPlotColor(i, r, g, b);
        ccolor += cstep;
        arrayCount ++;
        }      
      }
    
    if ( arrayCount > 1 )
      {
      xyp->LegendOn();
      }
    else 
      {
      xyp->LegendOff();
      xyp->SetYTitle(arrayName);
      xyp->SetPlotColor(0, 1, 1, 1);
      }
    if (this->ShowXYPlotToggle->GetState())
      {
      this->GetPVRenderView()->Enable3DWidget(this->XYPlotWidget);
      }
    else
      {
      this->XYPlotWidget->SetEnabled(0);
      }
    
    window->GetMainView()->Render();
    }
  
  probeOutput->Delete();
}
 
//----------------------------------------------------------------------------
void vtkPVProbe::Deselect(int doPackForget)
{
  this->vtkPVSource::Deselect(doPackForget);
}

//----------------------------------------------------------------------------
void vtkPVProbe::SaveInBatchScript(ofstream *file)
{
  Tcl_Interp *interp = this->GetPVApplication()->GetMainInterp(); 
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();

  if (this->VisitedFlag)
    {
    return;
    } 
  this->VisitedFlag = 1;

  if (this->GetDimensionality() == 0)
    {
    *file << "vtkIdList pointList" << this->InstanceCount << "\n";
    *file << "\tpointList" << this->InstanceCount << " Allocate 1 1\n";
    *file << "\tpointList" << this->InstanceCount << " InsertNextId 0\n\n";

    *file << "vtkPoints points" << this->InstanceCount << "\n";
    *file << "\tpoints" << this->InstanceCount << " Allocate 1 1\n";
    *file << "\tpoints" << this->InstanceCount << " InsertNextPoint " << interp->result << "\n\n";
    
    *file << "vtkPolyData probeInput" << this->InstanceCount << "\n";
    *file << "\tprobeInput" << this->InstanceCount << " Allocate 1 1\n";
    *file << "\tprobeInput" << this->InstanceCount << " SetPoints points" << this->InstanceCount << "\n";
    *file << "\tprobeInput" << this->InstanceCount << " InsertNextCell 1 pointList" << this->InstanceCount << "\n\n";
    }
  else
    {
    *file << "vtkLineSource line" << this->InstanceCount << "\n";

    pm->GetStream() << vtkClientServerStream::Invoke << this->GetVTKSourceID(0)
                    << "GetInput"
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << vtkClientServerStream::LastResult
                    << "GetSource" << 0 
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << vtkClientServerStream::LastResult
                    << "GetGetPoint" << 0 
                    << vtkClientServerStream::End;
    pm->SendStreamToClient();
    {
    ostrstream result;
    pm->GetLastClientResult().PrintArgumentValue(result, 0,0);
    result << ends;
    *file << "\tline" << this->InstanceCount << " SetPoint1 " << result.str() << "\n";
    delete []result.str();
    }
    pm->GetStream() << vtkClientServerStream::Invoke << this->GetVTKSourceID(0)
                    << "GetInput"
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << vtkClientServerStream::LastResult
                    << "GetSource" << 0 
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << vtkClientServerStream::LastResult
                    << "GetGetPoint" << 0 
                    << vtkClientServerStream::End;
    pm->SendStreamToClient();
    {
    ostrstream result;
    pm->GetLastClientResult().PrintArgumentValue(result, 0,0);
    result << ends;
    *file << "\tline" << this->InstanceCount << " SetPoint2 " << result.str() << "\n";
    delete []result.str();
    }
    pm->GetStream() << vtkClientServerStream::Invoke << this->GetVTKSourceID(0)
                    << "GetInput"
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << vtkClientServerStream::LastResult
                    << "GetSource" << 0 
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << vtkClientServerStream::LastResult
                    << "GetResolution" << 0 
                    << vtkClientServerStream::End;
    pm->SendStreamToClient();
    {
    ostrstream result;
    pm->GetLastClientResult().PrintArgumentValue(result, 0,0);
    result << ends;
    *file << "\tline" << this->InstanceCount << " SetResolution " << result.str() << "\n\n";
    delete []result.str();
    }
    }
  
//  *file << this->GetVTKSource()->GetClassName() << " "
  *file << this->GetSourceClassName() << " pvTemp"
        << this->GetVTKSourceID(0) << "\n";
  if (this->GetDimensionality() == 0)
    {
    *file << "\tpvTemp" << this->GetVTKSourceID(0) << " SetInput probeInput" 
          << this->InstanceCount << "\n";
    }
  else
    {
    *file << "\tpvTemp" << this->GetVTKSourceID(0)
          << " SetInput [line" << this->InstanceCount << " GetOutput]\n";
    }
  
  *file << "\tpvTemp" << this->GetVTKSourceID(0) << " SetSource [pvTemp";
  *file << this->GetPVInput(0)->GetVTKSourceID(0)
        << " GetOutput]\n\n";
  
  if (this->GetDimensionality() == 1)
    {    
    if (this->ShowXYPlotToggle->GetState())
      {
      double pos[3];
      vtkXYPlotActor* xyp = this->XYPlotWidget->GetXYPlotActor();
      *file << "vtkXYPlotActor " << this->XYPlotTclName << "\n";
      xyp->GetPositionCoordinate()->GetValue(pos);
      *file << "\t[" << this->XYPlotTclName << " GetPositionCoordinate] SetValue " 
            << pos[0] << " " << pos[1] << " 0\n";
      xyp->GetPosition2Coordinate()->GetValue(pos);
      *file << "\t[" << this->XYPlotTclName << " GetPosition2Coordinate] SetValue " 
            << pos[0] << " " << pos[1] << " 0\n";
      *file << "\t" << this->XYPlotTclName << " SetNumberOfXLabels "
            << xyp->GetNumberOfXLabels() << "\n";
      *file << "\t" << this->XYPlotTclName << " SetXTitle \"" 
            << xyp->GetXTitle() << "\" \n";
      *file << "\t" << this->XYPlotTclName << " RemoveAllInputs" << endl;
      *file << "\t" << this->XYPlotTclName << " AddInput [ pvTemp" 
            << this->GetVTKSourceID(0) << " GetOutput ]" << endl;

      *file << "Ren1" << " AddActor "
            << this->XYPlotTclName << endl;

      }
    }
  
  this->GetPVOutput()->SaveInBatchScript(file);
}

//----------------------------------------------------------------------------
int vtkPVProbe::GetDimensionality()
{
  if (!this->GetVTKSourceID(0).ID)
    {
    return 0;
    }
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  pm->GetStream() << vtkClientServerStream::Invoke <<  this->GetVTKSourceID(0)
                  << "GetInput" 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult 
                  << "GetSource"
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult 
                  << "GetClassName"
                  << vtkClientServerStream::End;
  pm->SendStreamToClient();
  const char* name = 0;
  pm->GetLastClientResult().GetArgument(0,0,&name);
  if ( name && vtkString::Equals(name, "vtkLineSource") )
    {
    return 1;
    }
  return 0;
}


//----------------------------------------------------------------------------
void vtkPVProbe::HSVtoRGB(float h, float s, float v, float *r, float *g, float *b)
{
  float R, G, B;
  float max = 1.0;
  float third = max / 3.0;
  float temp;

  // compute rgb assuming S = 1.0;
  if (h >= 0.0 && h <= third) // red -> green
    {
    G = h/third;
    R = 1.0 - G;
    B = 0.0;
    }
  else if (h >= third && h <= 2.0*third) // green -> blue
    {
    B = (h - third)/third;
    G = 1.0 - B;
    R = 0.0;
    }
  else // blue -> red
    {
    R = (h - 2.0 * third)/third;
    B = 1.0 - R;
    G = 0.0;
    }
        
  // add Saturation to the equation.
  s = s / max;
  //R = S + (1.0 - S)*R;
  //G = S + (1.0 - S)*G;
  //B = S + (1.0 - S)*B;
  // what happend to this?
  R = s*R + (1.0 - s);
  G = s*G + (1.0 - s);
  B = s*B + (1.0 - s);
      
  // Use value to get actual RGB 
  // normalize RGB first then apply value
  temp = R + G + B; 
  //V = 3 * V / (temp * max);
  // and what happend to this?
  v = 3 * v / (temp);
  R = R * v;
  G = G * v;
  B = B * v;
      
  // clip below 255
  //if (R > 255.0) R = max;
  //if (G > 255.0) G = max;
  //if (B > 255.0) B = max;
  // mixed constant 255 and max ?????
  if (R > max)
    {
    R = max;
    }
  if (G > max)
    {
    G = max;
    }
  if (B > max)
    {
    B = max;
    }
  *r = R;
  *g = G;
  *b = B;
}
//----------------------------------------------------------------------------
void vtkPVProbe::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ShowXYPlotToggle: " << this->GetShowXYPlotToggle() << endl;
  os << indent << "XYPlotWidget: " << this->XYPlotWidget << endl;
}
