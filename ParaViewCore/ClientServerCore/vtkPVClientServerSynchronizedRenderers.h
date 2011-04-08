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
// .NAME vtkPVClientServerSynchronizedRenderers
// .SECTION Description
// vtkPVClientServerSynchronizedRenderers is similar to
// vtkClientServerSynchronizedRenderers except that it optionally uses image
// compressors to compress the image before transmitting.

#ifndef __vtkPVClientServerSynchronizedRenderers_h
#define __vtkPVClientServerSynchronizedRenderers_h

#include "vtkSynchronizedRenderers.h"

class vtkImageCompressor;
class vtkUnsignedCharArray;

class VTK_EXPORT vtkPVClientServerSynchronizedRenderers : public vtkSynchronizedRenderers
{
public:
  static vtkPVClientServerSynchronizedRenderers* New();
  vtkTypeMacro(vtkPVClientServerSynchronizedRenderers, vtkSynchronizedRenderers);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Descritpion:
  // This flag is set by the renderer during still renderers. When set
  // compressor must use loss-less compression. When unset compressor
  // can (if it's enabled) use lossy compression.
  vtkSetMacro(LossLessCompression, bool);
  vtkGetMacro(LossLessCompression, bool);

  // Description:
  // Set and configure a compressor from it's own configuration stream. This
  // is used by ParaView to configure the compressor from application wide
  // user settings.
  virtual void ConfigureCompressor(const char *stream);

//BTX
protected:
  vtkPVClientServerSynchronizedRenderers();
  ~vtkPVClientServerSynchronizedRenderers();

  // Description:
  // Set/Get the compressor object, it's setting can be manipulated directly.
  void SetCompressor(vtkImageCompressor *comp);
  vtkGetObjectMacro(Compressor,vtkImageCompressor);

  vtkUnsignedCharArray* Compress(vtkUnsignedCharArray*);
  void Decompress(vtkUnsignedCharArray* input, vtkUnsignedCharArray* outputBuffer);

  virtual void MasterEndRender();
  virtual void SlaveEndRender();

  vtkImageCompressor* Compressor;
  bool LossLessCompression;
private:
  vtkPVClientServerSynchronizedRenderers(const vtkPVClientServerSynchronizedRenderers&); // Not implemented
  void operator=(const vtkPVClientServerSynchronizedRenderers&); // Not implemented
//ETX
};

#endif
