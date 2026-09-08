// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMAnimationSceneSeriesWriter
 * @brief   helper class to write file series from animated scene
 *
 * vtkSMAnimationSceneSeriesWriter is a concrete implementation of
 * vtkSMAnimationSceneWriter that can write one file per animation time, using a provided exporter.
 *
 * As an exporter, it is intended to write the data as displayed in the current view.
 */

#ifndef vtkSMAnimationSceneSeriesWriter_h
#define vtkSMAnimationSceneSeriesWriter_h

#include "vtkRemotingAnimationModule.h" //needed for exports
#include "vtkSMAnimationSceneWriter.h"

class vtkSMExporterProxy;
class vtkSMViewProxy;

class VTKREMOTINGANIMATION_EXPORT vtkSMAnimationSceneSeriesWriter : public vtkSMAnimationSceneWriter
{
public:
  static vtkSMAnimationSceneSeriesWriter* New();
  vtkTypeMacro(vtkSMAnimationSceneSeriesWriter, vtkSMAnimationSceneWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * When true, the delegate exporter is expected to support writing all
   * animation frames into a single file as time samples: "Start" and
   * "Finish" commands bracket the frame sequence and a "TimeValue"
   * property is set before each frame, instead of generating a new
   * filename per frame. Default is false, and means that each frame
   * will be exported to one file. Only exporters whose proxy XML declares
   * these commands should enable this.
   */
  vtkSetMacro(SingleFile, bool);
  vtkGetMacro(SingleFile, bool);
  vtkBooleanMacro(SingleFile, bool);
  ///@}

protected:
  vtkSMAnimationSceneSeriesWriter() = default;
  ~vtkSMAnimationSceneSeriesWriter() override = default;

  /**
   * Called to initialize saving.
   * Return false if FrameExporterDelegate is nullptr.
   */
  bool SaveInitialize(int startCount) override;

  /**
   * Write current scene frame using Exporter.
   *
   * This creates a dedicated filename for the current frame.
   */
  bool SaveFrame(double time) override;

  /**
   * Called to finalize saving.
   */
  bool SaveFinalize() override;

private:
  vtkSMAnimationSceneSeriesWriter(const vtkSMAnimationSceneSeriesWriter&) = delete;
  void operator=(const vtkSMAnimationSceneSeriesWriter&) = delete;

  /**
   * Return FileName if AnimationEnabled is false.
   * Construct a file path from FileName and FrameCounter otherwise:
   * For FileName being at format /path/basename.extension,
   * return /path/basename.FrameCounter.extension
   */
  std::string BuildCurrentFilePath();

  int FrameCounter = 0;
  bool SingleFile = false;
};

#endif
