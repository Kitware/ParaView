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
#include "vtkKWFrameLabeled.h"
#include "vtkPVArrayInformation.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPVProcessModule.h"
#include "vtkColorTransferFunction.h"
#include "vtkPVDataInformation.h"
#include "vtkVolumeProperty.h"
#include "vtkPVVolumePropertyWidget.h"
#include "vtkKWPiecewiseFunctionEditor.h"
#include "vtkKWColorTransferFunctionEditor.h"

#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMDisplayProxy.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVVolumeAppearanceEditor);
vtkCxxRevisionMacro(vtkPVVolumeAppearanceEditor, "1.27.2.1");

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
  this->ArrayInfo                    = NULL;

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
  this->VolumePropertyWidget->ShowComponentSelectionOff();
  this->VolumePropertyWidget->ShowInterpolationTypeOff();
  this->VolumePropertyWidget->ShowMaterialPropertyOff();
  this->VolumePropertyWidget->ShowGradientOpacityFunctionOff();
  this->VolumePropertyWidget->ShowComponentWeightsOff();
  this->VolumePropertyWidget->GetScalarOpacityFunctionEditor()->ShowWindowLevelModeButtonOff();
  this->VolumePropertyWidget->Create(pvApp, 0);
  this->VolumePropertyWidget->SetVolumePropertyChangedCommand(
    this, "VolumePropertyChangedCallback");
  this->VolumePropertyWidget->SetVolumePropertyChangingCommand(
    this, "VolumePropertyChangingCallback");

  this->Script("pack %s %s -side top -anchor n -fill x -padx 2 -pady 2", 
               this->VolumePropertyWidget->GetWidgetName(),
               this->BackButton->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::VolumePropertyInternalCallback()
{
  vtkPVApplication *pvApp = NULL;

  if ( this->GetApplication() )
    {
    pvApp =
      vtkPVApplication::SafeDownCast(this->GetApplication());
    }

  vtkSMDisplayProxy* pDisp  = this->PVSource->GetDisplayProxy();

  // Scalar Opacity (vtkPiecewiseFunction)
  vtkKWPiecewiseFunctionEditor *kwfunc =
    this->VolumePropertyWidget->GetScalarOpacityFunctionEditor();
  vtkPiecewiseFunction *func = kwfunc->GetPiecewiseFunction();
  double *points = func->GetDataPointer();

  // Unit distance:
  vtkKWScale* scale =
    this->VolumePropertyWidget->GetScalarOpacityUnitDistanceScale();
  double unitDistance = scale->GetValue();

  // Color Ramp (vtkColorTransferFunction)
  vtkKWColorTransferFunctionEditor *kwcolor =
    this->VolumePropertyWidget->GetScalarColorFunctionEditor();
  vtkColorTransferFunction* color = kwcolor->GetColorTransferFunction();
  double *rgb = color->GetDataPointer();

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("Points"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Points on DisplayProxy.");
    return;
    }
    
  // 1. ScalarOpacity
  this->AddTraceEntry("$kw(%s) RemoveAllScalarOpacityPoints",
    this->GetTclName());
  for(int j=0; j<func->GetSize(); j++)
    {
    // we don't directly call the AppendScalarOpacityPoint method, since 
    // it's slow (as it calls UpdateVTKObjects for each point.
    this->AddTraceEntry("$kw(%s) AppendScalarOpacityPoint %f %f", this->GetTclName(), 
      points[2*j], points[2*j+1]);
    }
  dvp->SetNumberOfElements(func->GetSize()*2);
  dvp->SetElements(points);


  //2. Color Ramp, similar to ScalarOpacity
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("RGBPoints"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property RGBPoints on DisplayProxy.");
    return;
    }

  this->AddTraceEntry("$kw(%s) RemoveAllColorPoints",
    this->GetTclName());

  for(int k=0; k<color->GetSize(); k++)
    {
    this->AddTraceEntry("$kw(%s) AppendColorPoint %f %f %f %f",
      this->GetTclName(),
      rgb[4*k], rgb[4*k+1], rgb[4*k+2], rgb[4*k+3]);
    }
  dvp->SetNumberOfElements(color->GetSize()*4);
  dvp->SetElements(rgb);

  //3. ScalarOpacityUnitDistance
  this->SetScalarOpacityUnitDistance( unitDistance );

  //4. HSVWrap
  this->SetHSVWrap( color->GetHSVWrap() );

  //5. ColorSpace.
  this->SetColorSpace( color->GetColorSpace() );
  pDisp->UpdateVTKObjects();
  this->RenderView();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::SetColorSpace(int w)
{
  if ( !this->PVSource)
    {
    return;
    }

  // Save trace 
  vtkSMDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();

  this->AddTraceEntry("$kw(%s) SetColorSpace %d", 
    this->GetTclName(), w );

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    pDisp->GetProperty("ColorSpace"));

  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ColorSpace on "
      "DisplayProxy.");
    return;
    }
  ivp->SetElement(0, w);
  pDisp->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::SetHSVWrap(int w)
{
  if ( !this->PVSource)
    {
    return;
    }

  // Save trace 
  vtkSMDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();

  this->AddTraceEntry("$kw(%s) SetHSVWrap %d", 
    this->GetTclName(), w );

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    pDisp->GetProperty("HSVWrap"));

  if (!ivp)
    {
    vtkErrorMacro("Failed to find property HSVWrap on "
      "DisplayProxy.");
    return;
    }
  ivp->SetElement(0, w);
  pDisp->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::VolumePropertyChangedCallback()
{
  this->PVRenderView->GetPVWindow()->InteractiveRenderEnabledOn();
  this->VolumePropertyInternalCallback();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::VolumePropertyChangingCallback()
{
  this->PVRenderView->GetPVWindow()->InteractiveRenderEnabledOff();
  this->VolumePropertyInternalCallback();
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
  if ( !this->PVSource || !this->ArrayInfo || !pvApp || !dataInfo ||
    this->PVSource->GetNumberOfParts() <= 0 )
    {
    return;
    }

  // Create the VolumeProperty/OpacityFunction/ColorTransferFuntion
  // that will be manipulated by the widget.

  this->VolumePropertyWidget->SetDataInformation(dataInfo);
  this->VolumePropertyWidget->SetArrayName(this->ArrayInfo->GetName());
  if (this->PVSource->GetDisplayProxy()->cmGetScalarMode() == 
    vtkSMDisplayProxy::POINT_FIELD_DATA)
    {
    this->VolumePropertyWidget->SetScalarModeToUsePointFieldData();
    }
  else
    {
    this->VolumePropertyWidget->SetScalarModeToUseCellFieldData();
    }

  if (!this->InternalVolumeProperty)
    {
    this->InternalVolumeProperty = vtkVolumeProperty::New();

    vtkPiecewiseFunction* opacityFunc = vtkPiecewiseFunction::New();
    vtkColorTransferFunction* colorFunc = vtkColorTransferFunction::New();
    this->InternalVolumeProperty->SetScalarOpacity(opacityFunc);
    this->InternalVolumeProperty->SetColor(colorFunc);
    opacityFunc->Delete();
    colorFunc->Delete();

    this->VolumePropertyWidget->SetVolumeProperty(
      this->InternalVolumeProperty); 
    }

  this->UpdateFromProxy();
  this->VolumePropertyWidget->Update();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::UpdateFromProxy()
{
  unsigned int i;
  vtkPiecewiseFunction* opacityFunc = 
    this->InternalVolumeProperty->GetScalarOpacity();
  vtkColorTransferFunction* colorFunc =
    this->InternalVolumeProperty->GetRGBTransferFunction();
  
  colorFunc->RemoveAllPoints();
  opacityFunc->RemoveAllPoints();
  
  
  vtkSMDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();
  vtkSMDoubleVectorProperty* dvp ;
  vtkSMIntVectorProperty* ivp;
  
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("Points")); // OpacityFunction:Points.
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property Points on DisplayProxy.");
    return;
    }
  
  for (i=0; (i + 1) < dvp->GetNumberOfElements(); i+=2)
    {
    opacityFunc->AddPoint(dvp->GetElement(i), dvp->GetElement(i+1));
    }

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("RGBPoints"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property RGBPoints on DisplayProxy.");
    return;
    }

  for (i=0; (i+3) < dvp->GetNumberOfElements(); i+=4)
    {
    colorFunc->AddRGBPoint(dvp->GetElement(i),
      dvp->GetElement(i+1), dvp->GetElement(i+2), dvp->GetElement(i+3));
    }
  
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    pDisp->GetProperty("ColorSpace"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property ColorSpace on DisplayProxy.");
    return;
    }
  colorFunc->SetColorSpace(ivp->GetElement(0));

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    pDisp->GetProperty("HSVWrap"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property HSVWrap on DisplayProxy.");
    return;
    }
  colorFunc->SetHSVWrap(ivp->GetElement(0));

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("ScalarOpacityUnitDistance"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property ScalarOpacityUnitDistance on DisplayProxy.");
    return;
    }

  this->InternalVolumeProperty->SetScalarOpacityUnitDistance(dvp->GetElement(0));
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

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::SetScalarOpacityUnitDistance(double d)
{
  if ( !this->PVSource && this->ArrayInfo )
    {
    return;
    }

  // Save trace 
  vtkSMDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();
  
  this->AddTraceEntry("$kw(%s) SetScalarOpacityUnitDistance %f", 
    this->GetTclName(), d );
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("ScalarOpacityUnitDistance"));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property ScalarOpacityUnitDistance on "
      "DisplayProxy.");
    return;
    }
  dvp->SetElement(0, d);
  pDisp->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::AppendColorPoint(double s, double r, 
  double g, double b)
{
  if ( !this->PVSource)
    {
    vtkErrorMacro("PVSource not set!");
    return ;
    }

  // Save trace:
  this->AddTraceEntry("$kw(%s) AppendColorPoint %f %f %f %f", 
    this->GetTclName(), s, r, g, b);

  vtkSMDisplayProxy *pDisp = this->PVSource->GetDisplayProxy();
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("RGBPoints")); 
  int num = dvp->GetNumberOfElements();
  
  dvp->SetNumberOfElements(num+4);
  dvp->SetElement(num, s);
  dvp->SetElement(num+1, r);
  dvp->SetElement(num+2, g);
  dvp->SetElement(num+3, b);

  pDisp->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::RemoveAllColorPoints()
{
  if ( !this->PVSource)
    {
    vtkErrorMacro("PVSource not set!");
    return ;
    }

  // Save trace:
  vtkSMDisplayProxy *pDisp = this->PVSource->GetDisplayProxy();
  this->AddTraceEntry("$kw(%s) RemoveAllColorPoints ",
    this->GetTclName());

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("RGBPoints")); 
  dvp->SetNumberOfElements(0);
  pDisp->UpdateVTKObjects(); 
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::RemoveAllScalarOpacityPoints()
{
  if (!this->PVSource)
    {
    vtkErrorMacro("Source not set!");
    return;
    }

  vtkSMDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();

  // Save trace:
  this->AddTraceEntry("$kw(%s) RemoveAllScalarOpacityPoints ", 
    this->GetTclName());

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("Points")); 
  dvp->SetNumberOfElements(0);
  pDisp->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::AppendScalarOpacityPoint(double scalar,
  double opacity)
{
  if (!this->PVSource)
    {
    vtkErrorMacro("Source not set!");
    return;
    }
  
  vtkSMDisplayProxy* pDisp = this->PVSource->GetDisplayProxy();

  // Save trace:
  this->AddTraceEntry("$kw(%s) AppendScalarOpacityPoint %f %f", this->GetTclName(), 
    scalar, opacity);
  
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    pDisp->GetProperty("Points")); 
  int num = dvp->GetNumberOfElements();
  dvp->SetNumberOfElements(num+2);
  dvp->SetElement(num, scalar);
  dvp->SetElement(num+1, opacity);
  pDisp->UpdateVTKObjects();

}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::SaveState(ofstream *file)
{
  vtkPVApplication *pvApp = NULL;

  if ( this->GetApplication() )
    {
    pvApp =
      vtkPVApplication::SafeDownCast(this->GetApplication());
    }

  if ( !this->PVSource || !this->ArrayInfo || !pvApp )
    {
    return;
    }
  *file << "set kw(" << this->GetTclName() << ") [$kw("
    << this->PVRenderView->GetPVWindow()->GetTclName()
    << ") GetVolumeAppearanceEditor]" << endl;

  // this is handled in vtkPVDisplayGUI for each individual source:
  //    *file << "[$kw(" << this->PVSource->GetTclName() << ") GetPVOutput] "
  //      << "VolumeRenderPointField {" << this->ArrayInfo->GetName() << "} "
  //      << this->ArrayInfo->GetNumberOfComponents() << endl;

  *file << "[$kw(" << this->PVSource->GetTclName() << ") GetPVOutput] "
    << "ShowVolumeAppearanceEditor" << endl;

  // Scalar Opacity (vtkPiecewiseFunction)
  vtkKWPiecewiseFunctionEditor *kwfunc =
    this->VolumePropertyWidget->GetScalarOpacityFunctionEditor();
  vtkPiecewiseFunction *func = kwfunc->GetPiecewiseFunction();
  double *points = func->GetDataPointer();

  // Unit distance:
  vtkKWScale* scale =
    this->VolumePropertyWidget->GetScalarOpacityUnitDistanceScale();
  double unitDistance = scale->GetValue();

  // Color Ramp (vtkColorTransferFunction)
  vtkKWColorTransferFunctionEditor *kwcolor =
    this->VolumePropertyWidget->GetScalarColorFunctionEditor();
  vtkColorTransferFunction* color = kwcolor->GetColorTransferFunction();
  double *rgb = color->GetDataPointer();

  // 1. ScalarOpacity
  *file << "$kw(" << this->GetTclName() << ") "
    << "RemoveAllScalarOpacityPoints" << endl;

  for(int j=0; j<func->GetSize(); j++)
    {
    // Copy points one by one from the vtkPiecewiseFunction:
    *file << "$kw(" << this->GetTclName() << ") "
      << "AppendScalarOpacityPoint " << points[2*j] << " " << points[2*j+1]
      << endl;
    }

  //2. ScalarOpacityUnitDistance
  *file << "$kw(" << this->GetTclName() << ") "
    << "SetScalarOpacityUnitDistance " << unitDistance
    << endl;

  //3. Color Ramp, similar to ScalarOpacity
  *file << "$kw(" << this->GetTclName() << ") "
    << "RemoveAllColorPoints" << endl;

  for(int k=0; k<color->GetSize(); k++)
    {
    *file << "$kw(" << this->GetTclName() << ") "
      << "AppendColorPoint " << rgb[4*k] << " " << rgb[4*k+1]
      << " " << rgb[4*k+2] << " " << rgb[4*k+3]
      << endl;
    }
  *file << "$kw(" << this->GetTclName() << ") "
    << "SetHSVWrap " << color->GetHSVWrap() << endl;

  *file << "$kw(" << this->GetTclName() << ") " 
    << "SetColorSpace " << color->GetColorSpace() << endl;
}

