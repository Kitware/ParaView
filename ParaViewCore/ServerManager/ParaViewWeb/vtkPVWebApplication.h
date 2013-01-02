/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVWebApplication - defines ParaViewWeb application interface.
// .SECTION Description
// vtkPVWebApplication defines the core interface for a ParaViewWeb application.
// This exposes methods that make it easier to manage views and rendered images
// from views.

#ifndef __vtkPVWebApplication_h
#define __vtkPVWebApplication_h

#include "vtkObject.h"
#include "vtkParaViewWebModule.h" // needed for exports

class vtkUnsignedCharArray;
class vtkSMViewProxy;
class vtkPVWebInteractionEvent;

class VTKPARAVIEWWEB_EXPORT vtkPVWebApplication : public vtkObject
{
public:
  static vtkPVWebApplication* New();
  vtkTypeMacro(vtkPVWebApplication, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the encoding to be used for rendered images.
  enum
    {
    ENCODING_NONE=0,
    ENCODING_BASE64=1
    };
  vtkSetClampMacro(ImageEncoding, int, ENCODING_NONE, ENCODING_BASE64);
  vtkGetMacro(ImageEncoding, int);

  // Description:
  // Set the compression to be used for rendered images.
  enum
    {
    COMPRESSION_NONE=0,
    COMPRESSION_PNG=1,
    COMPRESSION_JPEG=2
    };
  vtkSetClampMacro(ImageCompression, int, COMPRESSION_NONE, COMPRESSION_JPEG);
  vtkGetMacro(ImageCompression, int);

  // Description:
  // Render a view and obtain the rendered image.
  vtkUnsignedCharArray* StillRender(vtkSMViewProxy* view);
  vtkUnsignedCharArray* InteractiveRender(vtkSMViewProxy* view);
  const char* StillRenderToString(vtkSMViewProxy* view);

  // Description:
  // StillRenderToString() need not necessary returns the most recently rendered
  // image. Use this method to get whether there are any pending images being
  // processed concurrently.
  bool GetHasImagesBeingProcessed(vtkSMViewProxy*);

  // Description:
  // Communicate mouse interaction to a view.
  // Returns true if the interaction changed the view state, otherwise returns false.
  bool HandleInteractionEvent(
    vtkSMViewProxy* view, vtkPVWebInteractionEvent* event);

//BTX
protected:
  vtkPVWebApplication();
  ~vtkPVWebApplication();

  int ImageEncoding;
  int ImageCompression;

private:
  vtkPVWebApplication(const vtkPVWebApplication&); // Not implemented
  void operator=(const vtkPVWebApplication&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;

//ETX
};

#endif
