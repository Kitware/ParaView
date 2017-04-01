/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAnimationSceneImageWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMAnimationSceneImageWriter
 * @brief   helper class to write animation
 * images/movies.
 *
 * vtkSMAnimationSceneImageWriter is a subclass of
 * vtkSMAnimationSceneWriter that can write movies or images. This is not
 * intended to be used directly.
 * @sa vtkSMSaveAnimationProxy.
*/

#ifndef vtkSMAnimationSceneImageWriter_h
#define vtkSMAnimationSceneImageWriter_h

#include "vtkSMAnimationSceneWriter.h"

#include "vtkPVAnimationModule.h" // needed for exports
#include "vtkSmartPointer.h"      // needed for vtkSmartPointer.
#include <string>                 // needed for std::string

class vtkGenericMovieWriter;
class vtkImageData;
class vtkImageWriter;

class VTKPVANIMATION_EXPORT vtkSMAnimationSceneImageWriter : public vtkSMAnimationSceneWriter
{
public:
  vtkTypeMacro(vtkSMAnimationSceneImageWriter, vtkSMAnimationSceneWriter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the quality for the generated movie. 0 is worst quality,
   * 100 is best quality.
   */
  vtkSetClampMacro(Quality, int, 0, 100);
  vtkGetMacro(Quality, int);
  //@}

  //@{
  /**
   * Get the error code which is set if there's an error while writing
   * the images.
   */
  vtkGetMacro(ErrorCode, int);
  //@}

  //@{
  /**
  * Get/Set the frame rate to use for saving the animation.
  * This frame rate is the frame rate that gets saved in the movie
  * file generated, if applicable. If does not affect the FrameRate
  * set on the animation scene at all. In other words, this is the
  * playback frame rate and not the animation generation frame rate.
  * Default value is 1.
  */
  vtkSetMacro(FrameRate, double);
  vtkGetMacro(FrameRate, double);
  //@}

protected:
  vtkSMAnimationSceneImageWriter();
  ~vtkSMAnimationSceneImageWriter();

  /**
   * Called to initialize saving.
   */
  virtual bool SaveInitialize(int startCount) VTK_OVERRIDE;

  /**
   * Called to save a particular frame.
   */
  virtual bool SaveFrame(double time) VTK_OVERRIDE;

  /**
   * Called to finalize saving.
   */
  virtual bool SaveFinalize() VTK_OVERRIDE;

  /**
   * Capture and return an image for the current frame.
   * If nullptr is returned, then the frame is skipped. If all frames are empty,
   * then no output is generated.
   */
  virtual vtkSmartPointer<vtkImageData> CaptureFrame() = 0;

  // Creates the writer based on file type.
  bool CreateWriter();

  int Quality;
  int FileCount;
  int ErrorCode;
  double FrameRate;
  std::string Prefix;
  std::string Suffix;
  vtkSmartPointer<vtkImageWriter> ImageWriter;
  vtkSmartPointer<vtkGenericMovieWriter> MovieWriter;

private:
  vtkSMAnimationSceneImageWriter(const vtkSMAnimationSceneImageWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMAnimationSceneImageWriter&) VTK_DELETE_FUNCTION;

  bool MovieWriterStarted;
};

#endif
