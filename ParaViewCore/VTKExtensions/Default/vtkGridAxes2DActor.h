/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGridAxes2DActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGridAxes2DActor
 *
*/

#ifndef vtkGridAxes2DActor_h
#define vtkGridAxes2DActor_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkProp3D.h"

#include "vtkGridAxesHelper.h"       // needed of Helper
#include "vtkGridAxesPlane2DActor.h" // needed for inline methods
#include "vtkNew.h"                  // needed for vtkNew.
#include "vtkSmartPointer.h"         // needed for vtkSmartPointer.
#include "vtkStdString.h"            // needed for vtkStdString.

class vtkAxis;
class vtkContextScene;
class vtkDoubleArray;
class vtkProperty;
class vtkTextProperty;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkGridAxes2DActor : public vtkProp3D
{
public:
  static vtkGridAxes2DActor* New();
  vtkTypeMacro(vtkGridAxes2DActor, vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Set the bounding box defining the grid space. This, together with the
   * \c Face identify which planar surface this class is interested in. This
   * class is designed to work with a single planar surface.
   */
  vtkSetVector6Macro(GridBounds, double);
  vtkGetVector6Macro(GridBounds, double);
  //@}

  // These are in the same order as the faces of a vtkVoxel.
  enum Faces
  {
    MIN_YZ = vtkGridAxesHelper::MIN_YZ,
    MIN_ZX = vtkGridAxesHelper::MIN_ZX,
    MIN_XY = vtkGridAxesHelper::MIN_XY,
    MAX_YZ = vtkGridAxesHelper::MAX_YZ,
    MAX_ZX = vtkGridAxesHelper::MAX_ZX,
    MAX_XY = vtkGridAxesHelper::MAX_XY
  };

  //@{
  /**
   * Indicate which face of the specified bounds is this class operating with.
   */
  vtkSetClampMacro(Face, int, MIN_YZ, MAX_XY);
  vtkGetMacro(Face, int);
  //@}

  /**
   * Valid values for LabelMask.
   */
  enum LabelMasks
  {
    MIN_X = vtkGridAxesHelper::MIN_X,
    MIN_Y = vtkGridAxesHelper::MIN_Y,
    MIN_Z = vtkGridAxesHelper::MIN_Z,
    MAX_X = vtkGridAxesHelper::MAX_X,
    MAX_Y = vtkGridAxesHelper::MAX_Y,
    MAX_Z = vtkGridAxesHelper::MAX_Z
  };

  //@{
  /**
   * Set the axes to label.
   */
  vtkSetMacro(LabelMask, unsigned int);
  vtkGetMacro(LabelMask, unsigned int);
  //@}

  //@{
  /**
   * Enable/Disable layer support. Default is off. When enabled, the prop can
   * render in there separate layers:
   * \li \c BackgroundLayer for all text labels and titles on the back faces,
   * \li \c GeometryLayer for all 3D geometry e.g the grid wireframe, and
   * \li \c ForegroundLayer for all text labels and titles on the front faces.
   */
  vtkSetMacro(EnableLayerSupport, bool);
  vtkGetMacro(EnableLayerSupport, bool);
  vtkBooleanMacro(EnableLayerSupport, bool);
  //@}

  //@{
  /**
   * Get/Set the layer in which to render all background actors/text when
   * EnableLayerSupport is ON. Default is 0.
   */
  vtkSetMacro(BackgroundLayer, int);
  vtkGetMacro(BackgroundLayer, int);
  //@}

  //@{
  /**
   * Get/Set the layer in which to render all 3D actors when
   * EnableLayerSupport is ON. Default is 0.
   */
  vtkSetMacro(GeometryLayer, int);
  vtkGetMacro(GeometryLayer, int);
  //@}

  //@{
  /**
   * Get/Set the layer in which to render all foreground actors/text when
   * EnableLayerSupport is ON. Default is 0.
   */
  vtkSetMacro(ForegroundLayer, int);
  vtkGetMacro(ForegroundLayer, int);
  //@}

  //@{
  /**
   * Get/Set the property used to control the appearance of the rendered grid.
   */
  void SetProperty(vtkProperty*);
  vtkProperty* GetProperty();
  //@}

  //@{
  /**
   * Get/Set the title text properties for each of the coordinate axes. Which
   * properties will be used depends on the selected Face being rendered.
   */
  void SetTitleTextProperty(int axis, vtkTextProperty*);
  vtkTextProperty* GetTitleTextProperty(int axis);
  //@}

  //@{
  /**
   * Get/Set the label text properties for each of the coordinate axes. Which
   * properties will be used depends on the selected Face being rendered.
   */
  void SetLabelTextProperty(int axis, vtkTextProperty*);
  vtkTextProperty* GetLabelTextProperty(int axis);
  //@}

  //@{
  /**
   * Set titles for each of the axes.
   */
  void SetTitle(int axis, const vtkStdString& title);
  const vtkStdString& GetTitle(int axis);
  //@}

  //@{
  /**
   * Get/set the numerical notation, standard, scientific or mixed (0, 1, 2).
   * Accepted values are vtkAxis::AUTO, vtkAxis::FIXED, vtkAxis::CUSTOM.
   */
  void SetNotation(int axis, int notation);
  int GetNotation(int axis);
  //@}

  //@{
  /**
   * Get/set the numerical precision to use, default is 2.
   */
  void SetPrecision(int axis, int val);
  int GetPrecision(int axis);
  //@}

  /**
   * Set custom tick positions for each of the axes.
   * The positions are deep copied. Set to NULL to not use custom tick positions
   * for the axis.
   */
  void SetCustomTickPositions(int axis, vtkDoubleArray* positions);

  //---------------------------------------------------------------------------
  // *** Properties to control grid rendering ***
  //---------------------------------------------------------------------------

  /**
   * Turn off to not generate polydata for the plane's grid.
   */
  void SetGenerateGrid(bool val) { this->PlaneActor->SetGenerateGrid(val); }
  bool GetGenerateGrid() { return this->PlaneActor->GetGenerateGrid(); }
  vtkBooleanMacro(GenerateGrid, bool);

  /**
   * Turn off to not generate the polydata for the plane's edges. Which edges
   * are rendered is defined by the EdgeMask.
   */
  void SetGenerateEdges(bool val) { this->PlaneActor->SetGenerateEdges(val); }
  bool GetGenerateEdges() { return this->PlaneActor->GetGenerateEdges(); }
  vtkBooleanMacro(GenerateEdges, bool);

  // Turn off to not generate the markers for the tick positions. Which egdes
  // are rendered is defined by the TickMask.
  void SetGenerateTicks(bool val) { this->PlaneActor->SetGenerateTicks(val); }
  bool GetGenerateTicks() { return this->PlaneActor->GetGenerateTicks(); }
  vtkBooleanMacro(GenerateTicks, bool);

  //--------------------------------------------------------------------------
  // Methods for vtkProp3D API.
  //--------------------------------------------------------------------------

  //@{
  /**
   * Returns the prop bounds.
   */
  virtual double* GetBounds()
  {
    this->GetGridBounds(this->Bounds);
    return this->Bounds;
  }
  //@}

  virtual int RenderOpaqueGeometry(vtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport* viewport);
  virtual int RenderOverlay(vtkViewport* viewport);
  virtual int HasTranslucentPolygonalGeometry();
  virtual void ReleaseGraphicsResources(vtkWindow*);

  /**
   * Overridden to include the mtime for the text properties.
   */
  vtkMTimeType GetMTime();

protected:
  vtkGridAxes2DActor();
  ~vtkGridAxes2DActor();

  bool Update(vtkViewport* viewport);
  void UpdateTextProperties(vtkViewport* viewport);
  void UpdateLabelPositions(vtkViewport* viewport);
  void UpdateTextActors(vtkViewport* viewport);
  friend class vtkGridAxes3DActor;

  double GridBounds[6];
  int Face;
  unsigned int LabelMask;

  bool EnableLayerSupport;
  int BackgroundLayer;
  int ForegroundLayer;
  int GeometryLayer;

  vtkTuple<vtkSmartPointer<vtkTextProperty>, 3> TitleTextProperty;
  vtkTuple<vtkSmartPointer<vtkTextProperty>, 3> LabelTextProperty;
  vtkTuple<vtkStdString, 3> Titles;

  vtkNew<vtkGridAxesHelper> Helper;
  vtkSmartPointer<vtkGridAxesPlane2DActor> PlaneActor;
  vtkNew<vtkAxis> AxisHelpers[3];
  vtkNew<vtkContextScene> AxisHelperScene;
  vtkTimeStamp UpdateLabelTextPropertiesMTime;

private:
  vtkGridAxes2DActor(const vtkGridAxes2DActor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGridAxes2DActor&) VTK_DELETE_FUNCTION;

  class vtkLabels;
  vtkLabels* Labels;
  friend class vtkLabels;

  bool DoRender;
};

#endif
