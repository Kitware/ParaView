/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVActorComposite.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
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
// .NAME vtkPVActorComposite - a composite for actors
// .SECTION Description
// A composite designed for actors. The actor has a vtkPolyDataMapper as
// a mapper, and the user specifies vtkPolyData as the input of this 
// composite.

#ifndef __vtkPVActorComposite_h
#define __vtkPVActorComposite_h

#include "vtkKWActorComposite.h"
#include "vtkKWCheckButton.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWScale.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWMenuButton.h"
#include "vtkKWOptionMenu.h"
#include "vtkPVApplication.h"
#include "vtkDataSetMapper.h"

//class vtkPVImageTextureFilter;
class vtkPVApplication;
class vtkPVData;
class vtkKWCheckButton;
class vtkScalarBarActor;


#define VTK_PV_ACTOR_COMPOSITE_NO_MODE            0
#define VTK_PV_ACTOR_COMPOSITE_DATA_SET_MODE      1
#define VTK_PV_ACTOR_COMPOSITE_POLY_DATA_MODE     2
#define VTK_PV_ACTOR_COMPOSITE_IMAGE_OUTLINE_MODE 3
#define VTK_PV_ACTOR_COMPOSITE_IMAGE_TEXTURE_MODE 4

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
  
  // Description:
  // Parallel methods for computing the scalar range from the input,
  /// and setting the scalar range of the mapper.
  void SetScalarRange(float min, float max);
  
  // Description:
  // Casts to vtkPVApplication.
  vtkPVApplication *GetPVApplication();
  
  vtkGetObjectMacro(Mapper, vtkPolyDataMapper);
  
  // Description:
  // to change the ambient component of the light
  void AmbientChanged();
  void SetAmbient(float ambient);
  
  // Description:
  // Different modes for displaying the input.
  void SetMode(int mode);
  void SetModeToDataSet()
    {this->SetMode(VTK_PV_ACTOR_COMPOSITE_DATA_SET_MODE);}
  void SetModeToPolyData()
    {this->SetMode(VTK_PV_ACTOR_COMPOSITE_POLY_DATA_MODE);}
  void SetModeToImageOutline()
    {this->SetMode(VTK_PV_ACTOR_COMPOSITE_IMAGE_OUTLINE_MODE);}
  void SetModeToImageTexture()
    {this->SetMode(VTK_PV_ACTOR_COMPOSITE_IMAGE_TEXTURE_MODE);}

  // Description:
  // We need our own set input to take any type of data (based on mode).
  //void SetInput(vtkDataSet *input);
  //void SetInput(vtkPolyData *input) {this->SetInput((vtkDataSet*)input);}

  // Description:
  // Tcl name of the actor across all processes.
  vtkGetStringMacro(ActorTclName);
  
  // Description:
  // Methods called when item chosen from ColorMenu
  void ColorByProperty();
  void ColorByPointScalars();
  void ColorByCellScalars();
  void ColorByPointFieldComponent(char *name, int comp);
  void ColorByCellFieldComponent(char *name, int comp);

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
  // Callback for the ResetColorRange button.
  void ResetColorRange();
  
  // Description:
  // Turn tree compositing on/off.
  void SetComposite(int val);
  vtkGetMacro(Composite, int)
  vtkBooleanMacro(Composite, int);
  void CompositeCheckCallback();

  void SetScalarBarVisibility(int val);  
  void ScalarBarCheckCallback();
  void ScalarBarOrientationCallback();
  
  // Description:
  // Temporary intoerface for seting the compositing reduction.
  void ReductionCallback();

  // Description:
  // Save out the mapper and actor to a file.
  void Save(ofstream *file, const char *sourceName);
  
protected:

  vtkPVActorComposite();
  ~vtkPVActorComposite();
  vtkPVActorComposite(const vtkPVActorComposite&) {};
  void operator=(const vtkPVActorComposite&) {};
  
  vtkKWWidget *Properties;
  char *Name;
  vtkKWLabel *NumCellsLabel;
  vtkKWLabel *BoundsLabel;
  vtkKWLabel *XRangeLabel;
  vtkKWLabel *YRangeLabel;
  vtkKWLabel *ZRangeLabel;
  
  vtkKWScale *AmbientScale;
  
  vtkKWLabel *ColorMenuLabel;
  vtkKWOptionMenu *ColorMenu;

  vtkKWLabel *RepresentationMenuLabel;
  vtkKWOptionMenu *RepresentationMenu;

  vtkKWLabel *InterpolationMenuLabel;
  vtkKWOptionMenu *InterpolationMenu;

  vtkKWCheckButton *CompositeCheck;

  vtkKWCheckButton *VisibilityCheck;
  
  // I merged the PVData object and the PVActorComposite.  
  // I do not know what this point is for.  It is probably obsolete.
  // ???The data object that owns this composite???
  vtkPVData *PVData;
  
  //vtkPVImageTextureFilter *TextureFilter;
  
  // An experiment with compositing.
  int Composite;

  // How to convert data set to polydata.
  int Mode;
  // Super class stores a vtkPolyDataInput, this is a more general input.
  vtkDataSet *DataSetInput;

  char *ActorTclName;
  vtkSetStringMacro(ActorTclName);
  
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
  
  vtkKWEntry *ReductionEntry;

  vtkKWCheckButton *ScalarBarCheck;
  vtkScalarBarActor *ScalarBar;
  vtkKWCheckButton *ScalarBarOrientationCheck;
};

#endif
