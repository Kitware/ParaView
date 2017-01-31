/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClientServerSynchronizedRenderers.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVClientServerSynchronizedRenderers
 *
 * vtkPVClientServerSynchronizedRenderers is similar to
 * vtkClientServerSynchronizedRenderers except that it optionally uses image
 * compressors to compress the image before transmitting.
*/

#ifndef vtkPVClientServerSynchronizedRenderers_h
#define vtkPVClientServerSynchronizedRenderers_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkSynchronizedRenderers.h"

class vtkImageCompressor;
class vtkUnsignedCharArray;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVClientServerSynchronizedRenderers
  : public vtkSynchronizedRenderers
{
public:
  static vtkPVClientServerSynchronizedRenderers* New();
  vtkTypeMacro(vtkPVClientServerSynchronizedRenderers, vtkSynchronizedRenderers);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // Descritpion:
  // This flag is set by the renderer during still renderers. When set
  // compressor must use loss-less compression. When unset compressor
  // can (if it's enabled) use lossy compression.
  vtkSetMacro(LossLessCompression, bool);
  vtkGetMacro(LossLessCompression, bool);

  /**
   * Set and configure a compressor from it's own configuration stream. This
   * is used by ParaView to configure the compressor from application wide
   * user settings.
   */
  virtual void ConfigureCompressor(const char* stream);

protected:
  vtkPVClientServerSynchronizedRenderers();
  ~vtkPVClientServerSynchronizedRenderers();

  /**
   * Overridden to not clear the color buffer before pasting back image from
   * the server. This ensures that any annotations rendered on the back of any
   * 3D geometry will be preserved.
   */
  virtual void PushImageToScreen() VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the compressor object, it's setting can be manipulated directly.
   */
  void SetCompressor(vtkImageCompressor* comp);
  vtkGetObjectMacro(Compressor, vtkImageCompressor);
  //@}

  vtkUnsignedCharArray* Compress(vtkUnsignedCharArray*);
  void Decompress(vtkUnsignedCharArray* input, vtkUnsignedCharArray* outputBuffer);

  virtual void MasterEndRender() VTK_OVERRIDE;
  virtual void SlaveStartRender() VTK_OVERRIDE;
  virtual void SlaveEndRender() VTK_OVERRIDE;

  vtkImageCompressor* Compressor;
  bool LossLessCompression;

private:
  vtkPVClientServerSynchronizedRenderers(
    const vtkPVClientServerSynchronizedRenderers&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVClientServerSynchronizedRenderers&) VTK_DELETE_FUNCTION;
};

#endif
