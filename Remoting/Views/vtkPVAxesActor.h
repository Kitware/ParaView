/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVAxesActor
 * @brief   a 3D axes representation
 *
 *
 * vtkPVAxesActor is used to represent 3D axes in the scene. The user can
 * define the geometry to use for the shaft and the tip, and the user can
 * set the text for the three axes. The text will follow the camera.
*/

#ifndef vtkPVAxesActor_h
#define vtkPVAxesActor_h

#include "vtkProp3D.h"
#include "vtkRemotingViewsModule.h" // needed for export macro

class vtkRenderer;
class vtkPropCollection;
class vtkMapper;
class vtkProperty;
class vtkActor;
class vtkFollower;
class vtkCylinderSource;
class vtkLineSource;
class vtkConeSource;
class vtkSphereSource;
class vtkPolyData;
class vtkVectorText;

class VTKREMOTINGVIEWS_EXPORT vtkPVAxesActor : public vtkProp3D
{
public:
  static vtkPVAxesActor* New();
  vtkTypeMacro(vtkPVAxesActor, vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * For some exporters and other other operations we must be
   * able to collect all the actors or volumes. These methods
   * are used in that process.
   */
  void GetActors(vtkPropCollection*) override;

  //@{
  /**
   * Support the standard render methods.
   */
  int RenderOpaqueGeometry(vtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport* viewport) override;
  int HasTranslucentPolygonalGeometry() override;
  //@}

  /**
   * Shallow copy of an axes actor. Overloads the virtual vtkProp method.
   */
  void ShallowCopy(vtkProp* prop) override;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  //@{
  /**
   * Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax). (The
   * method GetBounds(double bounds[6]) is available from the superclass.)
   */
  void GetBounds(double bounds[6]);
  double* GetBounds() override;
  //@}

  /**
   * Get the actors mtime plus consider its properties and texture if set.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Return the mtime of anything that would cause the rendered image to
   * appear differently. Usually this involves checking the mtime of the
   * prop plus anything else it depends on such as properties, textures
   * etc.
   */
  vtkMTimeType GetRedrawMTime() override;

  //@{
  /**
   * Set the total length of the axes in 3 dimensions.
   */
  void SetTotalLength(float v[3]) { this->SetTotalLength(v[0], v[1], v[2]); }
  void SetTotalLength(float x, float y, float z);
  vtkGetVectorMacro(TotalLength, float, 3);
  //@}

  //@{
  /**
   * Set the normalized (0-1) length of the shaft.
   */
  void SetNormalizedShaftLength(float v[3]) { this->SetNormalizedShaftLength(v[0], v[1], v[2]); }
  void SetNormalizedShaftLength(float x, float y, float z);
  vtkGetVectorMacro(NormalizedShaftLength, float, 3);
  //@}

  //@{
  /**
   * Set the normalized (0-1) length of the tip.
   */
  void SetNormalizedTipLength(float v[3]) { this->SetNormalizedTipLength(v[0], v[1], v[2]); }
  void SetNormalizedTipLength(float x, float y, float z);
  vtkGetVectorMacro(NormalizedTipLength, float, 3);
  //@}

  //@{
  /**
   * Set/get the resolution of the pieces of the axes actor
   */
  vtkSetClampMacro(ConeResolution, int, 3, 128);
  vtkGetMacro(ConeResolution, int);
  vtkSetClampMacro(SphereResolution, int, 3, 128);
  vtkGetMacro(SphereResolution, int);
  vtkSetClampMacro(CylinderResolution, int, 3, 128);
  vtkGetMacro(CylinderResolution, int);
  //@}

  //@{
  /**
   * Set/get the radius of the pieces of the axes actor
   */
  vtkSetClampMacro(ConeRadius, float, 0, VTK_FLOAT_MAX);
  vtkGetMacro(ConeRadius, float);
  vtkSetClampMacro(SphereRadius, float, 0, VTK_FLOAT_MAX);
  vtkGetMacro(SphereRadius, float);
  vtkSetClampMacro(CylinderRadius, float, 0, VTK_FLOAT_MAX);
  vtkGetMacro(CylinderRadius, float);
  //@}

  //@{
  /**
   * Set/get the positions of the axis labels
   */
  vtkSetClampMacro(XAxisLabelPosition, float, 0, 1);
  vtkGetMacro(XAxisLabelPosition, float);
  vtkSetClampMacro(YAxisLabelPosition, float, 0, 1);
  vtkGetMacro(YAxisLabelPosition, float);
  vtkSetClampMacro(ZAxisLabelPosition, float, 0, 1);
  vtkGetMacro(ZAxisLabelPosition, float);
  //@}

  /**
   * Set the type of the shaft to a cylinder, line, or user defined geometry.
   */
  void SetShaftType(int type);
  void SetShaftTypeToCylinder() { this->SetShaftType(vtkPVAxesActor::CYLINDER_SHAFT); }
  void SetShaftTypeToLine() { this->SetShaftType(vtkPVAxesActor::LINE_SHAFT); }
  void SetShaftTypeToUserDefined() { this->SetShaftType(vtkPVAxesActor::USER_DEFINED_SHAFT); }

  /**
   * Set the type of the tip to a cone, sphere, or user defined geometry.
   */
  void SetTipType(int type);
  void SetTipTypeToCone() { this->SetTipType(vtkPVAxesActor::CONE_TIP); }
  void SetTipTypeToSphere() { this->SetTipType(vtkPVAxesActor::SPHERE_TIP); }
  void SetTipTypeToUserDefined() { this->SetTipType(vtkPVAxesActor::USER_DEFINED_TIP); }

  //@{
  /**
   * Set the user defined tip polydata.
   */
  void SetUserDefinedTip(vtkPolyData*);
  vtkGetObjectMacro(UserDefinedTip, vtkPolyData);
  //@}

  //@{
  /**
   * Set the user defined shaft polydata.
   */
  void SetUserDefinedShaft(vtkPolyData*);
  vtkGetObjectMacro(UserDefinedShaft, vtkPolyData);
  //@}

  //@{
  /**
   * Get the tip properties.
   */
  vtkProperty* GetXAxisTipProperty();
  vtkProperty* GetYAxisTipProperty();
  vtkProperty* GetZAxisTipProperty();
  //@}

  //@{
  /**
   * Get the shaft properties.
   */
  vtkProperty* GetXAxisShaftProperty();
  vtkProperty* GetYAxisShaftProperty();
  vtkProperty* GetZAxisShaftProperty();
  //@}

  //@{
  /**
   * Get the label properties.
   */
  vtkProperty* GetXAxisLabelProperty();
  vtkProperty* GetYAxisLabelProperty();
  vtkProperty* GetZAxisLabelProperty();
  //@}

  //
  //@{
  /**
   * Set the label text.
   */
  vtkSetStringMacro(XAxisLabelText);
  vtkSetStringMacro(YAxisLabelText);
  vtkSetStringMacro(ZAxisLabelText);
  //@}

  enum
  {
    CYLINDER_SHAFT,
    LINE_SHAFT,
    USER_DEFINED_SHAFT
  };

  enum
  {
    CONE_TIP,
    SPHERE_TIP,
    USER_DEFINED_TIP
  };

protected:
  vtkPVAxesActor();
  ~vtkPVAxesActor() override;

  vtkCylinderSource* CylinderSource;
  vtkLineSource* LineSource;
  vtkConeSource* ConeSource;
  vtkSphereSource* SphereSource;

  vtkActor* XAxisShaft;
  vtkActor* YAxisShaft;
  vtkActor* ZAxisShaft;

  vtkActor* XAxisTip;
  vtkActor* YAxisTip;
  vtkActor* ZAxisTip;

  void UpdateProps();

  float TotalLength[3];
  float NormalizedShaftLength[3];
  float NormalizedTipLength[3];

  int ShaftType;
  int TipType;

  vtkPolyData* UserDefinedTip;
  vtkPolyData* UserDefinedShaft;

  char* XAxisLabelText;
  char* YAxisLabelText;
  char* ZAxisLabelText;

  vtkVectorText* XAxisVectorText;
  vtkVectorText* YAxisVectorText;
  vtkVectorText* ZAxisVectorText;

  vtkFollower* XAxisLabel;
  vtkFollower* YAxisLabel;
  vtkFollower* ZAxisLabel;

  int ConeResolution;
  int SphereResolution;
  int CylinderResolution;

  float ConeRadius;
  float SphereRadius;
  float CylinderRadius;

  float XAxisLabelPosition;
  float YAxisLabelPosition;
  float ZAxisLabelPosition;

private:
  vtkPVAxesActor(const vtkPVAxesActor&) = delete;
  void operator=(const vtkPVAxesActor&) = delete;
};

#endif
