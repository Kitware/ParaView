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
// .NAME vtkSMAnimationSceneImageWriter - helper class to write animation 
// images/movies.
// .SECTION Description
// vtkSMAnimationSceneImageWriter is a concrete implementation of
// vtkSMAnimationSceneWriter that can write movies or images. The generated
// output's size and alignment is exactly as specified on the GUISize,
// WindowPosition properties of the view modules. One can optionally specify
// Magnification to scale the output.
// .SECTION Notes
// This class does not support changing the dimensions of the view, one has to 
// do that before calling Save(). It only provides Magnification which can scale 
// the size using integral scale factor.

#ifndef __vtkSMAnimationSceneImageWriter_h
#define __vtkSMAnimationSceneImageWriter_h

#include "vtkSMAnimationSceneWriter.h"

class vtkGenericMovieWriter;
class vtkImageData;
class vtkImageWriter;
class vtkSMViewProxy;

class VTK_EXPORT vtkSMAnimationSceneImageWriter : public vtkSMAnimationSceneWriter
{
public:
  static vtkSMAnimationSceneImageWriter* New();
  vtkTypeMacro(vtkSMAnimationSceneImageWriter, vtkSMAnimationSceneWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the magnification factor to use for the saved animation.
  vtkSetClampMacro(Magnification, int, 1, VTK_INT_MAX);
  vtkGetMacro(Magnification, int);

  // Description:
  // Get/Set the quality for the generated movie.
  // Applicable only if the choose file format supports it.
  // 0 means worst quality and smallest file size
  // 2 means best quality and largest file size
  vtkSetClampMacro(Quality, int, 0, 2);
  vtkGetMacro(Quality, int);

  // Description:
  // Get/Set the setting whether the movie encoder should use subsampling of
  // the chrome planes or not, if applicable. Since the human eye is more
  // sensitive to brightness than color variations, subsampling can be
  // useful to reduce the bitrate. Default value is 0.
  vtkSetMacro(Subsampling, int);
  vtkGetMacro(Subsampling, int);
  vtkBooleanMacro(Subsampling, int);

  // Description:
  // Get the error code which is set if there's an error while writing
  // the images.
  vtkGetMacro(ErrorCode, int);


  // Description:
  // Get/Set the RGB background color to use to fill empty spaces in the image.
  // RGB components are in the range [0,1].
  vtkSetVector3Macro(BackgroundColor, double);
  vtkGetVector3Macro(BackgroundColor, double);

  // Get/Set the frame rate to use for saving the animation.
  // This frame rate is the frame rate that gets saved in the movie 
  // file generated, if applicable. If does not affect the FrameRate
  // set on the animation scene at all. In other words, this is the 
  // playback frame rate and not the animation generation frame rate.
  // Default value is 1.
  vtkSetMacro(FrameRate, double);
  vtkGetMacro(FrameRate, double);


  // Description:
  // Convenience method used to merge a smaller image (\c src) into a 
  // larger one (\c dest). The location of the smaller image in the larger image
  // are determined by their extents.
  static void Merge(vtkImageData* dest, vtkImageData* src);
protected:
  vtkSMAnimationSceneImageWriter();
  ~vtkSMAnimationSceneImageWriter();

  // Description:
  // Called to initialize saving.
  virtual bool SaveInitialize();

  // Description:
  // Called to save a particular frame.
  virtual bool SaveFrame(double time);

  // Description:
  // Called to finalize saving.
  virtual bool SaveFinalize();

  // Creates the writer based on file type.
  bool CreateWriter();

  // Updates the ActualSize which is the 
  // resolution of the generated animation frame.
  void UpdateImageSize();

  // Description:
  // Captures the view from the given module and
  // returns a new Image data object. May return NULL.
  // Default implementation can only handle vtkSMViewProxy subclasses.
  // Subclassess must override to handle other types of view modules.
  virtual vtkImageData* CaptureViewImage(
    vtkSMViewProxy*, int magnification);

  vtkImageData* NewFrame();

  vtkSetVector2Macro(ActualSize, int);
  int ActualSize[2];
  int Quality;
  int Magnification;
  int FileCount;
  int ErrorCode;
  int Subsampling;

  char* Prefix;
  char* Suffix;
  vtkSetStringMacro(Prefix);
  vtkSetStringMacro(Suffix);

  double BackgroundColor[3];
  double FrameRate;

  vtkImageWriter* ImageWriter;
  vtkGenericMovieWriter* MovieWriter;

  void SetImageWriter(vtkImageWriter*);
  void SetMovieWriter(vtkGenericMovieWriter*);
private:
  vtkSMAnimationSceneImageWriter(const vtkSMAnimationSceneImageWriter&); // Not implemented.
  void operator=(const vtkSMAnimationSceneImageWriter&); // Not implemented.
};


#endif

