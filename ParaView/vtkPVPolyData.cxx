/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPolyData.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#include "vtkPVData.h"
#include "vtkPVPolyData.h"
#include "vtkPVGlyph3D.h"
#include "vtkKWView.h"
#include "vtkKWEventNotifier.h"

#include "vtkKWScale.h"
#include "vtkKWPushButton.h"
#include "vtkKWEntry.h"
#include "vtkPVWindow.h"

#include "vtkPVApplication.h"
#include "vtkPVActorComposite.h"
#include "vtkKWMenuButton.h"
#include "vtkDataSetMapper.h"

#include "vtkPVPolyDataToPolyDataFilter.h"

int vtkPVPolyDataCommand(ClientData cd, Tcl_Interp *interp,
		                     int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVPolyData::vtkPVPolyData()
{
  this->CommandFunction = vtkPVPolyDataCommand;
  
  this->DecimateButton = vtkKWCheckButton::New();
  this->LocalRepresentation = NULL;
}


//----------------------------------------------------------------------------
void vtkPVPolyData::CreateParallelTclObjects(vtkPVApplication *pvApp)
{
  static int instanceCount = 0;
  char tclName[100];
  
  // The output numbers may not match the source numbers but so what.
  ++instanceCount;
  // Make a tcl object.
  sprintf(tclName, "PolyData%d", instanceCount);
  this->VTKData = (vtkPolyData*)pvApp->MakeTclObject("vtkPolyData", tclName);
  this->SetVTKDataTclName(tclName);
  
  // Create and hook up the ActorComposite.
  this->vtkPVData::CreateParallelTclObjects(pvApp);
}


//----------------------------------------------------------------------------
vtkPVPolyData::~vtkPVPolyData()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  this->DecimateButton->Delete();
  this->DecimateButton = NULL;
  
  this->SetLocalRepresentation(NULL);
  
  // Even though this ivar is in the superclass:  I created it, so I will delete it.
  pvApp->BroadcastScript("%s Delete", this->GetVTKDataTclName());
}

//----------------------------------------------------------------------------
vtkPVPolyData* vtkPVPolyData::New()
{
  return new vtkPVPolyData();
}

//----------------------------------------------------------------------------
void vtkPVPolyData::Shrink()
{
  static int instanceCount = 0;
  vtkPVPolyDataToPolyDataFilter *f;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetPVSource()->GetWindow();

  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  f = vtkPVPolyDataToPolyDataFilter::SafeDownCast(
          pvApp->MakePVSource("vtkPVPolyDataToPolyDataFilter",
                              "vtkShrinkPolyData", "Shrink", ++instanceCount));
  if (f == NULL) {return;}
  f->SetPVInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
  f->AddPVInputList();
  f->AddScale("Shrink Factor:", "SetShrinkFactor", "GetShrinkFactor",
		  0.0, 1.0, 0.01);
  f->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //f->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPolyData::TubeFilter()
{
  static int instanceCount = 0;
  vtkPVPolyDataToPolyDataFilter *f;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetPVSource()->GetWindow();

  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  f = vtkPVPolyDataToPolyDataFilter::SafeDownCast(
          pvApp->MakePVSource("vtkPVPolyDataToPolyDataFilter",
                              "vtkTubeFilter", "Tuber", ++instanceCount));
  if (f == NULL) {return;}
  f->SetPVInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
  f->AddPVInputList();
  f->AddLabeledEntry("Radius:", "SetRadius", "GetRadius");
  f->AddLabeledEntry("Number of Sides:", "SetNumberOfSides", "GetNumberOfSides");
  f->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //f->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPolyData::PolyDataNormals()
{
  static int instanceCount = 0;
  vtkPVPolyDataToPolyDataFilter *f;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetPVSource()->GetWindow();

  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  f = vtkPVPolyDataToPolyDataFilter::SafeDownCast(
          pvApp->MakePVSource("vtkPVPolyDataToPolyDataFilter",
                              "vtkPolyDataNormals", "Normals", ++instanceCount));
  if (f == NULL) {return;}
  f->SetPVInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
  f->AddPVInputList();
  f->AddLabeledEntry("FeatureAngle:", "SetFeatureAngle", "GetFeatureAngle");
  f->AddLabeledToggle("Splitting:", "SetSplitting", "GetSplitting");
  f->AddLabeledToggle("Consistency:", "SetConsistency", "GetConsistency");
  f->AddLabeledToggle("ComputePointNormals:", "SetComputePointNormals", "GetComputePointNormals");
  f->AddLabeledToggle("ComputeCellNormals:", "SetComputeCellNormals", "GetComputeCellNormals");
  f->AddLabeledToggle("FlipNormals:", "SetFlipNormals", "GetFlipNormals");
  f->AddLabeledToggle("NonManifoldTraversal:", "SetNonManifoldTraversal", "GetNonManifoldTraversal");
  f->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //f->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPolyData::Glyph()
{
  static int instanceCount = 0;
  vtkPVGlyph3D *f;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetPVSource()->GetWindow();

  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  f = vtkPVGlyph3D::SafeDownCast(
          pvApp->MakePVSource("vtkPVGlyph3D",
                              "vtkGlyph3D", 
                              "Glyph", ++instanceCount));
  if (f == NULL) {return;}
  f->SetPVInput(this);
 
  this->GetPVSource()->GetView()->AddComposite(f);
  
  window->SetCurrentSource(f);
  f->AddPVInputList();
}

//----------------------------------------------------------------------------
void vtkPVPolyData::LoopSubdivision()
{
  static int instanceCount = 0;
  vtkPVPolyDataToPolyDataFilter *f;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetPVSource()->GetWindow();

  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  f = vtkPVPolyDataToPolyDataFilter::SafeDownCast(
          pvApp->MakePVSource("vtkPVPolyDataToPolyDataFilter",
                              "vtkLoopSubDivisionFilter", 
                              "Subdiv", ++instanceCount));
  if (f == NULL) {return;}
  f->SetPVInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
  f->AddPVInputList();
  f->AddLabeledEntry("NumberOfSubdivisions:", "SetNumberOfSubdivisions", "GetNumberOfSubdivisions");
  f->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //f->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPolyData::Clean()
{
  static int instanceCount = 0;
  vtkPVPolyDataToPolyDataFilter *f;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetPVSource()->GetWindow();

  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  f = vtkPVPolyDataToPolyDataFilter::SafeDownCast(
          pvApp->MakePVSource("vtkPVPolyDataToPolyDataFilter",
                              "vtkCleanPolyData", "Clean", ++instanceCount));
  if (f == NULL) {return;}
  f->SetPVInput(this);
 
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
  f->AddPVInputList();
  f->AddScale("Tolerance:","SetTolerance","GetTolerance",0,1,0.01);
  f->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //f->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPolyData::Triangulate()
{
  static int instanceCount = 0;
  vtkPVPolyDataToPolyDataFilter *f;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetPVSource()->GetWindow();

  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  f = vtkPVPolyDataToPolyDataFilter::SafeDownCast(
          pvApp->MakePVSource("vtkPVPolyDataToPolyDataFilter",
                              "vtkTriangleFilter", 
                              "Triangulate", ++instanceCount));
  if (f == NULL) {return;}
  f->SetPVInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
  f->AddPVInputList();
  f->AddLabeledToggle("Pass Verts:","SetPassVerts","GetPassVerts");
  f->AddLabeledToggle("Pass Lines:","SetPassLines","GetPassLines");
  f->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.  
  //f->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPolyData::Decimate()
{
  static int instanceCount = 0;
  vtkPVPolyDataToPolyDataFilter *f;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetPVSource()->GetWindow();

  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  f = vtkPVPolyDataToPolyDataFilter::SafeDownCast(
          pvApp->MakePVSource("vtkPVPolyDataToPolyDataFilter",
                              "vtkDecimatePro", 
                              "Decimate", ++instanceCount));
  if (f == NULL) {return;}
  f->SetPVInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
  f->AddPVInputList();
  f->AddScale("TargetReduction:","SetTargetReduction","GetTargetReduction",
              0.0,1.0,0.01);
  f->AddLabeledToggle("PreserveTopology:","SetPreserveTopology","GetPreserveTopology");
  f->AddScale("FeatureAngle:","SetFeatureAngle","GetFeatureAngle", 0.0, 180.0, 1.0);
  f->AddLabeledToggle("Splitting:","SetSplitting","GetSplitting");
  f->AddScale("SplitAngle:","SetSplitAngle","GetSplitAngle", 0.0, 180.0, 1.0);
  f->AddLabeledToggle("PreSplit Mesh:","SetPreSplitMesh","GetPreSplitMesh");
  f->AddLabeledToggle("AccumulateError:","SetAccumulateError","GetAccumulateError");
  f->AddLabeledEntry("MaximumError","SetMaximumError","GetMaximumError");
  f->AddLabeledToggle("ErrorIsAbsolute:","SetErrorIsAbsolute","GetErrorIsAbsolute");
  f->AddLabeledEntry("AbsoluteError","SetAbsoluteError","GetAbsoluteError");
  f->AddLabeledToggle("BoundaryVertexDeletion:","SetBoundaryVertexDeletion","GetBoundaryVertexDeletion");
  f->AddScale("Degree:","SetDegree","GetDegree", 25, VTK_CELL_SIZE, 1.0);
  f->AddLabeledEntry("InflectionPointRatio:","SetInflectionPointRatio","GetInflectionPointRatio");
  f->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //f->Delete();
}
//----------------------------------------------------------------------------
void vtkPVPolyData::QuadricClustering()
{
  static int instanceCount = 0;
  vtkPVPolyDataToPolyDataFilter *f;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetPVSource()->GetWindow();

  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  f = vtkPVPolyDataToPolyDataFilter::SafeDownCast(
          pvApp->MakePVSource("vtkPVPolyDataToPolyDataFilter",
                              "vtkQuadricClustering", 
                              "QuadCluster", ++instanceCount));
  if (f == NULL) {return;}
  f->SetPVInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
  f->AddPVInputList();
  f->AddLabeledEntry("XDivisions","SetNumberOfXDivisions","GetNumberOfXDivisions");
  f->AddLabeledEntry("YDivisions","SetNumberOfYDivisions","GetNumberOfYDivisions");
  f->AddLabeledEntry("ZDivisions","SetNumberOfZDivisions","GetNumberOfZDivisions");
  f->AddLabeledToggle("UseInputPoints:","SetUseInputPoints","GetUseInputPoints");
  f->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //f->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPolyData::GetGhostCells()
{

}

//----------------------------------------------------------------------------
void vtkPVPolyData::ParallelDecimate()
{
}

//----------------------------------------------------------------------------
void vtkPVPolyData::PieceScalars()
{
  static int instanceCount = 0;
  vtkPVPolyDataToPolyDataFilter *f;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetPVSource()->GetWindow();

  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  f = vtkPVPolyDataToPolyDataFilter::SafeDownCast(
          pvApp->MakePVSource("vtkPVPolyDataToPolyDataFilter",
                              "vtkPieceScalars", "PieceScalars", 
			      ++instanceCount));
  if (f == NULL) {return;}
  f->SetPVInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
  f->AddPVInputList();
  f->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //f->Delete();
}

//----------------------------------------------------------------------------
int vtkPVPolyData::Create(char *args)
{
  if (this->vtkPVData::Create(args) == 0)
    {
    return 0;
    }
  
  this->FiltersMenuButton->AddCommand("ShrinkPolyData", this,
				      "Shrink");
  this->FiltersMenuButton->AddCommand("Glyph3D", this,
				      "Glyph");
  this->FiltersMenuButton->AddCommand("LoopSubdivision", this,
				      "LoopSubdivision");
  this->FiltersMenuButton->AddCommand("Clean", this, "Clean");
  this->FiltersMenuButton->AddCommand("Triangulate", this, "Triangulate");
  this->FiltersMenuButton->AddCommand("Decimate", this, "Decimate");
  this->FiltersMenuButton->AddCommand("QuadricClustering", this, "QuadricClustering");
  this->FiltersMenuButton->AddCommand("GetRemoteGhostCells", this,
				      "GetGhostCells");
  this->FiltersMenuButton->AddCommand("PolyDataNormals", this,
				      "PolyDataNormals");
  this->FiltersMenuButton->AddCommand("TubeFilter", this,
				      "TubeFilter");
  this->FiltersMenuButton->AddCommand("PieceScalars", this,
				      "PieceScalars");
  
  this->DecimateButton->SetParent(this);
  this->DecimateButton->Create(this->Application, "-text Decimate");
  this->DecimateButton->SetCommand(this, "Decimate");
  this->Script("pack %s", this->DecimateButton->GetWidgetName());
  
  return 1;
}


//----------------------------------------------------------------------------
void vtkPVPolyData::SetLocalRepresentation(vtkPVPolyData *data)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int myId;
  
  if (this->LocalRepresentation == data)
    {
    return;
    }
  
  myId = pvApp->GetController()->GetLocalProcessId();
  
  if (myId == 0)
    {
    //pvApp->BroadcastScript("%s SetLocalRepresentation %s",
    //		   this->GetTclName(), data->GetTclName());
    
    // Manage callbacks.
    if (this->LocalRepresentation && data == NULL)
      {
      this->Application->GetEventNotifier()->
	RemoveCallback("InteractiveRenderStart",
		       this->GetPVSource()->GetWindow(), this,
		       "InteractiveOnCallback" );
      this->Application->GetEventNotifier()->
	RemoveCallback("InteractiveRenderEnd",
		       this->GetPVSource()->GetWindow(),
		       this, "InteractiveOffCallback" );
      }
    if (this->LocalRepresentation == NULL && data != NULL)
      {
      this->Application->GetEventNotifier()->
	AddCallback("InteractiveRenderStart", this->GetPVSource()->GetWindow(),
		    this, "InteractiveOnCallback" );
      this->Application->GetEventNotifier()->
	AddCallback("InteractiveRenderEnd", this->GetPVSource()->GetWindow(), 
		    this, "InteractiveOffCallback" );
      }
    }
  
  if (this->LocalRepresentation)
    {
    this->LocalRepresentation->UnRegister(this);
    this->LocalRepresentation = NULL;
    }
  if (data)
    {
    this->LocalRepresentation = data;
    data->Register(this);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVPolyData::InteractiveOnCallback()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int myId = pvApp->GetController()->GetLocalProcessId();
  
  //pvApp->BroadcastScript("%s InteractiveOnCallback", this->GetTclName());
  
  if (this->LocalRepresentation)
    {
    this->GetActorComposite()->GetMapper()->
      SetInput(this->LocalRepresentation->GetVTKPolyData());
    }
}

//----------------------------------------------------------------------------
void vtkPVPolyData::InteractiveOffCallback()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int myId = pvApp->GetController()->GetLocalProcessId();
  
  //pvApp->BroadcastScript("%s InteractiveOffCallback", this->GetTclName());
  
  this->GetActorComposite()->GetMapper()->SetInput(this->GetVTKPolyData());
}

//----------------------------------------------------------------------------
vtkPolyData *vtkPVPolyData::GetVTKPolyData()
{
  return (vtkPolyData*)this->VTKData;
}

