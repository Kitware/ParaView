/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODVolume.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVLODVolume
 * @brief   an actor that supports multiple levels of detail
 *
 * vtkPVLODVolume is much like vtkPVLODActor except that it works on
 * volumes instead of surfaces.  This just has two mappers: full res and
 * LOD, and this actor knows which is which.
 *
 * @sa
 * vtkActor vtkRenderer vtkLODProp3D vtkLODActor
*/

#ifndef vtkPVLODVolume_h
#define vtkPVLODVolume_h

#include "vtkRemotingViewsModule.h" // needed for export macro
#include "vtkVolume.h"

class vtkLODProp3D;
class vtkMapper;

class VTKREMOTINGVIEWS_EXPORT vtkPVLODVolume : public vtkVolume
{
public:
  vtkTypeMacro(vtkPVLODVolume, vtkVolume);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPVLODVolume* New();

  //@{
  /**
   * This method is used internally by the rendering process.
   */
  int RenderOpaqueGeometry(vtkViewport* viewport) override;
  int RenderVolumetricGeometry(vtkViewport* viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  int HasTranslucentPolygonalGeometry() override;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  //@{
  /**
   * Set the high res input.  Overloads the virtual vtkVolume method.
   */
  virtual void SetMapper(vtkAbstractVolumeMapper*);
  vtkAbstractVolumeMapper* GetMapper() override;
  //@}

  //@{
  /**
   * This sets the low res input.
   */
  virtual void SetLODMapper(vtkAbstractVolumeMapper*);
  virtual void SetLODMapper(vtkMapper*);
  //@}

  /**
   * Sets the volume property.  Overloads the virtual vtkVolume method.
   */
  virtual void SetProperty(vtkVolumeProperty* property) override;

  /**
   * Shallow copy of an LOD actor. Overloads the virtual vtkProp method.
   */
  void ShallowCopy(vtkProp* prop) override;

  /**
   * Get the bounds of the current mapper.
   */
  double* GetBounds() override;

  /**
   * Overloads the virtual vtkProp method.
   */
  void SetAllocatedRenderTime(double t, vtkViewport* v) override;

  //@{
  /**
   * When set, LODMapper, if present it used, otherwise the regular mapper is
   * used.
   */
  vtkSetMacro(EnableLOD, int);
  vtkGetMacro(EnableLOD, int);

  void SetPropertyKeys(vtkInformation* keys) override;

protected:
  vtkPVLODVolume();
  ~vtkPVLODVolume() override;
  //@}

  /**
   * Since volume mapper are notorious for segfaulting when the scalar array is
   * missing we use this method to validate that we can actually render the
   * data.
   */
  bool CanRender();

  vtkLODProp3D* LODProp;
  int HighLODId;
  int LowLODId;
  int EnableLOD;
  int SelectLOD();
  double MapperBounds[6];
  vtkTimeStamp BoundsMTime;
  virtual void UpdateLODProperty();

private:
  vtkPVLODVolume(const vtkPVLODVolume&) = delete;
  void operator=(const vtkPVLODVolume&) = delete;
};

#endif
