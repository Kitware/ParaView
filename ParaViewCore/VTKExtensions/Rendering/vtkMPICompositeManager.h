/*=========================================================================

  Program:   ParaView
  Module:    vtkMPICompositeManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMPICompositeManager
 * @brief   Composites when running in MPI mode.
 *
 *
*/

#ifndef vtkMPICompositeManager_h
#define vtkMPICompositeManager_h

#include "vtkCompositeRenderManager.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkMPICompositeManager : public vtkCompositeRenderManager
{
public:
  static vtkMPICompositeManager* New();
  vtkTypeMacro(vtkMPICompositeManager, vtkCompositeRenderManager);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * If the user wants to handle the event loop, then they must call this
   * method to initialize the RMIs.
   */
  virtual void InitializeRMIs() override;

  /**
   * Get the z buffer value at a pixel.  GatherZBufferValue is
   * an internal method. Called only on root node.
   */
  float GetZBufferValue(int x, int y);

  /**
   * Internal method: called only on non-Root nodes.
   */
  void GatherZBufferValueRMI(int x, int y);

  enum Tags
  {
    GATHER_Z_RMI_TAG = 987987,
    Z_TAG = 88771
  };

  /**
   * Overridden to set the Image Size when ParallelRendering is off.
   */
  virtual void StartRender() override;

  //@{
  /**
   * Select buffer to read from / render into.
   * Overridden to choose the back buffer only when the
   * buffers haven;t been swapped yet.
   */
  virtual int ChooseBuffer() override;

protected:
  vtkMPICompositeManager();
  ~vtkMPICompositeManager();
  //@}

private:
  vtkMPICompositeManager(const vtkMPICompositeManager&) = delete;
  void operator=(const vtkMPICompositeManager&) = delete;
};

#endif
