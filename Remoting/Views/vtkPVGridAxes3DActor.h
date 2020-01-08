/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGridAxes3DActor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVGridAxes3DActor
 * @brief   ParaView extensions for vtkGridAxes3DActor.
 *
 * vtkPVGridAxes3DActor adds support for ParaView-specific use-cases to
 * vtkGridAxes3DActor.
 * The special use-cases are:
 * \li Showing original data bounds when data has been transformed by
 * specifying a transform on the \c Display properties: To support this, we add
 * API to set the DataScale and DataPosition (we cannot support rotations, I am
 * afraid). User is expected to set these to match the Scale and Position set
 * on the \c Display properties of the data. vtkPVGridAxes3DActor converts that
 * to scale and position on the vtkProperty for the superclass and sets the
 * GridBounds on the superclass transformed using an inverse of the specified
 * transform on the bounds reported by vtkPVRenderView.
 * vtkPVRenderView uses SetTransformedBounds to set the bounds.
 *
 * \li Supporting change of basis transformations: for that, one is expected to
 * set UseModelTransform to true, and then specify the ModelBounds and
 * ModelTransformMatrix. Note: ModelTransform and DataScale/DataPosition are
 * mutually exclusive.
*/

#ifndef vtkPVGridAxes3DActor_h
#define vtkPVGridAxes3DActor_h

#include "vtkGridAxes3DActor.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class vtkMatrix4x4;

class VTKREMOTINGVIEWS_EXPORT vtkPVGridAxes3DActor : public vtkGridAxes3DActor
{
public:
  static vtkPVGridAxes3DActor* New();
  vtkTypeMacro(vtkPVGridAxes3DActor, vtkGridAxes3DActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Shallow copy from another vtkPVGridAxes3DActor.
   */
  void ShallowCopy(vtkProp* prop) override;

  //@{
  /**
   * Specify transformation used by the data.
   */
  vtkSetVector3Macro(DataScale, double);
  vtkGetVector3Macro(DataScale, double);
  //@}

  vtkSetVector3Macro(DataPosition, double);
  vtkGetVector3Macro(DataPosition, double);

  //@{
  /**
   * Specify the scale factor used to proportionally
   * scale each axis. 1 means no change.
   */
  vtkSetMacro(DataBoundsScaleFactor, double);
  vtkGetMacro(DataBoundsScaleFactor, double);
  //@}

  //@{
  /**
   * Another way for specifying grid bounds except here the bounds are
   * considered to be pre-transformed. Using the DataPosition, DataScale, and
   * DataBoundsScaleFactor, the provided bounds will un-transformed before
   * calling this->SetGridBounds() and then position and scale are instead
   * passed on the vtkActor. This results in display scaling rather than data
   * scaling.
   */
  vtkSetVector6Macro(TransformedBounds, double);
  vtkGetVector6Macro(TransformedBounds, double);
  //@}

  //@{
  /**
   * If set to true, CustomTransformedBounds are used instead of
   * TransformedBounds. Default is false.
   */
  vtkSetMacro(UseCustomTransformedBounds, bool);
  vtkGetMacro(UseCustomTransformedBounds, bool);
  //@}

  //@{
  /**
   * Same as TransformedBounds, except used only when UseCustomTransformedBounds
   * is set.
   */
  vtkSetVector6Macro(CustomTransformedBounds, double);
  vtkGetVector6Macro(CustomTransformedBounds, double);
  //@}

  vtkSetMacro(UseModelTransform, bool);
  vtkGetMacro(UseModelTransform, bool);
  vtkBooleanMacro(UseModelTransform, bool);
  vtkSetVector6Macro(ModelBounds, double);
  vtkGetVector6Macro(ModelBounds, double);
  void SetModelTransformMatrix(double* matrix);

  /**
   * Overridden to ensure that the transform information is passed on the
   * superclass.
   */
  double* GetBounds() override;

protected:
  vtkPVGridAxes3DActor();
  ~vtkPVGridAxes3DActor() override;

  void Update(vtkViewport* viewport) override;
  void UpdateGridBounds();
  void UpdateGridBoundsUsingDataBounds();
  void UpdateGridBoundsUsingModelTransform();

  double DataScale[3];
  double DataPosition[3];
  double DataBoundsScaleFactor;
  double TransformedBounds[6];

  bool UseModelTransform;
  double ModelBounds[6];
  vtkNew<vtkMatrix4x4> ModelTransformMatrix;

  bool UseCustomTransformedBounds;
  double CustomTransformedBounds[6];

private:
  vtkPVGridAxes3DActor(const vtkPVGridAxes3DActor&) = delete;
  void operator=(const vtkPVGridAxes3DActor&) = delete;

  vtkTimeStamp BoundsUpdateTime;
};

#endif
