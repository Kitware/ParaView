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
 
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPVSource.h"
#include "vtkPVData.h"
#include "vtkPVPart.h"
#include "vtkPVPartDisplay.h"
#include "vtkKWRange.h"
#include "vtkKWLabeledFrame.h"
#include "vtkPVArrayInformation.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPVProcessModule.h"
#include "vtkKWLabel.h"
#include "vtkKWScale.h"
#include "vtkKWFrame.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWMenuButton.h"
#include "vtkKWChangeColorButton.h"
#include "vtkColorTransferFunction.h"
#include "vtkKWWidget.h"
#include "vtkPVDataInformation.h"
#include "vtkVolumeProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVVolumeAppearanceEditor);
vtkCxxRevisionMacro(vtkPVVolumeAppearanceEditor, "1.11");

int vtkPVVolumeAppearanceEditorCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVVolumeAppearanceEditor::vtkPVVolumeAppearanceEditor()
{
  this->CommandFunction              = vtkPVVolumeAppearanceEditorCommand;
  this->PVRenderView                 = NULL;

  // Don't actually create any of the widgets 
  // This allows a subclass to replace this one

  this->ScalarOpacityFrame             = NULL;
  this->ColorFrame                     = NULL;
  this->BackButton                     = NULL;
  
  this->ScalarOpacityRampLabel         = NULL;
  this->ScalarOpacityRampRange         = NULL;  
  this->ScalarOpacityStartValueLabel   = NULL;
  this->ScalarOpacityStartValueScale   = NULL;
  this->ScalarOpacityEndValueLabel     = NULL;
  this->ScalarOpacityEndValueScale     = NULL;
  this->ScalarOpacityUnitDistanceLabel = NULL;
  this->ScalarOpacityUnitDistanceScale = NULL;
  
  this->ColorRampLabel                 = NULL;
  this->ColorRampRange                 = NULL;
  this->ColorEditorFrame               = NULL;
  this->ColorStartValueButton          = NULL;
  this->ColorEndValueButton            = NULL;
  this->ColorMapLabel                  = NULL;
  
  this->ScalarRange[0]                 = 0.0;
  this->ScalarRange[1]                 = 1.0;
  
  this->MapData                        = NULL;
  this->MapDataSize                    = 0;
  this->MapHeight                      = 25;
  this->MapWidth                       = 20;
}

//----------------------------------------------------------------------------
vtkPVVolumeAppearanceEditor::~vtkPVVolumeAppearanceEditor()
{
  if ( this->ScalarOpacityFrame )
    {
    this->ScalarOpacityFrame->Delete();
    this->ScalarOpacityFrame = NULL;
    }

  if ( this->ColorFrame )
    {
    this->ColorFrame->Delete();
    this->ColorFrame = NULL;
    }

  if ( this->BackButton )
    {
    this->BackButton->Delete();
    this->BackButton = NULL;
    }

  if ( this->ScalarOpacityRampLabel )
    {
    this->ScalarOpacityRampLabel->Delete();
    this->ScalarOpacityRampLabel = NULL;
    }
  
  if ( this->ScalarOpacityRampRange )
    {
    this->ScalarOpacityRampRange->Delete();
    this->ScalarOpacityRampRange = NULL;
    }
  
  if ( this->ScalarOpacityStartValueLabel )
    {
    this->ScalarOpacityStartValueLabel->Delete();
    this->ScalarOpacityStartValueLabel = NULL;    
    }

  if ( this->ScalarOpacityStartValueScale )
    {
    this->ScalarOpacityStartValueScale->Delete();
    this->ScalarOpacityStartValueScale = NULL;    
    }

  if ( this->ScalarOpacityEndValueLabel )
    {
    this->ScalarOpacityEndValueLabel->Delete();
    this->ScalarOpacityEndValueLabel = NULL;    
    }

  if ( this->ScalarOpacityEndValueScale )
    {
    this->ScalarOpacityEndValueScale->Delete();
    this->ScalarOpacityEndValueScale = NULL;    
    }

  if ( this->ScalarOpacityUnitDistanceLabel )
    {
    this->ScalarOpacityUnitDistanceLabel->Delete();
    this->ScalarOpacityUnitDistanceLabel = NULL;
    }
  
  if ( this->ScalarOpacityUnitDistanceScale )
    {
    this->ScalarOpacityUnitDistanceScale->Delete();
    this->ScalarOpacityUnitDistanceScale = NULL;
    }
  
  if ( this->ColorRampLabel )
    {
    this->ColorRampLabel->Delete();
    this->ColorRampLabel = NULL;
    }
  
  if ( this->ColorRampRange )
    {
    this->ColorRampRange->Delete();
    this->ColorRampRange = NULL;
    }
  
  if ( this->ColorEditorFrame )
    {
    this->ColorEditorFrame->Delete();
    this->ColorEditorFrame = NULL;
    }

  if ( this->ColorStartValueButton )
    {
    this->ColorStartValueButton->Delete();
    this->ColorStartValueButton = NULL;    
    }

  if ( this->ColorEndValueButton )
    {
    this->ColorEndValueButton->Delete();
    this->ColorEndValueButton = NULL;    
    }

  if ( this->ColorMapLabel )
    {
    this->ColorMapLabel->Delete();
    this->ColorMapLabel = NULL;    
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
  
  this->ScalarOpacityFrame = vtkKWLabeledFrame::New();
  this->ScalarOpacityFrame->SetParent(this);
  this->ScalarOpacityFrame->ShowHideFrameOn();
  this->ScalarOpacityFrame->Create(this->GetApplication(), "");
  this->ScalarOpacityFrame->SetLabel("Volume Scalar Opacity");

  this->ColorFrame = vtkKWLabeledFrame::New();
  this->ColorFrame->SetParent(this);
  this->ColorFrame->ShowHideFrameOn();
  this->ColorFrame->Create(this->GetApplication(), "");
  this->ColorFrame->SetLabel("Volume Scalar Color");

  this->ScalarOpacityRampLabel = vtkKWLabel::New();
  this->ScalarOpacityRampLabel->SetParent( this->ScalarOpacityFrame->GetFrame() );
  this->ScalarOpacityRampLabel->Create( this->GetApplication(), "-anchor w");
  this->ScalarOpacityRampLabel->SetLabel("Scalar Opacity Ramp:");
  this->ScalarOpacityRampLabel->SetBalloonHelpString(
    "Set the range for the scalar opacity ramp used to display the volume rendered data." );
  
  this->ScalarOpacityRampRange = vtkKWRange::New();
  this->ScalarOpacityRampRange->SetParent(this->ScalarOpacityFrame->GetFrame());
  this->ScalarOpacityRampRange->ShowEntriesOn();
  this->ScalarOpacityRampRange->Create(this->GetApplication());
  this->ScalarOpacityRampRange->SetEndCommand( this, "ScalarOpacityRampChanged" );
  
  this->ScalarOpacityStartValueLabel = vtkKWLabel::New();
  this->ScalarOpacityStartValueLabel->SetParent( this->ScalarOpacityFrame->GetFrame() );
  this->ScalarOpacityStartValueLabel->Create( this->GetApplication(), "-anchor w");
  this->ScalarOpacityStartValueLabel->SetLabel("Ramp Start Opacity:");  
  
  this->ScalarOpacityStartValueScale = vtkKWScale::New();
  this->ScalarOpacityStartValueScale->SetParent( this->ScalarOpacityFrame->GetFrame() );
  this->ScalarOpacityStartValueScale->Create( this->GetApplication(), "" );
  this->ScalarOpacityStartValueScale->SetRange( 0.0, 1.0 );
  this->ScalarOpacityStartValueScale->SetResolution( 0.01 );
  this->ScalarOpacityStartValueScale->DisplayEntry();
  this->ScalarOpacityStartValueScale->DisplayEntryAndLabelOnTopOff();
  this->ScalarOpacityStartValueScale->SetEndCommand( this, "ScalarOpacityRampChanged" );
  this->ScalarOpacityStartValueScale->SetEntryCommand( this, "ScalarOpacityRampChanged" );
  
  this->ScalarOpacityEndValueLabel = vtkKWLabel::New();
  this->ScalarOpacityEndValueLabel->SetParent( this->ScalarOpacityFrame->GetFrame() );
  this->ScalarOpacityEndValueLabel->Create( this->GetApplication(), "-anchor w" );
  this->ScalarOpacityEndValueLabel->SetLabel("Ramp End Opacity:");  

  this->ScalarOpacityEndValueScale = vtkKWScale::New();
  this->ScalarOpacityEndValueScale->SetParent( this->ScalarOpacityFrame->GetFrame() );
  this->ScalarOpacityEndValueScale->Create( this->GetApplication(), "" );
  this->ScalarOpacityEndValueScale->SetRange( 0.0, 1.0 );
  this->ScalarOpacityEndValueScale->SetResolution( 0.01 );
  this->ScalarOpacityEndValueScale->DisplayEntry();
  this->ScalarOpacityEndValueScale->DisplayEntryAndLabelOnTopOff();
  this->ScalarOpacityEndValueScale->SetEndCommand( this, "ScalarOpacityRampChanged" );
  this->ScalarOpacityEndValueScale->SetEntryCommand( this, "ScalarOpacityRampChanged" );

  this->ScalarOpacityUnitDistanceLabel = vtkKWLabel::New();
  this->ScalarOpacityUnitDistanceLabel->SetParent( this->ScalarOpacityFrame->GetFrame() );
  this->ScalarOpacityUnitDistanceLabel->Create( this->GetApplication(), "-anchor w" );
  this->ScalarOpacityUnitDistanceLabel->SetLabel("Unit Distance:");
  
  this->ScalarOpacityUnitDistanceScale = vtkKWScale::New();
  this->ScalarOpacityUnitDistanceScale->SetParent( this->ScalarOpacityFrame->GetFrame() );
  this->ScalarOpacityUnitDistanceScale->Create( this->GetApplication(), "" );
  this->ScalarOpacityUnitDistanceScale->SetRange( 1.0, 10.0 );
  this->ScalarOpacityUnitDistanceScale->SetResolution( 0.1 );
  this->ScalarOpacityUnitDistanceScale->DisplayEntry();
  this->ScalarOpacityUnitDistanceScale->DisplayEntryAndLabelOnTopOff();
  this->ScalarOpacityUnitDistanceScale->SetEndCommand(this,"ScalarOpacityUnitDistanceChanged");
  this->ScalarOpacityUnitDistanceScale->SetEntryCommand(this,"ScalarOpacityUnitDistanceChanged");
  this->ScalarOpacityUnitDistanceScale->SetBalloonHelpString(
    "Set the unit distance on which the scalar opacity transfer function "
    "is defined.");  
    
  this->Script("grid %s %s -sticky nsew -padx 2 -pady 2", 
               this->ScalarOpacityRampLabel->GetWidgetName(),  
               this->ScalarOpacityRampRange->GetWidgetName() );

  this->Script("grid %s %s -sticky nsew -padx 2 -pady 2", 
               this->ScalarOpacityStartValueLabel->GetWidgetName(),
               this->ScalarOpacityStartValueScale->GetWidgetName() );
  
  this->Script("grid %s %s -sticky nsew -padx 2 -pady 2", 
               this->ScalarOpacityEndValueLabel->GetWidgetName(),
               this->ScalarOpacityEndValueScale->GetWidgetName() );

  this->Script("grid %s %s -sticky nsew -padx 2 -pady 2", 
               this->ScalarOpacityUnitDistanceLabel->GetWidgetName(),
               this->ScalarOpacityUnitDistanceScale->GetWidgetName() );

  this->Script("grid columnconfigure %s 1 -weight 1",
               this->ScalarOpacityRampLabel->GetParent()->GetWidgetName());

  this->ColorRampLabel = vtkKWLabel::New();
  this->ColorRampLabel->SetParent( this->ColorFrame->GetFrame() );
  this->ColorRampLabel->Create( this->GetApplication(), "-anchor w" );
  this->ColorRampLabel->SetLabel("Color Ramp:");
  this->ColorRampLabel->SetBalloonHelpString(
    "Set the range for the color ramp used to display the volume rendered data." );
  
  this->ColorRampRange = vtkKWRange::New();
  this->ColorRampRange->SetParent(this->ColorFrame->GetFrame());
  this->ColorRampRange->ShowEntriesOn();
  this->ColorRampRange->Create(this->GetApplication());
  this->ColorRampRange->SetEndCommand( this, "ColorRampChanged" );

  this->ColorEditorFrame = vtkKWWidget::New();
  this->ColorEditorFrame->SetParent(this->ColorFrame->GetFrame());
  this->ColorEditorFrame->Create(this->GetApplication(), "frame", "");

  this->ColorStartValueButton = vtkKWChangeColorButton::New();
  this->ColorStartValueButton->SetParent(this->ColorEditorFrame);
  this->ColorStartValueButton->ShowLabelOff();
  this->ColorStartValueButton->Create(this->GetApplication(), "");
  this->ColorStartValueButton->SetColor(1.0, 0.0, 0.0);
  this->ColorStartValueButton->SetCommand(this, "ColorButtonCallback");
  this->ColorStartValueButton->SetBalloonHelpString("Select the minimum color.");

  this->ColorMapLabel = vtkKWLabel::New();
  this->ColorMapLabel->SetParent(this->ColorEditorFrame);
  this->ColorMapLabel->Create(this->GetApplication(), 
                    "-relief flat -bd 0 -highlightthickness 0 -padx 0 -pady 0");
  this->Script("bind %s <Configure> {%s ColorMapLabelConfigureCallback %s}", 
               this->ColorMapLabel->GetWidgetName(), 
               this->GetTclName(), "%w %h");

  this->ColorEndValueButton = vtkKWChangeColorButton::New();
  this->ColorEndValueButton->SetParent(this->ColorEditorFrame);
  this->ColorEndValueButton->ShowLabelOff();
  this->ColorEndValueButton->Create(this->GetApplication(), "");
  this->ColorEndValueButton->SetColor(0.0, 0.0, 1.0);
  this->ColorEndValueButton->SetCommand(this, "ColorButtonCallback");
  this->ColorEndValueButton->SetBalloonHelpString("Select the maximum color.");

  this->Script("grid %s %s %s -sticky news -padx 1 -pady 2",
               this->ColorStartValueButton->GetWidgetName(),
               this->ColorMapLabel->GetWidgetName(),
               this->ColorEndValueButton->GetWidgetName());
  
  this->Script("grid columnconfigure %s 1 -weight 1",
               this->ColorMapLabel->GetParent()->GetWidgetName());

  this->Script("grid %s %s -sticky nsew -padx 2 -pady 2", 
               this->ColorRampLabel->GetWidgetName(),  
               this->ColorRampRange->GetWidgetName() );

  this->Script("grid %s -columnspan 2 -sticky nsew -padx 2 -pady 2", 
               this->ColorEditorFrame->GetWidgetName());
  
  this->Script("grid columnconfigure %s 1 -weight 1",
               this->ColorRampLabel->GetParent()->GetWidgetName());
  
  // Back button
  this->BackButton = vtkKWPushButton::New();
  this->BackButton->SetParent(this);
  this->BackButton->Create(this->GetApplication(), "-text {Back}");
  this->BackButton->SetCommand(this, "BackButtonCallback");

  this->Script("pack %s %s %s -side top -anchor n -fill x -padx 2 -pady 2", 
               this->ScalarOpacityFrame->GetWidgetName(),
               this->ColorFrame->GetWidgetName(),
               this->BackButton->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::ColorMapLabelConfigureCallback(int width, int height)
{
  this->UpdateMap(width, height);
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::UpdateMap(int width, int height)
{
  int size;
  int i, j;
  double val, step;
  double *rgba;  
  unsigned char *ptr;  

  size = width*height;
  if (this->MapDataSize < size)
    {
    if (this->MapData)
      {
      delete [] this->MapData;
      }
    this->MapData = new unsigned char[size*3];
    this->MapDataSize = size;
    }
  this->MapWidth = width;
  this->MapHeight = height;

  vtkPVApplication *pvApp = NULL;
  
  if ( this->GetApplication() )
    {
    pvApp =
      vtkPVApplication::SafeDownCast(this->GetApplication());    
    }
  
  if ( !this->PVSource || !this->ArrayInfo || !pvApp ||
       this->PVSource->GetNumberOfParts() == 0 )
    {
    return;
    }
  
  vtkPVPart *part = this->PVSource->GetPart(0);

  vtkColorTransferFunction *colorFunc = 
      vtkColorTransferFunction::SafeDownCast(
        pvApp->GetProcessModule()->
        GetObjectFromID(part->GetPartDisplay()->
                        GetVolumeColorID()));

  step = (this->ScalarRange[1]-this->ScalarRange[0])/(double)(width);
  ptr = this->MapData;
  for (j = 0; j < height; ++j)
    {
    for (i = 0; i < width; ++i)
      {
      val = this->ScalarRange[0] + ((double)(i)*step);
      rgba = colorFunc->GetColor(val);
      
      ptr[0] = static_cast<unsigned char>(rgba[0]*255.0 + 0.5);
      ptr[1] = static_cast<unsigned char>(rgba[1]*255.0 + 0.5);
      ptr[2] = static_cast<unsigned char>(rgba[2]*255.0 + 0.5);
      ptr += 3;
      }
    }

  if (size > 0)
    {
    this->ColorMapLabel->SetImageOption(this->MapData, width, height, 3);
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
  this->PVRenderView->GetPVWindow()->GetCurrentPVSource()->GetPVOutput()->UpdateProperties();
  this->PVRenderView->GetPVWindow()->ShowCurrentSourceProperties();
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
    // as a side effect, reset the GUI for the current data
    arrayInfo->GetComponentRange(0, this->ScalarRange);
  
    this->ScalarOpacityRampRange->SetWholeRange( this->ScalarRange[0], 
                                                 this->ScalarRange[1] );
    this->ColorRampRange->SetWholeRange( this->ScalarRange[0], 
                                                 this->ScalarRange[1] );
      
    vtkPVPart *part = this->PVSource->GetPart(0);

    vtkPiecewiseFunction *opacityFunc = 
      vtkPiecewiseFunction::SafeDownCast(
        pvApp->GetProcessModule()->
        GetObjectFromID(part->GetPartDisplay()->
                        GetVolumeOpacityID()));

    vtkColorTransferFunction *colorFunc = 
      vtkColorTransferFunction::SafeDownCast(
        pvApp->GetProcessModule()->
        GetObjectFromID(part->GetPartDisplay()->
                        GetVolumeColorID()));

    int size = opacityFunc->GetSize();
    
    if ( size != 2 )
      {
      vtkErrorMacro("Expecting 2 points in opacity function!");
      return;
      }
    
    double *ptr = opacityFunc->GetDataPointer();
    this->ScalarOpacityRampRange->SetResolution( (ptr[2]-ptr[0])/10000.0 );
    this->ScalarOpacityRampRange->SetRange(ptr[0], ptr[2]);
    this->ScalarOpacityStartValueScale->SetValue( ptr[1] );
    this->ScalarOpacityEndValueScale->SetValue( ptr[3] );    
    
    size = colorFunc->GetSize();
    if ( size != 2 )
      {
      vtkErrorMacro("Expecting 2 points in color function!");
      return;
      }
    ptr = colorFunc->GetDataPointer();
    
    this->ColorRampRange->SetResolution( (ptr[4]-ptr[0])/10000.0 );
    this->ColorRampRange->SetRange(ptr[0], ptr[4]);
    
    this->ColorStartValueButton->SetColor( ptr[1], ptr[2], ptr[3] );
    this->ColorEndValueButton->SetColor(   ptr[5], ptr[6], ptr[7] );
    
    double bounds[6];
    dataInfo->GetBounds(bounds);
    
    double diameter = 
      sqrt( (bounds[1] - bounds[0]) * (bounds[1] - bounds[0]) +
            (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) +
            (bounds[5] - bounds[4]) * (bounds[5] - bounds[4]) );
    
    int numCells = dataInfo->GetNumberOfCells();
    double linearNumCells = pow( (double) numCells, 1.0/3.0 );
    
    this->ScalarOpacityUnitDistanceScale->SetResolution( diameter / (linearNumCells * 10.0) );
    this->ScalarOpacityUnitDistanceScale->SetRange( diameter / (linearNumCells * 10.0),
                                                    diameter / (linearNumCells / 10.0) );

    vtkVolumeProperty *volumeProperty = 
      vtkVolumeProperty::SafeDownCast(
        pvApp->GetProcessModule()->
        GetObjectFromID(part->GetPartDisplay()->
                        GetVolumePropertyID()));
    
    double unitDistance = volumeProperty->GetScalarOpacityUnitDistance();
    this->ScalarOpacityUnitDistanceScale->SetValue( unitDistance );
    }
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::ScalarOpacityRampChanged()
{
  this->ScalarOpacityRampChangedInternal();
  this->AddTraceEntry("$kw(%s) ScalarOpacityRampChanged", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::ScalarOpacityRampChangedInternal()
{
  vtkPVApplication *pvApp = NULL;
  
  if ( this->GetApplication() )
    {
    pvApp =
      vtkPVApplication::SafeDownCast(this->GetApplication());    
    }
  
  if ( this->PVSource && this->ArrayInfo && pvApp )
    {
    double range[2];
    this->ScalarOpacityRampRange->GetRange(range);
    
    double startValue = this->ScalarOpacityStartValueScale->GetValue();
    double endValue = this->ScalarOpacityEndValueScale->GetValue();

    int numParts = this->PVSource->GetNumberOfParts();
    int i;
    vtkPVPart *part;
    
    for (i = 0; i < numParts; i++)
      {
      part =  this->PVSource->GetPart(i);
      vtkClientServerID volumeOpacityID;
      
      volumeOpacityID = part->GetPartDisplay()->GetVolumeOpacityID();
      
      vtkPVProcessModule* pm = pvApp->GetProcessModule();
      vtkClientServerStream& stream = pm->GetStream();
      
      stream << vtkClientServerStream::Invoke << volumeOpacityID 
             << "RemoveAllPoints" << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke << volumeOpacityID 
             << "AddPoint" << range[0] << startValue << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke << volumeOpacityID 
             << "AddPoint" << range[1] << endValue << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    this->RenderView();
    }
}


//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::ScalarOpacityUnitDistanceChanged()
{
  this->ScalarOpacityUnitDistanceChangedInternal();
  this->AddTraceEntry("$kw(%s) ScalarOpacityUnitDistanceChanged", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::ScalarOpacityUnitDistanceChangedInternal()
{
  vtkPVApplication *pvApp = NULL;
  
  if ( this->GetApplication() )
    {
    pvApp =
      vtkPVApplication::SafeDownCast(this->GetApplication());    
    }
  
  if ( this->PVSource && this->ArrayInfo && pvApp )
    {
    double unitDistance = this->ScalarOpacityUnitDistanceScale->GetValue();

    int numParts = this->PVSource->GetNumberOfParts();
    int i;
    vtkPVPart *part;
    
    for (i = 0; i < numParts; i++)
      {
      part =  this->PVSource->GetPart(i);
      vtkClientServerID volumePropertyID;
      
      volumePropertyID = part->GetPartDisplay()->GetVolumePropertyID();
      
      vtkPVProcessModule* pm = pvApp->GetProcessModule();
      vtkClientServerStream& stream = pm->GetStream();
      
      stream << vtkClientServerStream::Invoke << volumePropertyID 
             << "SetScalarOpacityUnitDistance" << unitDistance << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    this->RenderView();
    }
}


//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::ColorButtonCallback( float vtkNotUsed(r), 
                                                       float vtkNotUsed(g), 
                                                       float vtkNotUsed(b) )
{
  this->ColorRampChanged();
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::ColorRampChanged()
{
  this->ColorRampChangedInternal();
  this->AddTraceEntry("$kw(%s) ColorRampChanged", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::ColorRampChangedInternal()
{
  vtkPVApplication *pvApp = NULL;
  
  if ( this->GetApplication() )
    {
    pvApp =
      vtkPVApplication::SafeDownCast(this->GetApplication());    
    }
  
  if ( this->PVSource && this->ArrayInfo && pvApp )
    {
    double range[2];
    this->ColorRampRange->GetRange(range);
    
    double *startColor = this->ColorStartValueButton->GetColor();
    double *endColor = this->ColorEndValueButton->GetColor();

    int numParts = this->PVSource->GetNumberOfParts();
    int i;
    vtkPVPart *part;
    
    for (i = 0; i < numParts; i++)
      {
      part =  this->PVSource->GetPart(i);
      vtkClientServerID volumeColorID;
      
      volumeColorID = part->GetPartDisplay()->GetVolumeColorID();
      
      vtkPVProcessModule* pm = pvApp->GetProcessModule();
      vtkClientServerStream& stream = pm->GetStream();
      
      stream << vtkClientServerStream::Invoke << volumeColorID 
             << "RemoveAllPoints" << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke << volumeColorID 
             << "AddRGBPoint" << range[0] 
             << startColor[0] << startColor[1] << startColor[2] 
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke << volumeColorID 
             << "AddRGBPoint" << range[1] 
             << endColor[0] << endColor[1] << endColor[2] 
             << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
      }
    this->RenderView();
    }
  
  if (this->MapWidth > 0 && this->MapHeight > 0)
    {
    this->UpdateMap(this->MapWidth, this->MapHeight);
    }
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->BackButton);
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

