/*=========================================================================

  Program:   ParaView
  Module:    vtkPVVolumeAppearanceEditor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVVolumeAppearanceEditor.h"
 
#include "vtkObjectFactory.h"
#include "vtkKWPushButton.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPVSource.h"
#include "vtkPVDisplayGUI.h"
#include "vtkKWScale.h"
#include "vtkSMPartDisplay.h"
#include "vtkKWLabeledFrame.h"
#include "vtkPVArrayInformation.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPVProcessModule.h"
#include "vtkColorTransferFunction.h"
#include "vtkPVDataInformation.h"
#include "vtkVolumeProperty.h"
#include "vtkPVVolumePropertyWidget.h"
#include "vtkKWPiecewiseFunctionEditor.h"
#include "vtkKWColorTransferFunctionEditor.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVVolumeAppearanceEditor);
vtkCxxRevisionMacro(vtkPVVolumeAppearanceEditor, "1.17");

int vtkPVVolumeAppearanceEditorCommand(ClientData cd, Tcl_Interp *interp,
                                       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVVolumeAppearanceEditor::vtkPVVolumeAppearanceEditor()
{
  this->CommandFunction              = vtkPVVolumeAppearanceEditorCommand;
  this->PVRenderView                 = NULL;

  // Don't actually create any of the widgets 
  // This allows a subclass to replace this one

  this->BackButton                   = NULL;
  this->PVSource                     = NULL;

  this->VolumePropertyWidget         = NULL;
  this->InternalVolumeProperty       = NULL;
}

//----------------------------------------------------------------------------
vtkPVVolumeAppearanceEditor::~vtkPVVolumeAppearanceEditor()
{
  if ( this->BackButton )
    {
    this->BackButton->Delete();
    this->BackButton = NULL;
    }

  if ( this->VolumePropertyWidget )
    {
    this->VolumePropertyWidget->Delete();
    this->VolumePropertyWidget = NULL;
    }

  if (this->InternalVolumeProperty)
    {
    this->InternalVolumeProperty->Delete();
    this->InternalVolumeProperty = NULL;
    }

  this->SetPVRenderView(NULL);
}
//----------------------------------------------------------------------------
// No register count because of reference loop.
void vtkPVVolumeAppearanceEditor::SetPVRenderView(vtkPVRenderView *rv)
{
  this->PVRenderView = rv;
}
//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", "-bd 0 -relief flat"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  // Superclass create takes a KWApplication, but we need a PVApplication.

  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(app);
  if (pvApp == NULL)
    {
    vtkErrorMacro("Need a PV application");
    return;
    }
  
  // Back button
  this->BackButton = vtkKWPushButton::New();
  this->BackButton->SetParent(this);
  this->BackButton->Create(this->GetApplication(), "-text {Back}");
  this->BackButton->SetCommand(this, "BackButtonCallback");


  this->VolumePropertyWidget = vtkPVVolumePropertyWidget::New();
  this->VolumePropertyWidget->SetParent(this);
  this->VolumePropertyWidget->Create(pvApp, 0);
  this->VolumePropertyWidget->ShowComponentSelectionOff();
  this->VolumePropertyWidget->ShowInterpolationTypeOff();
  this->VolumePropertyWidget->ShowMaterialPropertyOff();
  this->VolumePropertyWidget->ShowGradientOpacityFunctionOff();
  this->VolumePropertyWidget->ShowComponentWeightsOff();
  this->VolumePropertyWidget->GetScalarOpacityFunctionEditor()->ShowWindowLevelModeButtonOff();
  this->VolumePropertyWidget->SetVolumePropertyChangedCommand(
    this, "VolumePropertyChangedCallback");
  this->VolumePropertyWidget->SetVolumePropertyChangingCommand(
    this, "VolumePropertyChangingCallback");

  this->Script("pack %s %s -side top -anchor n -fill x -padx 2 -pady 2", 
               this->VolumePropertyWidget->GetWidgetName(),
               this->BackButton->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::VolumePropertyChangedCallback()
{
  vtkPVApplication *pvApp = NULL;
  
  if ( this->GetApplication() )
    {
    pvApp =
      vtkPVApplication::SafeDownCast(this->GetApplication());    
    }
  
  if ( this->PVSource && this->ArrayInfo && pvApp )
    {
    int numParts = this->PVSource->GetNumberOfParts();
    int i;
    vtkSMPart *part;
    vtkKWPiecewiseFunctionEditor *kwfunc = 
      this->VolumePropertyWidget->GetScalarOpacityFunctionEditor();
    vtkPiecewiseFunction *func = kwfunc->GetPiecewiseFunction();
    double *points = func->GetDataPointer();

    //unit distance:
    vtkKWScale* scale = 
      this->VolumePropertyWidget->GetScalarOpacityUnitDistanceScale();
    double unitDistance = scale->GetValue();


    // Color Ramp (vtkColorTransferFunction)
    vtkKWColorTransferFunctionEditor *kwcolor = 
      this->VolumePropertyWidget->GetScalarColorFunctionEditor();
    vtkColorTransferFunction* color = kwcolor->GetColorTransferFunction();
    double *rgb = color->GetDataPointer();

    for (i = 0; i < numParts; i++)
      {
      part = this->PVSource->GetPart(i);
      // Access the vtkPiecewiseFunction:
      vtkClientServerID volumeOpacityID =
        this->PVSource->GetPartDisplay()->GetVolumeOpacityProxy()->GetID(0);

      vtkPVProcessModule* pm = pvApp->GetProcessModule();
      vtkClientServerStream& stream = pm->GetStream();
      
      // 1. ScalarOpacity
      // Remove all previous points:
      stream << vtkClientServerStream::Invoke << volumeOpacityID 
             << "RemoveAllPoints" << vtkClientServerStream::End;
      for(int j=0; j<func->GetSize(); j++)
        {
        // Copy points one by one from the vtkPiecewiseFunction:
        stream << vtkClientServerStream::Invoke << volumeOpacityID 
               << "AddPoint" << points[2*j] << points[2*j+1] 
               << vtkClientServerStream::End;
        }

      //2. ScalarOpacityUnitDistance
      vtkClientServerID volumePropertyID = 
        this->PVSource->GetPartDisplay()->GetVolumePropertyProxy()->GetID(0);
      stream << vtkClientServerStream::Invoke << volumePropertyID 
             << "SetScalarOpacityUnitDistance" << unitDistance 
             << vtkClientServerStream::End;

      //3. Color Ramp, similar to ScalarOpacity
      vtkClientServerID volumeColorID = 
        this->PVSource->GetPartDisplay()->GetVolumeColorProxy()->GetID(0);

      stream << vtkClientServerStream::Invoke << volumeColorID 
             << "RemoveAllPoints" << vtkClientServerStream::End;
      for(int k=0; k<color->GetSize(); k++)
        {
        stream << vtkClientServerStream::Invoke << volumeColorID 
               << "AddRGBPoint" << rgb[4*k] << rgb[4*k+1] 
                                << rgb[4*k+2] << rgb[4*k+3]
               << vtkClientServerStream::End;
        }
 
      //Send everything:
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    this->RenderView();
    }
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::VolumePropertyChangingCallback()
{
  vtkPVApplication *pvApp = NULL;
  
  if ( this->GetApplication() )
    {
    pvApp =
      vtkPVApplication::SafeDownCast(this->GetApplication());    
    }
  
  if ( this->PVSource && this->ArrayInfo && pvApp )
    {
    int numParts = this->PVSource->GetNumberOfParts();
    int i;
    vtkSMPart *part;
    vtkKWPiecewiseFunctionEditor *kwfunc = 
      this->VolumePropertyWidget->GetScalarOpacityFunctionEditor();
    vtkPiecewiseFunction *func = kwfunc->GetPiecewiseFunction();
    double *points = func->GetDataPointer();

    //unit distance:
    vtkKWScale* scale = 
      this->VolumePropertyWidget->GetScalarOpacityUnitDistanceScale();
    double unitDistance = scale->GetValue();


    // Color Ramp (vtkColorTransferFunction)
    vtkKWColorTransferFunctionEditor *kwcolor = 
      this->VolumePropertyWidget->GetScalarColorFunctionEditor();
    vtkColorTransferFunction* color = kwcolor->GetColorTransferFunction();
    double *rgb = color->GetDataPointer();

    for (i = 0; i < numParts; i++)
      {
      part = this->PVSource->GetPart(i);
      // Access the vtkPiecewiseFunction:
      vtkClientServerID volumeOpacityID =
        this->PVSource->GetPartDisplay()->GetVolumeOpacityProxy()->GetID(0);

      vtkPVProcessModule* pm = pvApp->GetProcessModule();
      vtkClientServerStream& stream = pm->GetStream();
      
      // 1. ScalarOpacity
      // Remove all previous points:
      stream << vtkClientServerStream::Invoke << volumeOpacityID 
             << "RemoveAllPoints" << vtkClientServerStream::End;
      for(int j=0; j<func->GetSize(); j++)
        {
        // Copy points one by one from the vtkPiecewiseFunction:
        stream << vtkClientServerStream::Invoke << volumeOpacityID 
               << "AddPoint" << points[2*j] << points[2*j+1] << vtkClientServerStream::End;
        }

      //2. ScalarOpacityUnitDistance
      vtkClientServerID volumePropertyID = 
        this->PVSource->GetPartDisplay()->GetVolumePropertyProxy()->GetID(0);
      stream << vtkClientServerStream::Invoke << volumePropertyID 
             << "SetScalarOpacityUnitDistance" << unitDistance << vtkClientServerStream::End;

      //3. Color Ramp, similar to ScalarOpacity
      vtkClientServerID volumeColorID = 
        this->PVSource->GetPartDisplay()->GetVolumeColorProxy()->GetID(0);

      stream << vtkClientServerStream::Invoke << volumeColorID 
             << "RemoveAllPoints" << vtkClientServerStream::End;
      for(int k=0; k<color->GetSize(); k++)
        {
        stream << vtkClientServerStream::Invoke << volumeColorID 
               << "AddRGBPoint" << rgb[4*k] << rgb[4*k+1] 
                                << rgb[4*k+2] << rgb[4*k+3]
               << vtkClientServerStream::End;
        }
 
      //Send everything:
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    this->RenderView();
    }
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::BackButtonCallback()
{
  if (this->PVRenderView == NULL)
    {
    return;
    }

  // This has a side effect of gathering and display information.
  this->PVRenderView->GetPVWindow()->GetCurrentPVSource()->GetPVOutput()->Update();
  
  // Use the Callback version of the method to get the trace
  this->PVRenderView->GetPVWindow()->ShowCurrentSourcePropertiesCallback();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::RenderView()
{
  if (this->PVRenderView)
    {
    this->PVRenderView->EventuallyRender();
    }
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::Close()
{
  this->VolumePropertyWidget->SetVolumeProperty(NULL);
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::SetPVSourceAndArrayInfo(vtkPVSource *source,
                                                          vtkPVArrayInformation *arrayInfo)
{
  this->PVSource = source;
  this->ArrayInfo = arrayInfo;
  
  vtkPVApplication *pvApp = NULL;
  
  if ( this->GetApplication() )
    {
    pvApp =
      vtkPVApplication::SafeDownCast(this->GetApplication());    
    }

  vtkPVDataInformation* dataInfo = source->GetDataInformation();
  
  if ( this->PVSource && this->ArrayInfo && pvApp && dataInfo &&
       this->PVSource->GetNumberOfParts() > 0 )
    {
    vtkPiecewiseFunction *opacityFunc = 
      vtkPiecewiseFunction::SafeDownCast(
        pvApp->GetProcessModule()->
        GetObjectFromID(this->PVSource->GetPartDisplay()->
                        GetVolumeOpacityProxy()->GetID(0)));

    vtkColorTransferFunction *colorFunc = 
      vtkColorTransferFunction::SafeDownCast(
        pvApp->GetProcessModule()->
        GetObjectFromID(this->PVSource->GetPartDisplay()->
                        GetVolumeColorProxy()->GetID(0)));

    int size = opacityFunc->GetSize();
    
    if ( size < 2 )
      {
      vtkErrorMacro("Expecting at least 2 points in opacity function:" << size);
      return;
      }
    
    double *ptr = opacityFunc->GetDataPointer();
    
    size = colorFunc->GetSize();
    if ( size < 2 )
      {
      vtkErrorMacro("Expecting at least 2 points in color function!");
      return;
      }
    ptr = colorFunc->GetDataPointer();
    
    double bounds[6];
    dataInfo->GetBounds(bounds);
    
    double diameter = 
      sqrt( (bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
            (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
            (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]) );
    
    int numCells = dataInfo->GetNumberOfCells();
    double linearNumCells = pow( (double) numCells, 1.0/3.0 );
    
    double soud_res = diameter / (linearNumCells * 10.0);
    double soud_range[2];
    soud_range[0] = diameter / (linearNumCells * 10.0);
    soud_range[1] = diameter / (linearNumCells / 10.0);
      
    vtkVolumeProperty *volumeProperty = 
      vtkVolumeProperty::SafeDownCast(
        pvApp->GetProcessModule()->
        GetObjectFromID(this->PVSource->GetPartDisplay()->
                        GetVolumePropertyProxy()->GetID(0)));

    // It would be nicer if there would be a VolumeProperty proxy:
    if (!this->InternalVolumeProperty)
      {
      this->InternalVolumeProperty = vtkVolumeProperty::New();
      this->VolumePropertyWidget->SetVolumeProperty(
        this->InternalVolumeProperty);
      }
    this->InternalVolumeProperty->SetScalarOpacityUnitDistance(
      volumeProperty->GetScalarOpacityUnitDistance());
    this->InternalVolumeProperty->SetScalarOpacity(opacityFunc);
    this->InternalVolumeProperty->SetColor(colorFunc);

    this->VolumePropertyWidget->SetDataInformation(dataInfo);

    this->VolumePropertyWidget->GetScalarOpacityUnitDistanceScale()->
      SetResolution(soud_res);
    this->VolumePropertyWidget->GetScalarOpacityUnitDistanceScale()->
      SetRange( soud_range[0], soud_range[1]);
    this->VolumePropertyWidget->Update();
    }
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->BackButton);
  this->PropagateEnableState(this->VolumePropertyWidget);
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

