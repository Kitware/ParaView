/*=========================================================================

  Program:   ParaView
  Module:    vtkRMScalarBarWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRMScalarBarWidget.h"

#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkScalarBarWidget.h"
#include "vtkLookupTable.h"
#include "vtkClientServerStream.h"
#include "vtkKWEvent.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkScalarBarActor.h"
#include "vtkCommand.h"

class vtkScalarBarWidgetObserver : public vtkCommand
{
public:
  static vtkScalarBarWidgetObserver *New() 
    {return new vtkScalarBarWidgetObserver;};

  vtkScalarBarWidgetObserver()
    {
      this->RMScalarBarWidget = 0;
    }

  virtual void Execute(vtkObject* wdg, unsigned long event,  
                       void* calldata)
    {
      if ( this->RMScalarBarWidget )
        {
        //this->RMScalarBarWidget->ExecuteEvent(wdg, event, calldata);
        this->RMScalarBarWidget->ExecuteEvent(wdg,event,calldata);
        }
    }

  vtkRMScalarBarWidget* RMScalarBarWidget;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkRMScalarBarWidget);
vtkCxxRevisionMacro(vtkRMScalarBarWidget, "1.1");

#define VTK_PV_COLOR_MAP_RED_HUE 0.0
#define VTK_PV_COLOR_MAP_RED_SATURATION 1.0
#define VTK_PV_COLOR_MAP_RED_VALUE 1.0
#define VTK_PV_COLOR_MAP_BLUE_HUE 0.6667
#define VTK_PV_COLOR_MAP_BLUE_SATURATION 1.0
#define VTK_PV_COLOR_MAP_BLUE_VALUE 1.0
#define VTK_PV_COLOR_MAP_BLACK_HUE 0.0
#define VTK_PV_COLOR_MAP_BLACK_SATURATION 0.0
#define VTK_PV_COLOR_MAP_BLACK_VALUE 0.0
#define VTK_PV_COLOR_MAP_WHITE_HUE 0.0
#define VTK_PV_COLOR_MAP_WHITE_SATURATION 0.0
#define VTK_PV_COLOR_MAP_WHITE_VALUE 1.0

#define VTK_USE_LAB_COLOR_MAP 1.1
//----------------------------------------------------------------------------
vtkRMScalarBarWidget::vtkRMScalarBarWidget()
{
  this->PVProcessModule = 0;

  this->ScalarBarTitle = NULL;
  this->ScalarBarLabelFormat = NULL;
  this->ArrayName = NULL;
  this->NumberOfVectorComponents = 0;
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;


  this->StartHSV[0] = VTK_PV_COLOR_MAP_RED_HUE;
  this->StartHSV[1] = VTK_PV_COLOR_MAP_RED_SATURATION;
  this->StartHSV[2] = VTK_PV_COLOR_MAP_RED_VALUE;
  this->EndHSV[0] = VTK_PV_COLOR_MAP_BLUE_HUE;
  this->EndHSV[1] = VTK_PV_COLOR_MAP_BLUE_SATURATION;
  this->EndHSV[2] = VTK_PV_COLOR_MAP_BLUE_VALUE;

  this->NumberOfColors = 256;

  this->LookupTableID.ID = 0;
  this->LookupTable = NULL;
  this->ScalarBarActorID.ID = 0;

  this->VectorMode = vtkRMScalarBarWidget::MAGNITUDE;
  this->VectorComponent = 0;

  this->VectorMagnitudeTitle = new char[12];
  strcpy(this->VectorMagnitudeTitle, "Magnitude");
  this->VectorComponentTitles = NULL;
  this->ScalarBarVectorTitle = NULL;

  this->ScalarBar = NULL;
  this->ScalarBarObserver = NULL;
}

//----------------------------------------------------------------------------
vtkRMScalarBarWidget::~vtkRMScalarBarWidget()
{
  if (this->LookupTableID.ID && this->PVProcessModule)
    {
    this->PVProcessModule->DeleteStreamObject(this->LookupTableID);
    this->PVProcessModule->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    this->LookupTableID.ID = 0;
    this->LookupTable = 0;
    }

  if (this->ScalarBarActorID.ID && this->PVProcessModule)
    {
    this->PVProcessModule->DeleteStreamObject(this->ScalarBarActorID);
    this->PVProcessModule->SendStream(
      vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    this->ScalarBarActorID.ID = 0;
    }
  if (this->ArrayName)
    {
    delete [] this->ArrayName;
    this->ArrayName = NULL;
    }
  
  if (this->ScalarBarTitle)
    {
    delete [] this->ScalarBarTitle;
    this->ScalarBarTitle = NULL;
    }
  if (this->ScalarBarLabelFormat)
    {
    delete [] this->ScalarBarLabelFormat;
    this->ScalarBarLabelFormat = NULL;
    }

  if (this->VectorMagnitudeTitle)
    {
    delete [] this->VectorMagnitudeTitle;
    this->VectorMagnitudeTitle = NULL;
    }
  // This will delete the vector component titles.
  this->PVProcessModule = NULL;
  this->SetNumberOfVectorComponents(0); 
  if(this->ScalarBarVectorTitle)
    {
    delete [] this->ScalarBarVectorTitle;
    this->ScalarBarVectorTitle = NULL;
    }
  if(this->ScalarBar)
    {
    this->ScalarBar->Delete();
    this->ScalarBar = NULL;
    }
  if(this->ScalarBarObserver)
    {
    this->ScalarBarObserver->Delete();
    this->ScalarBarObserver = NULL;
    }
}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::ExecuteEvent(vtkObject*, unsigned long event,  
                            void* calldata)
{
    switch ( event )
    {
    case vtkCommand::StartInteractionEvent:
      break;
    case vtkCommand::EndInteractionEvent:
      vtkScalarBarActor* sact = this->ScalarBar->GetScalarBarActor();
      double *pos1 = sact->GetPositionCoordinate()->GetValue();

      // Synchronize the server scalar bar.
      vtkPVProcessModule* pm = this->PVProcessModule;
      pm->GetStream() << vtkClientServerStream::Invoke 
                      << this->ScalarBarActorID
                      << "GetPositionCoordinate" 
                      << vtkClientServerStream::End;
      pm->GetStream() << vtkClientServerStream::Invoke 
                      << vtkClientServerStream::LastResult 
                      << "SetValue" << pos1[0] << pos1[1]
                      << vtkClientServerStream::End;

      pm->GetStream() << vtkClientServerStream::Invoke 
                      << this->ScalarBarActorID
                      << "SetOrientation" 
                      << this->ScalarBar->GetScalarBarActor()->GetOrientation()
                      << vtkClientServerStream::End;

      pm->GetStream() << vtkClientServerStream::Invoke 
                      << this->ScalarBarActorID
                      << "SetWidth" 
                      << this->ScalarBar->GetScalarBarActor()->GetWidth() 
                      << vtkClientServerStream::End;

      pm->GetStream() << vtkClientServerStream::Invoke 
                      << this->ScalarBarActorID
                      << "SetHeight" 
                      << this->ScalarBar->GetScalarBarActor()->GetHeight() 
                      << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::RENDER_SERVER);

      break;
    }
  this->InvokeEvent(event,calldata);
}
void vtkRMScalarBarWidget::SetVisibility(int visible)
{
  if (visible)
    {
    // Current renderer was set to Null when visible was set off.
    // Set the CurrentRenderer to avoid the poked renderer issue.
    this->ScalarBar->SetCurrentRenderer(
      vtkRenderer::SafeDownCast(this->PVProcessModule->GetObjectFromID(this->Renderer2DID)));

    //// This removes all renderers from the render window before enabling 
    //// the widget. It then adds them back into the render window.
    //// I assume the widget needs to know which renderer it uses.
    //// It's the old poked renderer problem.
    //this->GetPVRenderView()->Enable3DWidget(this->RMScalarBarWidget->GetScalarBar());
    
    // Since there is no interactor on the server, add the prop directly.
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                    << this->Renderer2DID << "AddActor"
                    << this->ScalarBarActorID
                    << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(vtkProcessModule::RENDER_SERVER);
    
    // This is here in case a process has no geometry.  
    // We have to explicitly build the color map.  The mapper
    // skips this if there is nothing to render.
    // Shouldn't the scalar build the color map if necessary?
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                    << this->LookupTableID << "Build"
                    << vtkClientServerStream::End;
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                    << this->LookupTableID << "Modified"
                    << vtkClientServerStream::End; 
    this->PVProcessModule->SendStream(vtkProcessModule::RENDER_SERVER);
    }
  else
    {
    this->ScalarBar->SetEnabled(0);
    this->ScalarBar->SetCurrentRenderer(NULL);
    // Since there is no interactor on the server, remove the prop directly.
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                    << this->Renderer2DID << "RemoveActor"
                    << this->ScalarBarActorID 
                    << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(vtkProcessModule::RENDER_SERVER);
    }
}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ScalarBar: " << this->ScalarBar << endl;
  os << indent << "ScalarBarActorID: " << this->ScalarBarActorID << endl;
  os << indent << "ScalarBarTitle: " 
     << (this->ScalarBarTitle ? this->ScalarBarTitle : "none" )
     << endl;
  os << indent << "ScalarBarLabelFormat: " 
     << (this->ScalarBarLabelFormat ? this->ScalarBarLabelFormat : "none" )
     << endl;
  os << indent << "ArrayName: " 
     << (this->ArrayName ? this->ArrayName : "none" )
     << endl;
  os << indent << "NumberOfVectorComponents: " 
     << this->NumberOfVectorComponents << endl;
  
  os << indent << "VectorComponent: " << this->VectorComponent << endl;
  os << indent << "LookupTableID: " << this->LookupTableID << endl;
  os << indent << "LookupTable: " << this->LookupTable << endl;
  os << indent << "ScalarRange: " << this->ScalarRange[0] << ", "
     << this->ScalarRange[1] << endl;
  os << indent << "NumberOfColors: " << this->NumberOfColors << endl;
  os << indent << "EndHSV: " << this->EndHSV[0] << "," << this->EndHSV[1] <<
    "," << this->EndHSV[2] << endl;
  os << indent << "StartHSV: " << this->StartHSV[0] << "," <<
    this->StartHSV[1] << "," << this->StartHSV[2] << endl;

}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::SetVectorModeToMagnitude()
{
  this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                  << this->LookupTableID << "SetVectorModeToMagnitude"
                  << vtkClientServerStream::End;
  this->PVProcessModule->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  this->VectorMode = vtkRMScalarBarWidget::MAGNITUDE;
  this->UpdateScalarBarTitle();
}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::SetVectorModeToComponent()
{
  this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                  << this->LookupTableID << "SetVectorModeToComponent"
                  << vtkClientServerStream::End;
  this->PVProcessModule->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  this->VectorMode = vtkRMScalarBarWidget::COMPONENT;
  this->UpdateScalarBarTitle();
}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::Create(vtkPVProcessModule *pm, vtkClientServerID renderer2DID, 
    vtkClientServerID interactorID)
{
  this->PVProcessModule = pm;  
  this->Renderer2DID = renderer2DID;

  this->LookupTableID = pm->NewStreamObject("vtkLookupTable");
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  this->LookupTable =
    vtkLookupTable::SafeDownCast(pm->GetObjectFromID(this->LookupTableID));

  if (this->NumberOfVectorComponents > 1)
    {
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->LookupTableID << "SetVectorModeToMagnitude"
                    << vtkClientServerStream::End;
    }
  else
    {
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->LookupTableID << "SetVectorModeToComponent"
                    << vtkClientServerStream::End;
    }

  // Actor will be in server manager.  Widget will be in UI.
  this->ScalarBarActorID = pm->NewStreamObject("vtkScalarBarActor");
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->ScalarBarActorID 
                  << "GetPositionCoordinate" 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult 
                  << "SetValue" << 0.87 << 0.25
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->ScalarBarActorID 
                  << "SetWidth" << 0.13 
                  << vtkClientServerStream::End;
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->ScalarBarActorID 
                  << "SetHeight" << 0.5 
                  << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  this->ScalarBar = vtkScalarBarWidget::New();
  this->ScalarBarObserver = vtkScalarBarWidgetObserver::New();
  this->ScalarBarObserver->RMScalarBarWidget = this;

  this->ScalarBar->SetCurrentRenderer(
    vtkRenderer::SafeDownCast(pm->GetObjectFromID(renderer2DID)));
  
  // Actor will be in server manager.  Widget will be in UI.
  this->ScalarBar->SetScalarBarActor(
    vtkScalarBarActor::SafeDownCast(pm->GetObjectFromID(this->ScalarBarActorID)));
  
  this->ScalarBar->SetInteractor(
    vtkRenderWindowInteractor::SafeDownCast(pm->GetObjectFromID(interactorID)));

  this->SetScalarBarLabelFormat(
    this->ScalarBar->GetScalarBarActor()->GetLabelFormat());

  this->UpdateScalarBarTitle();

  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->ScalarBarActorID 
                  << "SetLookupTable" << this->LookupTableID
                  << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  this->ScalarBar->AddObserver(vtkCommand::InteractionEvent, 
                               this->ScalarBarObserver);
  this->ScalarBar->AddObserver(vtkCommand::StartInteractionEvent, 
                               this->ScalarBarObserver);
  this->ScalarBar->AddObserver(vtkCommand::EndInteractionEvent, 
                               this->ScalarBarObserver);

}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::SetNumberOfVectorComponents(int num)
{  
  int idx;

  if (this->PVProcessModule && num != 0)
    {
    vtkErrorMacro("You must set the number of vector components before "
                  "you create this color map.");
    return;
    }

  if (num == this->NumberOfVectorComponents)
    {
    return;
    }

  // Get rid of old arrays.
  // Use for delete.  This number shold not be changed after creation.
  if (this->VectorComponentTitles)
    {
    for (idx = 0; idx < this->NumberOfVectorComponents; ++idx)
      {
      delete [] this->VectorComponentTitles[idx];
      this->VectorComponentTitles[idx] = NULL;
      }
    }

  delete[] this->VectorComponentTitles;
  this->VectorComponentTitles = NULL;
  
  this->NumberOfVectorComponents = num;

  // Set defaults for component titles.
  if (num > 0)
    {
    this->VectorComponentTitles = new char* [num];
    }
  for (idx = 0; idx < num; ++idx)
    {
    this->VectorComponentTitles[idx] = new char[4];  
    }
  if (num == 3)
    { // Use XYZ for default of three component vectors.
    strcpy(this->VectorComponentTitles[0], "X");
    strcpy(this->VectorComponentTitles[1], "Y");
    strcpy(this->VectorComponentTitles[2], "Z");
    }
  else
    {
    for (idx = 0; idx < num; ++idx)
      {
      sprintf(this->VectorComponentTitles[idx], "%d", idx+1);
      }
    }
}
//----------------------------------------------------------------------------
const char* vtkRMScalarBarWidget::GetScalarBarVectorTitle()
{
  if(this->VectorMode == vtkRMScalarBarWidget::MAGNITUDE)
    {
    return this->VectorMagnitudeTitle;
    }
  else
    {
    if(this->VectorComponentTitles == NULL)
      {
      return NULL;
      }
    else
      {
      return this->VectorComponentTitles[this->VectorComponent];
      }
    }
}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::SetScalarBarVectorTitle(const char* name)
{
  if (this->VectorMode == vtkRMScalarBarWidget::MAGNITUDE)
    {
    if (this->VectorMagnitudeTitle == NULL && name == NULL) 
      { 
      return;
      }

    if (this->VectorMagnitudeTitle && name && 
        (!strcmp(this->VectorMagnitudeTitle, name)))
      {
      return;
      }

    if (this->VectorMagnitudeTitle)
      {
      delete [] this->VectorMagnitudeTitle;
      this->VectorMagnitudeTitle = NULL;
      }

    if (name)
      {
      this->VectorMagnitudeTitle = new char[strlen(name) + 1];
      strcpy(this->VectorMagnitudeTitle, name);
      }
    this->UpdateScalarBarTitle();
    }
  else
    {
    if (this->VectorComponentTitles == NULL)
      {
      return;
      }

    if (this->VectorComponentTitles[this->VectorComponent] == NULL && 
        name == NULL) 
      { 
      return;
      }

    if (this->VectorComponentTitles[this->VectorComponent] && 
        name && 
        (!strcmp(this->VectorComponentTitles[this->VectorComponent], name)))
      {
      return;
      }

    if (this->VectorComponentTitles[this->VectorComponent])
      {
      delete [] this->VectorComponentTitles[this->VectorComponent];
      this->VectorComponentTitles[this->VectorComponent] = NULL;
      }

    if (name)
      {
      this->VectorComponentTitles[this->VectorComponent] = 
        new char[strlen(name) + 1];
      strcpy(this->VectorComponentTitles[this->VectorComponent], name);
      }
    this->UpdateScalarBarTitle();
    }
}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::SetScalarBarLabelFormat(const char *name)
{
  if (this->ScalarBarLabelFormat == NULL && name == NULL) 
    { 
    return;
    }

  if (this->ScalarBarLabelFormat && 
      name && 
      (!strcmp(this->ScalarBarLabelFormat,name))) 
    { 
    return;
    }

  if (this->ScalarBarLabelFormat) 
    { 
    delete [] this->ScalarBarLabelFormat; 
    this->ScalarBarLabelFormat = NULL;
    }

  if (name)
    {
    this->ScalarBarLabelFormat = new char[strlen(name) + 1];
    strcpy(this->ScalarBarLabelFormat,name);
    } 

  if (this->PVProcessModule != 0 && this->ScalarBarLabelFormat != NULL)
    {
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke
                    << this->ScalarBarActorID << "SetLabelFormat"
                    << this->ScalarBarLabelFormat
                    << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::SetNumberOfColors(int num)
{
  this->NumberOfColors = num;
  this->UpdateLookupTable();
}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::SetStartHSV(double h, double s, double v)
{
  this->StartHSV[0] = h;
  this->StartHSV[1] = s;
  this->StartHSV[2] = v;
  this->UpdateLookupTable();
}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::SetEndHSV(double h, double s, double v)
{
  this->EndHSV[0] = h;
  this->EndHSV[1] = s;
  this->EndHSV[2] = v;
  this->UpdateLookupTable();
}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::SetScalarBarPosition1(float x, float y)
{
  this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                  << this->ScalarBarActorID << "GetPositionCoordinate"
                  << vtkClientServerStream::End;
  this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult << "SetValue"
                  << x << y
                  << vtkClientServerStream::End;  
  this->PVProcessModule->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::SetScalarBarPosition2(float x, float y)
{
  this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                  << this->ScalarBarActorID << "GetPosition2Coordinate"
                  << vtkClientServerStream::End;
  this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                  << vtkClientServerStream::LastResult << "SetValue"
                  << x << y
                  << vtkClientServerStream::End;  
  this->PVProcessModule->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::SetScalarBarOrientation(int o)
{
  this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                  << this->ScalarBarActorID << "SetOrientation" << o
                  << vtkClientServerStream::End;
  this->PVProcessModule->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}

//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::UpdateLookupTable()
{
  // The a hue is arbitrary, make is consistent
  // so we do not get unexpected interpolated hues.
  if(this->PVProcessModule == NULL)
    {
    return;
    }
  if (this->StartHSV[0]<VTK_USE_LAB_COLOR_MAP)
    {
    if (this->StartHSV[1] == 0.0)
      {
        this->StartHSV[0] = this->EndHSV[0];
      }
    if (this->EndHSV[1] == 0.0)
      {
        this->EndHSV[0] = this->StartHSV[0];
      }
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke
                    << this->LookupTableID << "SetNumberOfTableValues"
                    << this->NumberOfColors
                    << vtkClientServerStream::End;
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke
                    << this->LookupTableID << "SetHueRange"
                    << this->StartHSV[0] << this->EndHSV[0]
                    << vtkClientServerStream::End;
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke
                    << this->LookupTableID << "SetSaturationRange"
                    << this->StartHSV[1] << this->EndHSV[1]
                    << vtkClientServerStream::End;
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke
                    << this->LookupTableID << "SetValueRange"
                    << this->StartHSV[2] << this->EndHSV[2]
                    << vtkClientServerStream::End;
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke
                    << this->LookupTableID << "ForceBuild"
                    << vtkClientServerStream::End;
    
    }
  else
    {
      //now we need to loop through the number of colors setting the colors
      //in the table
      this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke
                      << this->LookupTableID << "SetNumberOfTableValues"
                      << this->NumberOfColors
                      << vtkClientServerStream::End;

      int i;
      double rgba[4];
      double xyz[3];
      double lab[3];
      //only use opaque colors
      rgba[3]=1;
      
      int numColors=this->NumberOfColors-1;
      if (numColors<=0) numColors=1;

      for (i=0;i<this->NumberOfColors;i++){
        //get the color
        for (int j=0;j<3;j++){
          lab[j]=this->StartHSV[j]+(this->EndHSV[j]-this->StartHSV[j])/
            (numColors)*i;
        }
        this->LabToXYZ(lab,xyz);
        this->XYZToRGB(xyz,rgba);
        this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke
                        << this->LookupTableID << "SetTableValue"
                        << i
                        << rgba[0] << rgba[1] << rgba[2] << rgba[3] 
                        << vtkClientServerStream::End;
      }

    }
  this->PVProcessModule->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}

//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::LabToXYZ(double Lab[3], double xyz[3])
{
  
  //LAB to XYZ
  double var_Y = ( Lab[0] + 16 ) / 116;
  double var_X = Lab[1] / 500 + var_Y;
  double var_Z = var_Y - Lab[2] / 200;
    
  if ( pow(var_Y,3) > 0.008856 ) var_Y = pow(var_Y,3);
  else var_Y = ( var_Y - 16 / 116 ) / 7.787;
                                                            
  if ( pow(var_X,3) > 0.008856 ) var_X = pow(var_X,3);
  else var_X = ( var_X - 16 / 116 ) / 7.787;

  if ( pow(var_Z,3) > 0.008856 ) var_Z = pow(var_Z,3);
  else var_Z = ( var_Z - 16 / 116 ) / 7.787;
  double ref_X =  95.047;
  double ref_Y = 100.000;
  double ref_Z = 108.883;
  xyz[0] = ref_X * var_X;     //ref_X =  95.047  Observer= 2° Illuminant= D65
  xyz[1] = ref_Y * var_Y;     //ref_Y = 100.000
  xyz[2] = ref_Z * var_Z;     //ref_Z = 108.883
}

//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::XYZToRGB(double xyz[3], double rgb[3])
{
  
  //double ref_X =  95.047;        //Observer = 2° Illuminant = D65
  //double ref_Y = 100.000;
  //double ref_Z = 108.883;
 
  double var_X = xyz[0] / 100;        //X = From 0 to ref_X
  double var_Y = xyz[1] / 100;        //Y = From 0 to ref_Y
  double var_Z = xyz[2] / 100;        //Z = From 0 to ref_Y
 
  double var_R = var_X *  3.2406 + var_Y * -1.5372 + var_Z * -0.4986;
  double var_G = var_X * -0.9689 + var_Y *  1.8758 + var_Z *  0.0415;
  double var_B = var_X *  0.0557 + var_Y * -0.2040 + var_Z *  1.0570;
 
  if ( var_R > 0.0031308 ) var_R = 1.055 * ( pow(var_R, ( 1 / 2.4 )) ) - 0.055;
  else var_R = 12.92 * var_R;
  if ( var_G > 0.0031308 ) var_G = 1.055 * ( pow(var_G ,( 1 / 2.4 )) ) - 0.055;
  else  var_G = 12.92 * var_G;
  if ( var_B > 0.0031308 ) var_B = 1.055 * ( pow(var_B, ( 1 / 2.4 )) ) - 0.055;
  else var_B = 12.92 * var_B;
                                                                                                 
  rgb[0] = var_R;
  rgb[1] = var_G;
  rgb[2] = var_B;
  
  //clip colors. ideally we would do something different for colors
  //out of gamut, but not really sure what to do atm.
  if (rgb[0]<0) rgb[0]=0;
  if (rgb[1]<0) rgb[1]=0;
  if (rgb[2]<0) rgb[2]=0;
  if (rgb[0]>1) rgb[0]=1;
  if (rgb[1]>1) rgb[1]=1;
  if (rgb[2]>1) rgb[2]=1;

}

//----------------------------------------------------------------------------
int vtkRMScalarBarWidget::MatchArrayName(const char* str, int numberOfComponents)
{
  if (str == NULL || this->ArrayName == NULL)
    {
    return 0;
    }
  if (strcmp(str, this->ArrayName) == 0 && 
      numberOfComponents == this->NumberOfVectorComponents)
    {
    return 1;
    }
  return 0;
}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::SetScalarBarTitle(const char* name)
{
  if (this->ScalarBarTitle == NULL && name == NULL) 
    { 
    return;
    }

  if (this->ScalarBarTitle && name && (!strcmp(this->ScalarBarTitle, name))) 
    { 
    return;
    }

  if (this->ScalarBarTitle) 
    { 
    delete [] this->ScalarBarTitle; 
    this->ScalarBarTitle = NULL;
    }

  if (name)
    {
    this->ScalarBarTitle = new char[strlen(name) + 1];
    strcpy(this->ScalarBarTitle, name);
    } 
  this->UpdateScalarBarTitle();
}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::UpdateScalarBarTitle()
{
  if (this->ScalarBarTitle == NULL || this->PVProcessModule == NULL)
    {
    return;
    }

  if (this->VectorMode == vtkRMScalarBarWidget::MAGNITUDE &&
      this->NumberOfVectorComponents > 1)
    {
    ostrstream ostr;
    ostr << this->ScalarBarTitle << " " << this->VectorMagnitudeTitle << ends;
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke
                    << this->ScalarBarActorID << "SetTitle" << ostr.str()
                    << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    ostr.rdbuf()->freeze(0);    
    }
  else if (this->NumberOfVectorComponents == 1)
    {
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke
                    << this->ScalarBarActorID << "SetTitle" 
                    << this->ScalarBarTitle
                    << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
  else
    {
    ostrstream ostr;
    ostr << this->ScalarBarTitle << " " 
         << this->VectorComponentTitles[this->VectorComponent] << ends;
    this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke
                    << this->ScalarBarActorID << "SetTitle" << ostr.str()
                    << vtkClientServerStream::End;
    this->PVProcessModule->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    ostr.rdbuf()->freeze(0);    
    }
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::SetScalarRange(double min, double max)
{
  if (this->ScalarRange[0] == min && this->ScalarRange[1] == max)
    {
    return;
    }

  this->ScalarRange[0] = min;
  this->ScalarRange[1] = max;
  this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                  << this->LookupTableID << "SetTableRange"
                  << min << max
                  << vtkClientServerStream::End;
  this->PVProcessModule->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::SetVectorComponent(int component)
{
  if (component < 0)
    {
    component = 0;
    }
  if (component >= this->NumberOfVectorComponents)
    {
    component = this->NumberOfVectorComponents-1;
    }

  if (this->VectorComponent == component)
    {
    return;
    }

  this->VectorComponent = component;

  // Change the title of the scalar bar.
  this->UpdateScalarBarTitle();

  if ( !this->PVProcessModule)
    {
    return;
    }
  this->PVProcessModule->GetStream() << vtkClientServerStream::Invoke 
                  << this->LookupTableID << "SetVectorComponent"
                  << component
                  << vtkClientServerStream::End;
  this->PVProcessModule->SendStream(
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}
//----------------------------------------------------------------------------
void vtkRMScalarBarWidget::SetArrayName(const char* str)
{
  if ( this->ArrayName == NULL && str == NULL) 
    { 
    return;
    }
  if ( this->ArrayName && str && (!strcmp(this->ArrayName,str))) 
    { 
    return;
    }
  if (this->ArrayName)
    {
    delete [] this->ArrayName;
    this->ArrayName = NULL;
    }
  if (str)
    {
    this->ArrayName = new char[strlen(str)+1];
    strcpy(this->ArrayName,str);
    }
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent);
}
