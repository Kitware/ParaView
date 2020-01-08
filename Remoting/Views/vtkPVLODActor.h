/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODActor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVLODActor
 * @brief   an actor that supports multiple levels of detail
 *
 * vtkPVLODActor  is a very simple version of vtkLODActor.  Both
 * vtkLODActor and vtkLODProp3D can get confused, and substitute
 * LOD mappers when they are not needed.  This just has two mappers:
 * full res and LOD, and this actor knows which is which.
 *
 * @sa
 * vtkActor vtkRenderer vtkLODProp3D vtkLODActor
*/

#ifndef vtkPVLODActor_h
#define vtkPVLODActor_h

#include "vtkActor.h"
#include "vtkRemotingViewsModule.h" // needed for export macro

class vtkMapper;
class vtkPiecewiseFunction;

class VTKREMOTINGVIEWS_EXPORT vtkPVLODActor : public vtkActor
{
public:
  vtkTypeMacro(vtkPVLODActor, vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPVLODActor* New();

  /**
   * This causes the actor to be rendered. It, in turn, will render the actor's
   * property and then mapper.
   */
  void Render(vtkRenderer*, vtkMapper*) override;

  /**
   * This method is used internally by the rendering process.
   * We override the superclass method to properly set the estimated render time.
   */
  int RenderOpaqueGeometry(vtkViewport* viewport) override;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  //@{
  /**
   * This sets the low res input.
   */
  virtual void SetLODMapper(vtkMapper*);
  vtkGetObjectMacro(LODMapper, vtkMapper);
  //@}

  /**
   * This is a bit of a hack.  This returns the last mapper used to render.
   * It does this so that compositing can decide if anything was actually renderered.
   */
  vtkMapper* GetMapper() override { return this->SelectMapper(); }

  /**
   * When this objects gets modified, this method also modifies the object.
   */
  void Modified() override;

  /**
   * Shallow copy of an LOD actor. Overloads the virtual vtkProp method.
   */
  void ShallowCopy(vtkProp* prop) override;

  /**
   * Get the bounds of the current mapper.
   */
  double* GetBounds() override;

  /**
   * When set, LODMapper, if present it used, otherwise the regular mapper is
   * used. We deliberately don't change the MTime of the actor when toggling
   * EnableLOD state to avoid rebuilding of rendering data structures.
   */
  void SetEnableLOD(int val) { this->EnableLOD = val; }
  vtkGetMacro(EnableLOD, int);

  //@{
  /**
   * For OSPRay controls sizing of implicit spheres (points) and
   * cylinders (lines)
   */
  virtual void SetEnableScaling(int v);
  virtual void SetScalingArrayName(const char*);
  virtual void SetScalingFunction(vtkPiecewiseFunction* pwf);
  //@}

protected:
  vtkPVLODActor();
  ~vtkPVLODActor() override;
  vtkActor* Device;
  vtkMapper* LODMapper;

  vtkMapper* SelectMapper();

  int EnableLOD;

private:
  vtkPVLODActor(const vtkPVLODActor&) = delete;
  void operator=(const vtkPVLODActor&) = delete;
};

#endif
