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

#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro
#include "vtkVolume.h"

class vtkLODProp3D;
class vtkMapper;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkPVLODVolume : public vtkVolume
{
public:
  vtkTypeMacro(vtkPVLODVolume, vtkVolume);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  static vtkPVLODVolume* New();

  //@{
  /**
   * This method is used internally by the rendering process.
   */
  virtual int RenderOpaqueGeometry(vtkViewport* viewport) VTK_OVERRIDE;
  virtual int RenderVolumetricGeometry(vtkViewport* viewport) VTK_OVERRIDE;
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*) VTK_OVERRIDE;
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  virtual int HasTranslucentPolygonalGeometry() VTK_OVERRIDE;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow*) VTK_OVERRIDE;

  //@{
  /**
   * Set the high res input.  Overloads the virtual vtkVolume method.
   */
  virtual void SetMapper(vtkAbstractVolumeMapper*);
  virtual vtkAbstractVolumeMapper* GetMapper() VTK_OVERRIDE;
  //@}

  //@{
  /**
   * This sets the low res input.
   */
  virtual void SetLODMapper(vtkAbstractVolumeMapper*);
  virtual void SetLODMapper(vtkMapper*);
  //@}

  /**
   * Sets the volume propery.  Overloads the virtual vtkVolume method.
   */
  virtual void SetProperty(vtkVolumeProperty* property);

  /**
   * Shallow copy of an LOD actor. Overloads the virtual vtkProp method.
   */
  virtual void ShallowCopy(vtkProp* prop) VTK_OVERRIDE;

  /**
   * Get the bounds of the current mapper.
   */
  virtual double* GetBounds() VTK_OVERRIDE;

  /**
   * Overloads the virtual vtkProp method.
   */
  virtual void SetAllocatedRenderTime(double t, vtkViewport* v) VTK_OVERRIDE;

  //@{
  /**
   * When set, LODMapper, if present it used, otherwise the regular mapper is
   * used.
   */
  vtkSetMacro(EnableLOD, int);
  vtkGetMacro(EnableLOD, int);

protected:
  vtkPVLODVolume();
  ~vtkPVLODVolume();
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
  vtkPVLODVolume(const vtkPVLODVolume&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVLODVolume&) VTK_DELETE_FUNCTION;
};

#endif
