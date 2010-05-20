/*=========================================================================

   Program: ParaView
   Module:    pqAnimationSceneImageWriter.h

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
#ifndef __pqAnimationSceneImageWriter_h
#define __pqAnimationSceneImageWriter_h

#include "vtkSMAnimationSceneImageWriter.h"
#include "pqCoreExport.h"

/// pqAnimationSceneImageWriter is a subclass of vtkSMAnimationSceneImageWriter
/// which knows how to generate images from a plot view modules.
class PQCORE_EXPORT pqAnimationSceneImageWriter : public vtkSMAnimationSceneImageWriter
{
public:
  static pqAnimationSceneImageWriter* New();
  vtkTypeMacro(pqAnimationSceneImageWriter, vtkSMAnimationSceneImageWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  pqAnimationSceneImageWriter();
  ~pqAnimationSceneImageWriter();

  // Description:
  // Captures the view from the given module and
  // returns a new Image data object. May return NULL.
  // Default implementation can only handle vtkSMViewProxy subclasses.
  // Subclassess must override to handle other types of view modules.
  virtual vtkImageData* CaptureViewImage(
    vtkSMViewProxy*, int magnification);

private:
  pqAnimationSceneImageWriter(const pqAnimationSceneImageWriter&); // Not implemented.
  void operator=(const pqAnimationSceneImageWriter&); // Not implemented.
};


#endif

