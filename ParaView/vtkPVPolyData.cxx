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
#include "vtkPVGetRemoteGhostCells.h"
#include "vtkPVGlyph3D.h"
#include "vtkPVParallelDecimate.h"
#include "vtkKWView.h"
#include "vtkKWEventNotifier.h"

#include "vtkKWScale.h"
#include "vtkKWPushButton.h"
#include "vtkKWEntry.h"
#include "vtkPVWindow.h"
#include "vtkPVAssignment.h"

#include "vtkPVApplication.h"
#include "vtkPVMenuButton.h"
#include "vtkDataSetMapper.h"
#include "vtkParallelDecimate.h"
#include "vtkPVActorComposite.h"

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
vtkPVPolyData::~vtkPVPolyData()
{
  this->DecimateButton->Delete();
  this->DecimateButton = NULL;
  
  this->SetLocalRepresentation(NULL);
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
  f->SetInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
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
  f->SetInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
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
  f->SetInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
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
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVGlyph3D *glyph;
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  
  glyph = vtkPVGlyph3D::New();
  glyph->Clone(pvApp);
  
  glyph->SetInput(this);
  glyph->SetName("glyph");
 
  this->GetPVSource()->GetView()->AddComposite(glyph);
  
  window->SetCurrentSource(glyph);

  glyph->Delete();
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
  f->SetInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
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
  f->SetInput(this);
 
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
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
  f->SetInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
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
  f->SetInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
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
  f->SetInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
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
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVGetRemoteGhostCells *rgc;
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  
  rgc = vtkPVGetRemoteGhostCells::New();
  rgc->Clone(pvApp);
  
  rgc->SetInput(this);
  
  rgc->SetName("get ghost cells");
  
  this->GetPVSource()->GetView()->AddComposite(rgc);
  
  window->SetCurrentSource(rgc);

  rgc->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPolyData::ParallelDecimate()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVParallelDecimate *paraDeci;
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  
  paraDeci = vtkPVParallelDecimate::New();
  paraDeci->Clone(pvApp);
  
  paraDeci->SetInput(this);
  
  paraDeci->SetName("parallel decimate");
  
  this->GetPVSource()->GetView()->AddComposite(paraDeci);
  
  window->SetCurrentSource(paraDeci);
  
  paraDeci->Delete();
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
  this->FiltersMenuButton->AddCommand("ParallelDecimate", this,
				      "ParallelDecimate");
  
  this->DecimateButton->SetParent(this);
  this->DecimateButton->Create(this->Application, "-text Decimate");
  this->DecimateButton->SetCommand(this, "Decimate");
  this->Script("pack %s", this->DecimateButton->GetWidgetName());
  
  return 1;
}


//----------------------------------------------------------------------------
void vtkPVPolyData::SetData(vtkDataSet *data)
{
  vtkPolyData *polyData = vtkPolyData::SafeDownCast(data);
  
  if (data != NULL && polyData == NULL)
    {
    vtkErrorMacro("Expecting a polydata object");
    return;
    }
  
  this->vtkPVData::SetData(polyData);

  if (this->ActorComposite)
    {
    this->ActorComposite->SetApplication(this->Application);
    this->ActorComposite->SetInput(polyData);
    }
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
    pvApp->BroadcastScript("%s SetLocalRepresentation %s",
			   this->GetTclName(), data->GetTclName());
    
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
  
  if (myId == 0)
    {
    pvApp->BroadcastScript("%s InteractiveOnCallback", this->GetTclName());
    }
  
  if (this->LocalRepresentation)
    {
    this->GetActorComposite()->GetMapper()->
      SetInput(this->LocalRepresentation->GetPolyData());
    }
}

//----------------------------------------------------------------------------
void vtkPVPolyData::InteractiveOffCallback()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int myId = pvApp->GetController()->GetLocalProcessId();
  
  if (myId == 0)
    {
    pvApp->BroadcastScript("%s InteractiveOffCallback", this->GetTclName());
    }
  
  this->GetActorComposite()->GetMapper()->SetInput(this->GetPolyData());
}

//----------------------------------------------------------------------------
vtkPolyData *vtkPVPolyData::GetPolyData()
{
  return (vtkPolyData*)this->Data;
}

