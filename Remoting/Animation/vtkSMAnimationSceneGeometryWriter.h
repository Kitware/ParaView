// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMAnimationSceneGeometryWriter
 * @brief   helper class to write
 * animation geometry in a data file.
 *
 * vtkSMAnimationSceneGeometryWriter is a concrete implementation of
 * vtkSMAnimationSceneWriter that can write the geometry as a data file.
 * This writer can only write the visible geometry in one view.
 */

#ifndef vtkSMAnimationSceneGeometryWriter_h
#define vtkSMAnimationSceneGeometryWriter_h

#include "vtkRemotingAnimationModule.h" //needed for exports
#include "vtkSMAnimationSceneWriter.h"

class vtkSMProxy;

class VTKREMOTINGANIMATION_EXPORT vtkSMAnimationSceneGeometryWriter
  : public vtkSMAnimationSceneWriter
{
public:
  static vtkSMAnimationSceneGeometryWriter* New();
  vtkTypeMacro(vtkSMAnimationSceneGeometryWriter, vtkSMAnimationSceneWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Get/Set the View Module from which we are writing the
  // geometry.
  vtkGetObjectMacro(ViewModule, vtkSMProxy);
  void SetViewModule(vtkSMProxy*);

protected:
  vtkSMAnimationSceneGeometryWriter();
  ~vtkSMAnimationSceneGeometryWriter() override;

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

  vtkSMProxy* GeometryWriter;
  vtkSMProxy* ViewModule;

private:
  vtkSMAnimationSceneGeometryWriter(const vtkSMAnimationSceneGeometryWriter&) = delete;
  void operator=(const vtkSMAnimationSceneGeometryWriter&) = delete;
};

#endif
