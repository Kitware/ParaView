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

class vtkImageWriter;
class vtkGenericMovieWriter;
class vtkImageData;

class VTK_EXPORT vtkSMAnimationSceneImageWriter : public vtkSMAnimationSceneWriter
{
public:
  static vtkSMAnimationSceneImageWriter* New();
  vtkTypeRevisionMacro(vtkSMAnimationSceneImageWriter, vtkSMAnimationSceneWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the magnification factor to use for the saved animation.
  vtkSetClampMacro(Magnification, int, 1, VTK_INT_MAX);
  vtkGetMacro(Magnification, int);

  // Description:
  // Get/Set the quality for the generated movie.
  // Applicable only if the choose file format supports it.
  vtkSetMacro(Quality, int);
  vtkGetMacro(Quality, int);

  // Description:
  // Get the error code which is set if there's an error while writing
  // the images.
  vtkGetMacro(ErrorCode, int);


  // Description:
  // Get/Set the RGB background color to use to fill empty spaces in the image.
  // RGB components are in the range [0,1].
  vtkSetVector3Macro(BackgroundColor, double);
  vtkGetVector3Macro(BackgroundColor, double);

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

  vtkImageData* NewFrame();
  void Merge(vtkImageData* dest, vtkImageData* src);

  vtkSetVector2Macro(ActualSize, int);
  int ActualSize[2];
  int Quality;
  int Magnification;
  int FileCount;
  int ErrorCode;
  
  double BackgroundColor[3];

  vtkImageWriter* ImageWriter;
  vtkGenericMovieWriter* MovieWriter;

  void SetImageWriter(vtkImageWriter*);
  void SetMovieWriter(vtkGenericMovieWriter*);
private:
  vtkSMAnimationSceneImageWriter(const vtkSMAnimationSceneImageWriter&); // Not implemented.
  void operator=(const vtkSMAnimationSceneImageWriter&); // Not implemented.
};


#endif

