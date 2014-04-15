/*=========================================================================

   Program: ParaView
   Module:    vtkPVRenderViewWithEDL.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef __vtkPVRenderViewForAssembly_h
#define __vtkPVRenderViewForAssembly_h

#include "vtkPVRenderView.h"
#include "vtkSmartPointer.h"
#include <sstream>

class vtkPVDataRepresentation;

class vtkPVRenderViewForAssembly : public vtkPVRenderView
{
public:
  static vtkPVRenderViewForAssembly* New();
  vtkTypeMacro(vtkPVRenderViewForAssembly, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the view with an identifier. Unless noted otherwise, this method
  // must be called before calling any other methods on this class.
  // @CallOnAllProcessess
  virtual void Initialize(unsigned int id);

  // Description:
  // Set the geometry bounds to use. If set to a valid value, these are the
  // bounds used to setup the clipping planes.
  void SetClippingBounds( double b1, double b2, double b3,
                          double b4, double b5, double b6 );
  void SetClippingBounds(double bds[6])
    {
    this->ClippingBounds.SetBounds(bds);
    }
  void ResetClippingBounds();
  void FreezeGeometryBounds();

  // Description:
  // This will trigger multiple renders based on the current scene composition
  // and will create a single string which will encode the ordering.
  void ComputeZOrdering();

  // Description:
  // Return an encoded string that represent the objects ordering for each pixel.
  // And NULL if no ComputeZOrdering() was done before.
  const char* GetZOrdering();

  // Description:
  // Set/Get directory used when dumping the RGB buffer as image on disk
  vtkSetStringMacro(CompositeDirectory)
  vtkGetStringMacro(CompositeDirectory)

  // Description:
  // Dump RGB buffer to the disk using the CompositeDirectory (rgb.jpg)
  void WriteImage();
  // Description:
  // Set image format type. Can only be 'jpg', 'png', 'tiff' where 'jpg' is
  // the default format.
  vtkGetStringMacro(ImageFormatExtension);
  vtkSetStringMacro(ImageFormatExtension);

  // Description:
  // Set/Get RGB image stack size
  vtkGetMacro(RGBStackSize,int);
  vtkSetMacro(RGBStackSize,int);

  // Description:
  // Reset active image stack to 0 so we can start capturing again
  // from the beginning the RGB buffers into our unique RGB generated image.
  void ResetActiveImageStack();

  // Description:
  // Capture RGB buffer in the proper RGB stack position
  // and increase current active stack position.
  void CaptureActiveRepresentation();

  // Description:
  // Dump composite information as JSON file into CompositeDirectory
  // (composite.json)
  void WriteComposite();

  void AddRepresentationForComposite(vtkPVDataRepresentation* r);
  void RemoveRepresentationForComposite(vtkPVDataRepresentation* r);

  // Description:
  // Return representation encoding code inside the ZOrdering string.
  // Each char map a single Representation index, while 0 is reserved
  // for the background, hence the corresponding char is shifted by 1.
  const char* GetRepresentationCodes();

  void SetActiveRepresentationForComposite(vtkPVDataRepresentation* r);

//BTX
protected:
  vtkPVRenderViewForAssembly();
  ~vtkPVRenderViewForAssembly();

  virtual void Render(bool interactive, bool skip_rendering);
  virtual void ResetCameraClippingRange();

private:
  vtkPVRenderViewForAssembly(const vtkPVRenderViewForAssembly&); // Not implemented
  void operator=(const vtkPVRenderViewForAssembly&); // Not implemented

  bool InRender;

  int ActiveStack;
  int RGBStackSize;
  bool InsideComputeZOrdering;
  bool InsideRGBDump;
  char* CompositeDirectory;
  int OrderingBufferSize;
  char* OrderingBuffer;
  int RepresentationToRender;
  char* ImageFormatExtension;

  vtkBoundingBox ClippingBounds;

  struct vtkInternals;
  vtkInternals* Internal;
//ETX
};

#endif
