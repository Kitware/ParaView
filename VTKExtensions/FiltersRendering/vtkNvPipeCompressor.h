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
#include "vtkPVVTKExtensionsFiltersRenderingModule.h" // needed for export macro

class vtkMultiProcessStream;
typedef void nvpipe;

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkNvPipeCompressor : public vtkImageCompressor
{
public:
  static vtkNvPipeCompressor* New();
  vtkTypeMacro(vtkNvPipeCompressor, vtkImageCompressor);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the quality measure. The value can be between 1 and 5. 1 means
   * preserve input image quality while 5 emphasizes compression ratio at the
   * expense of image quality.
   */
  vtkSetClampMacro(Quality, unsigned int, 1, 5);
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
  vtkNvPipeCompressor(const vtkNvPipeCompressor&) = delete;
  void operator=(const vtkNvPipeCompressor&) = delete;
};
#endif
