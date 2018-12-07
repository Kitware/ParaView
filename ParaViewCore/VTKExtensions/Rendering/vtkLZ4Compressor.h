/*=========================================================================

  Program:   ParaView
  Module:    vtkLZ4Compressor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLZ4Compressor
 * @brief   Image compressor/decompressor
 * that uses LZ4 for fast lossless compression.
 *
 * vtkLZ4Compressor uses LZ4 for fast lossless compression and decompression on
 * data.
*/

#ifndef vtkLZ4Compressor_h
#define vtkLZ4Compressor_h

#include "vtkImageCompressor.h"
#include "vtkNew.h"                            // needed for vtkNew
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for exports

class vtkMultiProcessStream;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkLZ4Compressor : public vtkImageCompressor
{
public:
  static vtkLZ4Compressor* New();
  vtkTypeMacro(vtkLZ4Compressor, vtkImageCompressor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the quality measure. The value can be between 0 and 5. 0 means preserve
   * input image quality while 5 means improve compression at the cost of image
   * quality. For quality values  > 1, we use a color mask on the input colors
   * similar to vtkSquirtCompressor.
   */
  vtkSetClampMacro(Quality, int, 0, 5);
  vtkGetMacro(Quality, int);
  //@}

  //@{
  /**
   * Compress/Decompress data array on the objects input with results
   * in the objects output. See also Set/GetInput/Output.
   */
  int Compress() override;
  int Decompress() override;
  //@}

  //@{
  /**
   * Serialize/Restore compressor configuration (but not the data) into the stream.
   */
  void SaveConfiguration(vtkMultiProcessStream* stream) override;
  bool RestoreConfiguration(vtkMultiProcessStream* stream) override;
  const char* SaveConfiguration() override;
  const char* RestoreConfiguration(const char* stream) override;
  //@}

protected:
  vtkLZ4Compressor();
  ~vtkLZ4Compressor() override;

  int Quality;

private:
  vtkLZ4Compressor(const vtkLZ4Compressor&) = delete;
  void operator=(const vtkLZ4Compressor&) = delete;

  // Used when Quality > 1.
  vtkNew<vtkUnsignedCharArray> TemporaryBuffer;
};

#endif
