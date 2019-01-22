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
/**
 * @class   vtkPVWebApplication
 * @brief   defines ParaViewWeb application interface.
 *
 * vtkPVWebApplication defines the core interface for a ParaViewWeb application.
 * This exposes methods that make it easier to manage views and rendered images
 * from views.
*/

#ifndef vtkPVWebApplication_h
#define vtkPVWebApplication_h

#include "vtkObject.h"
#include "vtkPVWebCoreModule.h" // needed for exports

class vtkUnsignedCharArray;
class vtkSMViewProxy;
class vtkWebInteractionEvent;

class VTKPVWEBCORE_EXPORT vtkPVWebApplication : public vtkObject
{
public:
  static vtkPVWebApplication* New();
  vtkTypeMacro(vtkPVWebApplication, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the encoding to be used for rendered images.
   */
  enum
  {
    ENCODING_NONE = 0,
    ENCODING_BASE64 = 1
  };
  vtkSetClampMacro(ImageEncoding, int, ENCODING_NONE, ENCODING_BASE64);
  vtkGetMacro(ImageEncoding, int);
  //@}

  //@{
  /**
   * Set the compression to be used for rendered images.
   */
  enum
  {
    COMPRESSION_NONE = 0,
    COMPRESSION_PNG = 1,
    COMPRESSION_JPEG = 2
  };
  vtkSetClampMacro(ImageCompression, int, COMPRESSION_NONE, COMPRESSION_JPEG);
  vtkGetMacro(ImageCompression, int);
  //@}

  //@{
  /**
   * Render a view and obtain the rendered image.
   */
  vtkUnsignedCharArray* StillRender(vtkSMViewProxy* view, int quality = 100);
  vtkUnsignedCharArray* InteractiveRender(vtkSMViewProxy* view, int quality = 50);
  const char* StillRenderToString(vtkSMViewProxy* view, unsigned long time = 0, int quality = 100);
  vtkUnsignedCharArray* StillRenderToBuffer(
    vtkSMViewProxy* view, unsigned long time = 0, int quality = 100);
  //@}

  /**
   * StillRenderToString() need not necessary returns the most recently rendered
   * image. Use this method to get whether there are any pending images being
   * processed concurrently.
   */
  bool GetHasImagesBeingProcessed(vtkSMViewProxy*);

  /**
   * Communicate mouse interaction to a view.
   * Returns true if the interaction changed the view state, otherwise returns false.
   */
  bool HandleInteractionEvent(vtkSMViewProxy* view, vtkWebInteractionEvent* event);

  /**
   * Invalidate view cache
   */
  void InvalidateCache(vtkSMViewProxy* view);

  //@{
  /**
   * Return the MTime of the last array exported by StillRenderToString, StillRenderToBuffer.
   */
  vtkGetMacro(LastStillRenderToMTime, vtkMTimeType);
  //@}

  /**
   * Return the Meta data description of the input scene in JSON format.
   * This is using the vtkWebGLExporter to parse the scene.
   * NOTE: This should be called before getting the webGL binary data.
   */
  const char* GetWebGLSceneMetaData(vtkSMViewProxy* view);

  /**
   * Return the binary data given the part index
   * and the webGL object piece id in the scene.
   */
  const char* GetWebGLBinaryData(vtkSMViewProxy* view, const char* id, int partIndex);

  //@{
  /**
   * Return the size of the last image exported.
   */
  vtkGetVector2Macro(LastStillRenderImageSize, int);
  //@}

protected:
  vtkPVWebApplication();
  ~vtkPVWebApplication();

  int ImageEncoding;
  int ImageCompression;
  vtkMTimeType LastStillRenderToMTime;
  int LastStillRenderImageSize[3];

private:
  vtkPVWebApplication(const vtkPVWebApplication&) = delete;
  void operator=(const vtkPVWebApplication&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
