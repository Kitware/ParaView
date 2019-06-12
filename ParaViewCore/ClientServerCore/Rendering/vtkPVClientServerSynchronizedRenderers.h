/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClientServerSynchronizedRenderers.h

  Copyright (c) Kitware, Inc.
  Copyright (c) 2017, NVIDIA CORPORATION.
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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // This flag is set by the renderer during still renderers. When set
  // compressor must use loss-less compression. When unset compressor
  // can (if it's enabled) use lossy compression.
  vtkSetMacro(LossLessCompression, bool);
  vtkGetMacro(LossLessCompression, bool);

  // Description:
  // This flag is set when NVPipe is supported.  NVPipe may not be available
  // even when compiled in, if the system is not using an NVIDIA GPU, for
  // example.
  vtkSetMacro(NVPipeSupport, bool);
  vtkGetMacro(NVPipeSupport, bool);

  /**
   * Set and configure a compressor from it's own configuration stream. This
   * is used by ParaView to configure the compressor from application wide
   * user settings.
   */
  virtual void ConfigureCompressor(const char* stream);

protected:
  vtkPVClientServerSynchronizedRenderers();
  ~vtkPVClientServerSynchronizedRenderers() override;

  //@{
  /**
   * Set/Get the compressor object, it's setting can be manipulated directly.
   */
  void SetCompressor(vtkImageCompressor* comp);
  vtkGetObjectMacro(Compressor, vtkImageCompressor);
  //@}

  vtkUnsignedCharArray* Compress(vtkUnsignedCharArray*);
  void Decompress(vtkUnsignedCharArray* input, vtkUnsignedCharArray* outputBuffer);

  void MasterEndRender() override;
  void SlaveEndRender() override;

  vtkImageCompressor* Compressor;
  bool LossLessCompression;
  bool NVPipeSupport;

private:
  vtkPVClientServerSynchronizedRenderers(const vtkPVClientServerSynchronizedRenderers&) = delete;
  void operator=(const vtkPVClientServerSynchronizedRenderers&) = delete;
};

#endif
