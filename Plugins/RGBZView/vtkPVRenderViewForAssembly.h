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

class vtkImageData;

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

  vtkImageData* GetRGBAData();
  vtkImageData* GetZBufferData();

  // Description:
  // Set/Get the name that should be used to save the ImageData that will contains
  // RGB and Z buffer information. If Null, the data won't be written to disk.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

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
  vtkBoundingBox ClippingBounds;
  vtkSmartPointer<vtkImageData> RGBAData;
  vtkSmartPointer<vtkImageData> ZBufferData;
  char* FileName;
//ETX
};

#endif
