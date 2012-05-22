/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPrismView
// .SECTION Description
// Derived from vtkPVRenderView to overload Update() function. This is
// so common scaling can be computed between PrismRepresentations.
//
#ifndef __vtkPrismView_h
#define __vtkPrismView_h

#include "vtkPVRenderView.h"
#include "vtkSmartPointer.h"
#include "vtkBoundingBox.h"

class vtkTransform;
class vtkInformationDoubleVectorKey;
class vtkInformationIntegerVectorKey;
class vtkInformationIntegerKey;

class VTK_EXPORT vtkPrismView : public vtkPVRenderView
{
  //*****************************************************************
public:
  static vtkPrismView* New();
  vtkTypeMacro(vtkPrismView, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  enum ScaleMode
    {
    FullBounds=0,
    ThresholdBounds,
    CustomBounds
    };
  //ETX

  // Description:
  // Adds the representation to the view.
  void AddRepresentation(vtkDataRepresentation* rep);

  // Description:
  // Removes the representation from the view.
  void RemoveRepresentation(vtkDataRepresentation* rep);

  // Description:
  // Bounds of the full geometry that can be used to determine the proper
  // scaling factor for the view
  static vtkInformationDoubleVectorKey* PRISM_GEOMETRY_BOUNDS();

  // Description:
  // Bounds of the thresholded geometry that can be used to determine the proper
  // scaling factor for the view
  static vtkInformationDoubleVectorKey* PRISM_THRESHOLD_BOUNDS();

  // Description:
  // Describes if the bounds of the geoemtery have been log scaled or not
  // this is needed as we always want to display in non log scaled space but
  // actually scale in log space
  static vtkInformationIntegerVectorKey* PRISM_USE_LOG_SCALING();

  // Description:
  // Describes the SESAME table that the prism view is currently
  // working on. This is needed to use the correct type of log scaling
  static vtkInformationIntegerKey* PRISM_TABLE_ID();

  // Description:
  // Overridden to use the meta-data obtained from vtkPrismRepresentation as
  // appropriate.
  virtual void Update();

  // Description:
  // Set / Get the world scale mode for each axis.
  //Note values will be between 0 and 2.
  //See ScaleMode enum for what each one means
  vtkSetVector3Macro(WorldScaleMode,int);
  vtkGetVector3Macro(WorldScaleMode,int);

  // Description:
  // Set / Get the custom bounds for each axes
  // These values are only used if the WorldScaleMode for that axes is set
  // too ScaleMode::CustomBounds
  vtkSetVector6Macro(CustomWorldBounds,double);
  vtkGetVector6Macro(CustomWorldBounds,double);

  // Description:
  // Get the current world scale mode for each axis.
  vtkGetVector6Macro(FullWorldBounds,double);

  // Description:
  // Get the current threshold scale mode for each axis.
  vtkGetVector6Macro(ThresholdWorldBounds,double);


//BTX
protected:
  vtkPrismView();
  ~vtkPrismView();

  bool UpdateWorldScale( );

  vtkTransform *Transform;

  int WorldScaleMode[3];
  double CustomWorldBounds[6];
  double FullWorldBounds[6];
  double ThresholdWorldBounds[6];

  int LogScalingEnabled[3];
  int PrismTableId;

  class vtkPrismViewLogScale;
  class vtkPrismViewTableId;

private:
  vtkPrismView(const vtkPrismView&); // Not implemented
  void operator=(const vtkPrismView&); // Not implemented

//ETX


};

#endif
