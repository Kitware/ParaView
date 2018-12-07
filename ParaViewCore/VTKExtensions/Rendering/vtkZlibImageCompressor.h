/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkZlibImageCompressor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkZlibImageCompressor
 * @brief   Image compressor/decompressor using Zlib.
 *
 * This class compresses Image data using Zlib. The compression level
 * varies between 1 and 9, 1 being the fastest at the cost of the
 * compression ratio, 9 producing the highest compression ratio at the
 * cost of speed. Optionally color depth may be reduced and alpha
 * stripped/restored.
 * @par Thanks:
 * SciberQuest Inc. contributed this class.
*/

#ifndef vtkZlibImageCompressor_h
#define vtkZlibImageCompressor_h

#include "vtkImageCompressor.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class vtkZlibCompressorImageConditioner;
class vtkMultiProcessStream;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkZlibImageCompressor : public vtkImageCompressor
{
public:
  static vtkZlibImageCompressor* New();
  vtkTypeMacro(vtkZlibImageCompressor, vtkImageCompressor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  //@}

  const char* SaveConfiguration() override;
  const char* RestoreConfiguration(const char* stream) override;

  //@{
  /**
   * Set compression level. A setting of 1 is the fastest producing the
   * smallest compression ratio while a setting of 9 is the slowest producing
   * the highest compression ratio. Zlib is loss-less regardless of level
   * however, setting SetColorSpaceReduction factor to a non zero value
   * will cause internal pre-processor to reduce the color space prior to
   * compression which can improve compression ratio realized.
   */
  vtkSetClampMacro(CompressionLevel, int, 1, 9);
  vtkGetMacro(CompressionLevel, int);
  //@}

  //@{
  /**
   * Set to an integer between 0 and 5. This uses the same color space reduction
   * as the squirt compressor. If set to 0 no colorspace reduction is performed.
   */
  void SetColorSpace(int csId);
  int GetColorSpace();
  //@}

  //@{
  /**
   * Set to boolean value indicating whether alpha values
   * should be stripped prior to compression. Stripping alpha values will reduce
   * input to compressor by 1/4 and results in speed up in compressor run time
   * and of course reduced image size. Stripped alpha value are reinstated to
   * 0xff during decompress.
   */
  void SetStripAlpha(int status);
  int GetStripAlpha();
  //@}

  /**
   * When set the implementation must use loss-less compression, otherwise
   * implemnetation should user provided settings.
   */
  void SetLossLessMode(int mode) override;

protected:
  vtkZlibImageCompressor();
  ~vtkZlibImageCompressor() override;

private:
  vtkZlibCompressorImageConditioner* Conditioner; // manages color space reduction and strip alpha
  int CompressionLevel;                           // zlib compression level

private:
  vtkZlibImageCompressor(const vtkZlibImageCompressor&) = delete;
  void operator=(const vtkZlibImageCompressor&) = delete;
};

#endif
