/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGridAxes3DActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGridAxes3DActor
// .SECTION Description

#ifndef vtkGridAxes3DActor_h
#define vtkGridAxes3DActor_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkProp3D.h"

#include "vtkGridAxesHelper.h" //  needed for vtkGridAxesHelper.
#include "vtkNew.h" // needed for vtkNew.
#include "vtkStdString.h" // needed for vtkStdString.

class vtkDoubleArray;
class vtkGridAxes2DActor;
class vtkProperty;
class vtkTextProperty;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkGridAxes3DActor : public vtkProp3D
{
public:
  static vtkGridAxes3DActor* New();
  vtkTypeMacro(vtkGridAxes3DActor, vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Shallow copy from another vtkGridAxes3DActor.
  virtual void ShallowCopy(vtkProp *prop);

  // Description:
  // Set the bounding box defining the grid space. This, together with the
  // \c Face identify which planar surface this class is interested in. This
  // class is designed to work with a single planar surface.
  vtkSetVector6Macro(GridBounds, double);
  vtkGetVector6Macro(GridBounds, double);

  // Description:
  // Values for FaceMask.
  // Developer note: these are deliberately in the same order as
  // vtkGridAxesHelper::Faces which is same order as faces in vtkVoxel.
  enum FaceMasks
    {
    MIN_YZ=0x01,
    MIN_ZX=0x02,
    MIN_XY=0x04,
    MAX_YZ=0x08,
    MAX_ZX=0x010,
    MAX_XY=0x020
    };

  // Description:
  // Set the mask to select faces. The faces rendered can be a subset of the
  // faces selected using the FaceMask based on the BackfaceCulling and
  // FrontfaceCulling flags set on the Property.
  virtual void SetFaceMask(unsigned int mask);
  vtkGetMacro(FaceMask, unsigned int);

  enum LabelMasks
    {
    MIN_X = vtkGridAxesHelper::MIN_X,
    MIN_Y = vtkGridAxesHelper::MIN_Y,
    MIN_Z = vtkGridAxesHelper::MIN_Z,
    MAX_X = vtkGridAxesHelper::MAX_X,
    MAX_Y = vtkGridAxesHelper::MAX_Y,
    MAX_Z = vtkGridAxesHelper::MAX_Z
    };

  // Description:
  // Set the axis to label.
  virtual void SetLabelMask(unsigned int mask);
  unsigned int GetLabelMask();

  // Description:
  // Set to true to only label edges shared with 1 face. Note that
  // if all faces are being rendered, this will generate no labels.
  vtkSetMacro(LabelUniqueEdgesOnly, bool);
  vtkGetMacro(LabelUniqueEdgesOnly, bool);

  // Description:
  // Turn off to not generate polydata for the plane's grid.
  void SetGenerateGrid(bool val);
  bool GetGenerateGrid();
  vtkBooleanMacro(GenerateGrid, bool);

  // Description:
  // Turn off to not generate the polydata for the plane's edges. Which edges
  // are rendered is defined by the EdgeMask.
  void SetGenerateEdges(bool val);
  bool GetGenerateEdges();
  vtkBooleanMacro(GenerateEdges, bool);

  // Description:
  // Turn off to not generate the markers for the tick positions. Which egdes
  // are rendered is defined by the TickMask.
  void SetGenerateTicks(bool val);
  bool GetGenerateTicks();
  vtkBooleanMacro(GenerateTicks, bool);

  // Description:
  // Get/Set the property used to control the appearance of the rendered grid.
  void SetProperty(vtkProperty*);
  vtkProperty* GetProperty();

  //---------------------------------------------------------------------------
  // *** Properties to control the axis titles ***

  // Description:
  // Get/Set the vtkTextProperty for the title for each the axes.
  // Note that the alignment properties are not used.
  void SetTitleTextProperty(int axis, vtkTextProperty*);
  void SetXTitleTextProperty(vtkTextProperty* prop)
    { this->SetTitleTextProperty(0, prop); }
  void SetYTitleTextProperty(vtkTextProperty* prop)
    { this->SetTitleTextProperty(1, prop); }
  void SetZTitleTextProperty(vtkTextProperty* prop)
    { this->SetTitleTextProperty(2, prop); }
  vtkTextProperty* GetTitleTextProperty(int axis);

  // Description:
  // Get/Set the text to use for titles for the axis. Setting the title to an
  // empty string will hide the title label for that axis.
  void SetTitle(int axis, const vtkStdString& title);
  void SetXTitle(const vtkStdString& title) { this->SetTitle(0, title); }
  void SetYTitle(const vtkStdString& title) { this->SetTitle(1, title); }
  void SetZTitle(const vtkStdString& title) { this->SetTitle(2, title); }
  const vtkStdString& GetTitle(int axis);

  // Description:
  // Set whether the specified axis should use custom labels instead of
  // automatically determined ones.
  void SetUseCustomLabels(int axis, bool val);
  void SetXUseCustomLabels(bool val) { this->SetUseCustomLabels(0, val); }
  void SetYUseCustomLabels(bool val) { this->SetUseCustomLabels(1, val); }
  void SetZUseCustomLabels(bool val) { this->SetUseCustomLabels(2, val); }

  // Description:
  void SetNumberOfLabels(int axis, vtkIdType val);
  void SetNumberOfXLabels(vtkIdType val) { this->SetNumberOfLabels(0, val); }
  void SetNumberOfYLabels(vtkIdType val) { this->SetNumberOfLabels(1, val); }
  void SetNumberOfZLabels(vtkIdType val) { this->SetNumberOfLabels(2, val); }

  // Description:
  void SetLabel(int axis, vtkIdType index, double value);
  void SetXLabel(vtkIdType index, double value) { this->SetLabel(0, index, value); }
  void SetYLabel(vtkIdType index, double value) { this->SetLabel(1, index, value); }
  void SetZLabel(vtkIdType index, double value) { this->SetLabel(2, index, value); }

  //---------------------------------------------------------------------------
  // *** Properties to control the axis data labels ***

  // Description:
  // Get/Set the vtkTextProperty that governs how the axis labels are displayed.
  // Note that the alignment properties are not used.
  void SetLabelTextProperty(int axis, vtkTextProperty*);
  void SetXLabelTextProperty(vtkTextProperty* prop)
    { this->SetLabelTextProperty(0, prop); }
  void SetYLabelTextProperty(vtkTextProperty* prop)
    { this->SetLabelTextProperty(1, prop); }
  void SetZLabelTextProperty(vtkTextProperty* prop)
    { this->SetLabelTextProperty(2, prop); }
  vtkTextProperty* GetLabelTextProperty(int axis);

  // Description:
  // Get/set the numerical notation, standard, scientific or mixed (0, 1, 2).
  // Accepted values are vtkAxis::AUTO, vtkAxis::FIXED, vtkAxis::CUSTOM.
  void SetNotation(int axis, int notation);
  void SetXNotation(int notation) { this->SetNotation(0, notation);}
  void SetYNotation(int notation) { this->SetNotation(1, notation);}
  void SetZNotation(int notation) { this->SetNotation(2, notation);}
  int GetNotation(int axis);

  // Description:
  // Get/set the numerical precision to use, default is 2.
  void SetPrecision(int axis, int val);
  void SetXPrecision(int val) { this->SetPrecision(0, val);}
  void SetYPrecision(int val) { this->SetPrecision(1, val);}
  void SetZPrecision(int val) { this->SetPrecision(2, val);}
  int GetPrecision(int axis);

  // Description:
  // Enable/Disable layer support. Default is off. When enabled, the prop can
  // render in there separate layers:
  // \li \c BackgroundLayer for all text labels and titles on the back faces,
  // \li \c GeometryLayer for all 3D geometry e.g the grid wireframe, and
  // \li \c ForegroundLayer for all text labels and titles on the front faces.
  void SetEnableLayerSupport(bool val);
  bool GetEnableLayerSupport();
  vtkBooleanMacro(EnableLayerSupport, bool);

  // Description:
  // Get/Set the layer in which to render all background actors/text when
  // EnableLayerSupport is ON. Default is 0.
  void SetBackgroundLayer(int val);
  int GetBackgroundLayer();

  // Description:
  // Get/Set the layer in which to render all 3D actors when
  // EnableLayerSupport is ON. Default is 0.
  void SetGeometryLayer(int val);
  int GetGeometryLayer();

  // Description:
  // Get/Set the layer in which to render all foreground actors/text when
  // EnableLayerSupport is ON. Default is 0.
  void SetForegroundLayer(int val);
  int GetForegroundLayer();

  //--------------------------------------------------------------------------
  // Methods for vtkProp3D API.
  //--------------------------------------------------------------------------

  // Description:
  // Returns the prop bounds.
  virtual double *GetBounds();

  virtual int RenderOpaqueGeometry(vtkViewport *);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport* viewport);
  virtual int RenderOverlay(vtkViewport* viewport);
  virtual int HasTranslucentPolygonalGeometry();
  virtual void ReleaseGraphicsResources(vtkWindow *);
//BTX
protected:
  vtkGridAxes3DActor();
  ~vtkGridAxes3DActor();

  virtual void Update(vtkViewport* viewport);

  double GridBounds[6];
  unsigned int FaceMask;
  unsigned int LabelMask;
  bool LabelUniqueEdgesOnly;
  vtkTuple<bool, 3> UseCustomLabels;
  vtkTuple<vtkNew<vtkDoubleArray>, 3> CustomLabels;
  unsigned long CustomLabelsMTime;

  vtkTuple<vtkNew<vtkGridAxes2DActor>, 6> GridAxes2DActors;
private:
  vtkGridAxes3DActor(const vtkGridAxes3DActor&); // Not implemented.
  void operator=(const vtkGridAxes3DActor&); // Not implemented.

  unsigned long GetBoundsMTime;
//ETX
};


#endif
