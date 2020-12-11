/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAnimationSceneWebWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMAnimationSceneWebWriter
 * @brief   helper class to write
 * animation geometry in a web archive.
 *
 * vtkSMAnimationSceneWebWriter is a concrete implementation of
 * vtkSMAnimationSceneWriter that can write the geometry as a web archive.
 * This writer can only write the visible geometry in one view.
*/

#ifndef vtkSMAnimationSceneWebWriter_h
#define vtkSMAnimationSceneWebWriter_h

#include "vtkRemotingAnimationModule.h" //needed for exports
#include "vtkSMAnimationSceneWriter.h"

class vtkSMProxy;
class vtkSMRenderViewProxy;

class VTKREMOTINGANIMATION_EXPORT vtkSMAnimationSceneWebWriter : public vtkSMAnimationSceneWriter
{
public:
  static vtkSMAnimationSceneWebWriter* New();
  vtkTypeMacro(vtkSMAnimationSceneWebWriter, vtkSMAnimationSceneWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get/Set the (render) View Module from which we are writing
   * the geometry.
   */
  vtkGetObjectMacro(RenderView, vtkSMRenderViewProxy);
  void SetRenderView(vtkSMRenderViewProxy*);

protected:
  vtkSMAnimationSceneWebWriter();
  ~vtkSMAnimationSceneWebWriter() override;

  /**
   * Called to initialize saving.
   */
  bool SaveInitialize(int startCount) override;

  /**
   * Called to save a particular frame.
   */
  bool SaveFrame(double time) override;

  /**
   * Called to finalize saving.
   */
  bool SaveFinalize() override;

  vtkSMRenderViewProxy* RenderView = nullptr;

private:
  struct vtkInternals;
  vtkInternals* Internals;

  vtkSMAnimationSceneWebWriter(const vtkSMAnimationSceneWebWriter&) = delete;
  void operator=(const vtkSMAnimationSceneWebWriter&) = delete;
};

#endif
