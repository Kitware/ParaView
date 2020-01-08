/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraCollection.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVCameraCollection
 * @brief collection of cameras used by vtkPVRenderView for cases where
 * one wants to limit interactions to a fixed set of cameras.
 *
 * vtkPVCameraCollection is designed for cases where one want to
 * limit interactions to a fixed set of cameras e.g. when showing pre-rendered
 * image frames e.g. coming from a Cinema database, one wants to limit the
 * camera positions to those available in the Cinema database. One can use this
 * class for that.
 *
 * This implementation provides API to add cameras, find best match camera, etc.
 *
 */

#ifndef vtkPVCameraCollection_h
#define vtkPVCameraCollection_h

#include "vtkObject.h"

#include "vtkNew.h"                 // needed for vtkNew
#include "vtkRemotingViewsModule.h" // needed for export macro

class vtkCamera;

class VTKREMOTINGVIEWS_EXPORT vtkPVCameraCollection : public vtkObject
{
public:
  static vtkPVCameraCollection* New();
  vtkTypeMacro(vtkPVCameraCollection, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Add  a cameras to the collection of discrete cameras. The code does not
   * check for duplicates. It's the callers responsibility to ensure duplicate
   * cameras are not added as it's not guaranteed which of the duplicates will
   * be selected.
   * @param[in] camera A camera to add to the collection of discrete cameras.
   * @returns Index of the added camera.
   */
  virtual int AddCamera(vtkCamera* camera);

  /**
   * Return camera at a particular index.
   * @param[in] index Index of added camera to return
   * @returns camera or NULL if index is invalid.
   */
  vtkCamera* GetCamera(int index);

  /**
   * Clear all cameras.
   */
  virtual void RemoveAllCameras();

  /**
   * Find a camera in the style that's closest to the `target` camera.
   * @param[in] target Camera to find the closet camera to.
   * @returns index of the found camera or -1 if none found.
   */
  int FindClosestCamera(vtkCamera* target);

  /**
   * Update the \c target camera to the values from a camera added using `AddCamera`.
   * The camera to copy values from is identified by the index.
   *
   * @param[in] index Index of source camera.
   * @param[out] target The camera to update.
   * @returns false if params are invalid, otherwise true.
   */
  virtual bool UpdateCamera(int index, vtkCamera* target);

protected:
  vtkPVCameraCollection();
  ~vtkPVCameraCollection() override;

  int LastCameraIndex;

private:
  vtkPVCameraCollection(const vtkPVCameraCollection&) = delete;
  void operator=(const vtkPVCameraCollection&) = delete;
  class vtkInternals;
  vtkInternals* Internals;
};

#endif
