/*=========================================================================

  Program:   ParaView
  Module:    vtkPVActorComposite.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPVActorComposite - a composite for actors
// .SECTION Description
// A composite designed for actors. The actor has a vtkPolyDataMapper as
// a mapper, and the user specifies vtkPolyData as the input of this 
// composite.

#ifndef __vtkPVActorComposite_h
#define __vtkPVActorComposite_h

#include "vtkKWActorComposite.h"
#include "vtkKWChangeColorButton.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWScale.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWMenuButton.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWLabeledFrame.h"
#include "vtkPVApplication.h"
#include "vtkDataSetMapper.h"

class vtkPVApplication;
class vtkPVRenderView;
class vtkPVData;
class vtkKWCheckButton;
class vtkKWBoundsDisplay;
class vtkScalarBarActor;
class vtkCubeAxesActor2D;


//#define VTK_PV_ACTOR_COMPOSITE_NO_MODE            0
//#define VTK_PV_ACTOR_COMPOSITE_DATA_SET_MODE      1
//#define VTK_PV_ACTOR_COMPOSITE_POLY_DATA_MODE     2
//#define VTK_PV_ACTOR_COMPOSITE_IMAGE_OUTLINE_MODE 3
//#define VTK_PV_ACTOR_COMPOSITE_IMAGE_TEXTURE_MODE 4

class VTK_EXPORT vtkPVActorComposite : public vtkKWActorComposite
{
public:
  static vtkPVActorComposite* New();
  vtkTypeMacro(vtkPVActorComposite, vtkKWActorComposite);

  // Description:
  // Create the properties object, called by UpdateProperties.
  void CreateProperties();
  void UpdateProperties();
  
  // Description:
  // This method should be called immediately after the object is constructed.
  // It create VTK objects which have to exeist on all processes.
  void CreateParallelTclObjects(vtkPVApplication *pvApp);
  
  // Description:
  // This name is used in the data list to identify the composite.
  virtual void SetName(const char *name);
  char* GetName();
  
  void Select(vtkKWView *v);
  void Deselect(vtkKWView *v);
  
  // Description:
  // This method is meant to setup the actor/mapper
  // to best disply it input.  This will involve setting the scalar range,
  // and possibly other properties. 
  void Initialize();

  // Description:
  // This flag turns the visibility of the prop on and off.  These methods transmit
  // the state change to all of the satellite processes.
  void SetVisibility(int v);
  int GetVisibility();
  vtkBooleanMacro(Visibility, int);
  void VisibilityCheckCallback();
  vtkGetObjectMacro(VisibilityCheck, vtkKWCheckButton);
  
  // Description:
  // ONLY SET THIS IF YOU ARE A PVDATA!
  // The actor composite needs to know which PVData it belongs to.
  void SetInput(vtkPVData *data);
  vtkGetObjectMacro(PVData, vtkPVData);
  void SetInput(vtkPolyData* d) {d=d; vtkErrorMacro("HiddenMethod");}
  
  // Description:
  // Parallel methods for computing the scalar range from the input,
  /// and setting the scalar range of the mapper.
  void SetScalarRange(float min, float max);
  
  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();

  virtual void SetApplication(vtkPVApplication *pvApp)
    {this->vtkKWActorComposite::SetApplication(pvApp);}
  virtual void SetApplication(vtkKWApplication *a) 
    {a=a;vtkErrorMacro("Hidden Method");}


  
  vtkGetObjectMacro(Mapper, vtkPolyDataMapper);
  
  // Description:
  // to change the ambient component of the light
  void AmbientChanged();
  void SetAmbient(float ambient);
  
  // This isn't currently being used.
  // Description:
  // Different modes for displaying the input.
//  void SetMode(int mode);
//  void SetModeToDataSet()
//    {this->SetMode(VTK_PV_ACTOR_COMPOSITE_DATA_SET_MODE);}
//  void SetModeToPolyData()
//    {this->SetMode(VTK_PV_ACTOR_COMPOSITE_POLY_DATA_MODE);}
//  void SetModeToImageOutline()
//    {this->SetMode(VTK_PV_ACTOR_COMPOSITE_IMAGE_OUTLINE_MODE);}
//  void SetModeToImageTexture()
//    {this->SetMode(VTK_PV_ACTOR_COMPOSITE_IMAGE_TEXTURE_MODE);}

  // Description:
  // We need our own set input to take any type of data (based on mode).
  //void SetInput(vtkDataSet *input);
  //void SetInput(vtkPolyData *input) {this->SetInput((vtkDataSet*)input);}

  // Description:
  // Tcl name of the actor across all processes.
  vtkGetStringMacro(PropTclName);  
  
  // Description:
  // Methods called when item chosen from RepresentationMenu
  void DrawWireframe();
  void DrawSurface();
  void DrawPoints();
  
  // Description:
  // Methods called when item chosen from RepresentationMenu
  void SetInterpolationToFlat();
  void SetInterpolationToGouraud();
  
  // Description:
  // Get the color range from the mappers on all the processes.
  void GetColorRange(float range[2]);
  
  // Description:
  // Sets the color range of all the mappers (all procs) and updates
  // the user interface as well.
  void SetColorRange(float min, float max);

  // Description:
  // Callback for the ResetColorRange button.
  void ResetColorRange();

  // Set the color range from the entry widgets.
  void ColorRangeEntryCallback();
  
  void SetScalarBarVisibility(int val);  
  void ScalarBarCheckCallback();
  void ScalarBarOrientationCallback();
  void SetScalarBarOrientationToVertical();
  void SetScalarBarOrientationToHorizontal();
  
  void SetCubeAxesVisibility(int val);
  void CubeAxesCheckCallback();

  void CenterCamera();
  
  // Description:
  // Save out the mapper and actor to a file.
  void SaveInTclScript(ofstream *file, const char *sourceName);
  
  // Description:
  // Callback for the change color button.
  void ChangeActorColor(float r, float g, float b);
  
  // Description:
  // Needed to render.
  vtkPVRenderView *GetPVRenderView();

  // Description:
  // Get the name of the scalar bar actor.
  vtkGetStringMacro(ScalarBarTclName);
  
  // Description:
  // Get the name of the cube axes actor.
  vtkGetStringMacro(CubeAxesTclName);

  // Description:
  // Access to pointSize for scripting.
  void SetPointSize(int size);
  void SetLineWidth(int width);
  
  // Description:
  // Callbacks for point size and line width sliders.
  void ChangePointSize();
  void ChangeLineWidth();

  // Description:
  // Access to option menus for scripting.
  vtkGetObjectMacro(ColorMenu, vtkKWOptionMenu);
  vtkGetObjectMacro(ColorMapMenu, vtkKWOptionMenu);

  // Description:
  // Callback methods when item chosen from ColorMenu
  void ColorByProperty();
  void ColorByPointFieldComponent(const char *name, int comp);
  void ColorByCellFieldComponent(const char *name, int comp);

  // Description:
  // Callback for color map menu.
  void ChangeColorMap();

protected:

  vtkPVActorComposite();
  ~vtkPVActorComposite();
  vtkPVActorComposite(const vtkPVActorComposite&) {};
  void operator=(const vtkPVActorComposite&) {};
  
  // Problems with vtkLODActor led me to use these.
  vtkProperty *Property;
  vtkProp *Prop;
  
  // Not properties does not mean the same thing as vtk.
  vtkKWWidget *Properties;
  char *Name;
  vtkKWLabel *NumCellsLabel;
  vtkKWLabel *NumPointsLabel;
  vtkKWBoundsDisplay *BoundsDisplay;
  
  vtkKWScale *AmbientScale;
  
  vtkKWLabeledFrame *ScalarBarFrame;
  vtkKWLabeledFrame *ColorFrame;
  vtkKWLabeledFrame *DisplayStyleFrame;
  vtkKWWidget *StatsFrame;
  vtkKWLabeledFrame *ViewFrame;
  
  vtkKWLabel *ColorMenuLabel;
  vtkKWOptionMenu *ColorMenu;

  vtkKWChangeColorButton *ColorButton;

  vtkKWLabel *ColorMapMenuLabel;
  vtkKWOptionMenu *ColorMapMenu;
  
  vtkKWWidget *RepresentationMenuFrame;
  vtkKWLabel *RepresentationMenuLabel;
  vtkKWOptionMenu *RepresentationMenu;
  vtkKWWidget *InterpolationMenuFrame;
  vtkKWLabel *InterpolationMenuLabel;
  vtkKWOptionMenu *InterpolationMenu;

  vtkKWWidget *DisplayScalesFrame;
  vtkKWLabel *PointSizeLabel;
  vtkKWScale *PointSizeScale;
  vtkKWLabel *LineWidthLabel;
  vtkKWScale *LineWidthScale;
  
  vtkKWCheckButton *VisibilityCheck;
  
  // I merged the PVData object and the PVActorComposite.  
  // I do not know what this point is for.  It is probably obsolete.
  // ???The data object that owns this composite???
  vtkPVData *PVData;
  
// not currently being used  
// How to convert data set to polydata.
//  int Mode;
  // Super class stores a vtkPolyDataInput, this is a more general input.
  vtkDataSet *DataSetInput;

  char *PropTclName;
  vtkSetStringMacro(PropTclName);
  
  char *PropertyTclName;
  vtkSetStringMacro(PropertyTclName);
  
  char *MapperTclName;
  vtkSetStringMacro(MapperTclName);

  char *LODMapperTclName;
  vtkSetStringMacro(LODMapperTclName);
  
  char *LODDeciTclName;
  vtkSetStringMacro(LODDeciTclName);
  
  char *OutlineTclName;
  vtkSetStringMacro(OutlineTclName);
  
  char *GeometryTclName;
  vtkSetStringMacro(GeometryTclName);
  
  char *OutputPortTclName;
  vtkSetStringMacro(OutputPortTclName);
  
  char *AppendPolyDataTclName;
  vtkSetStringMacro(AppendPolyDataTclName);
  
  // Here to create unique names.
  int InstanceCount;

  // If the data changes, we need to change to.
  vtkTimeStamp UpdateTime;
  
  vtkKWWidget *ScalarBarCheckFrame;
  vtkKWCheckButton *ScalarBarCheck;
  vtkKWCheckButton *ScalarBarOrientationCheck;
  char* ScalarBarTclName;
  vtkSetStringMacro(ScalarBarTclName);
  
  // Stuff for setting the range of the color map.
  vtkKWWidget *ColorRangeFrame;
  vtkKWPushButton *ColorRangeResetButton;
  vtkKWLabeledEntry *ColorRangeMinEntry;
  vtkKWLabeledEntry *ColorRangeMaxEntry;

  vtkKWCheckButton *CubeAxesCheck;
  char* CubeAxesTclName;
  vtkSetStringMacro(CubeAxesTclName);

  vtkKWPushButton *ResetCameraButton;

  float PreviousAmbient;
  float PreviousDiffuse;
  float PreviousSpecular;
  int PreviousWasSolid;
};

#endif
