/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNvPipeCompressor.h

  Copyright (c) 2016-2017, NVIDIA CORPORATION.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkNvPipeCompressor - Image compressor/decompressor using NvPipe.
// .SECTION Description
// This class compresses image data using NvPipe, a library that takes advantage
// of NVIDIA GPU's hardware-based video [de]compression hardware.
// .SECTION Thanks
// NVIDIA CORPORATION contributed this class.

#ifndef vtkNvPipeCompressor_h
#define vtkNvPipeCompressor_h

#include "vtkImageCompressor.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class vtkMultiProcessStream;
class vtkInformationIntegerKey;
typedef void nvpipe;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkNvPipeCompressor : public vtkImageCompressor
{
public:
  static vtkNvPipeCompressor* New();
  vtkTypeMacro(vtkNvPipeCompressor, vtkImageCompressor);
  void PrintSelf(std::ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set the quality measure. The value can be between 0 and 5. 0 means
   * preserve input image quality while 5 emphasizes compression ratio at the
   * expense of image quality.
   */
  vtkSetClampMacro(Quality, unsigned int, 0, 5);
  vtkGetMacro(Quality, unsigned int);
  //@}

  //@{
  // Description:
  // Compress/Decompress data array on the objects input with results
  // in the objects output. See also Set/GetInput/Output.
  virtual int Compress();
  virtual int Decompress();
  //@}

  void SetImageResolution(int img_width, int img_height);

  //@{
  /// Description:
  /// Serialize/Restore compressor configuration (but not the data) into the stream.
  virtual void SaveConfiguration(vtkMultiProcessStream* stream);
  virtual bool RestoreConfiguration(vtkMultiProcessStream* stream);
  virtual const char* SaveConfiguration();
  virtual const char* RestoreConfiguration(const char* stream);
  //@}

  /// Queries the system to see if NVIDIA cards are present to back this
  /// implementation.
  static bool Available();

  static vtkInformationIntegerKey* PIXELS_SKIPPED();

protected:
  vtkNvPipeCompressor();
  virtual ~vtkNvPipeCompressor();

  unsigned int Quality;

private:
  size_t Width;
  size_t Height;
  nvpipe* Pipe;
  uint64_t Bitrate;

private:
  vtkNvPipeCompressor(const vtkNvPipeCompressor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkNvPipeCompressor&) VTK_DELETE_FUNCTION;
};
#endif
