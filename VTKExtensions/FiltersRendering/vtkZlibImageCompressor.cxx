/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkZlibImageCompressor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkZlibImageCompressor.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"
#include "vtk_zlib.h"
#include <sstream>

vtkStandardNewMacro(vtkZlibImageCompressor);

//=============================================================================
class vtkZlibCompressorImageConditioner
{
public:
  // Description:
  // Construct with mask set to 0.
  vtkZlibCompressorImageConditioner();
  // Description:
  // Choose a color depth reducing mask, uses same masks
  // as Squirt.
  int GetMaskId() { return this->MaskId; }
  void SetMaskId(int maskId);
  // Description:
  // When set  pre-processing is garaunteed to be
  // loss-less regardless of settings.
  void SetLossLessMode(int mode) { this->LossLessMode = mode; }
  int GetLossLessMode() { return this->LossLessMode; }
  // Description:
  // This is used to control whether or not alpha channel
  // is stripped during the pre-processing (i.e. RGBA -> RGB).
  void SetStripAlpha(int status) { this->StripAlpha = status; }
  int GetStripAlpha() { return this->StripAlpha; }
  // Description:
  // Pre-process the provided image, pre-processed data is return
  // via the "out" parameter. A flag is returned through the "freeOut"
  // parameter indicating whether or not the caller needs to call free
  // on the returned array.
  void PreProcess(vtkUnsignedCharArray* in, unsigned char*& out, int& nCompsOut, vtkIdType& outSize,
    int& freeOut);
  // Description:
  // Post-process will restore the apha.
  void PostProcess(
    const unsigned char* in, unsigned char const* inEnd, int inComps, vtkUnsignedCharArray* out);
  // Description:
  // Print object state to the given stream.
  void PrintSelf(ostream& os, vtkIndent indent);

private:
  // Description:
  // Apply selected mask to RGB data.
  void MaskRGB(const unsigned char* in, const unsigned char* inEnd, unsigned char* out);
  // Description:
  // Apply selected mask to RGBA data, pass alpha.
  void MaskRGBA(const unsigned char* in, const unsigned char* inEnd, unsigned char* out);
  // Description:
  // Apply selected mask to RGBA data, do not pass alpha.
  void MaskRGBStripA(const unsigned char* in, const unsigned char* inEnd, unsigned char* out);
  // Description:
  // Do not apply the selected mask, copy RGBA, into RGB.
  void CopyRGBStripA(const unsigned char* in, const unsigned char* inEnd, unsigned char* out);
  // Description:
  // Copy RGB and add alpha 0xff
  void CopyRGBRestoreA(const unsigned char* in, const unsigned char* inEnd, unsigned char* out);

private:
  unsigned char Mask[7]; // mask used in color space reduction (lossy)
  int MaskId;            // id of above mask
  int StripAlpha;        // if set RGBA->RGB comversion is performed
  int LossLessMode;      // if set, garauntee loss-less
};

//-----------------------------------------------------------------------------
vtkZlibCompressorImageConditioner::vtkZlibCompressorImageConditioner()
  : MaskId(0)
  , StripAlpha(0)
  , LossLessMode(1)
{
  this->Mask[0] = 0xff; // 24 bpp
  this->Mask[1] = 0xfe; // 21 bpp
  this->Mask[2] = 0xfc; // 18 bpp
  this->Mask[3] = 0xf8; // 15 bpp
  this->Mask[4] = 0xf0; // 12 bpp
  this->Mask[5] = 0xe0; //  9 bpp
  this->Mask[6] = 0xc0; //  6 bpp
}

//-----------------------------------------------------------------------------
void vtkZlibCompressorImageConditioner::SetMaskId(int maskId)
{
  if (maskId < 0 || maskId > 6)
  {
    return;
  }
  this->MaskId = maskId;
  // const unsigned char m=this->Mask[this->MaskId];
  // cerr << "Mask=" << hex << (int)m << endl;
}

//-----------------------------------------------------------------------------
inline void vtkZlibCompressorImageConditioner::MaskRGB(
  const unsigned char* in, const unsigned char* inEnd, unsigned char* out)
{
  // Mask rgb no alpha
  const unsigned char m = this->Mask[this->MaskId];
  for (; in < inEnd; in += 3, out += 3)
  {
    out[0] = in[0] & m;
    out[1] = in[1] & m;
    out[2] = in[2] & m;
  }
}

//-----------------------------------------------------------------------------
inline void vtkZlibCompressorImageConditioner::MaskRGBA(
  const unsigned char* in, const unsigned char* inEnd, unsigned char* out)
{
  // Mask rgb and pass alpha
  const unsigned char m[4] = { this->Mask[this->MaskId], this->Mask[this->MaskId],
    this->Mask[this->MaskId], 0xff };
  int im;
  memcpy((void*)&im, (const void*)m, 4);
  const int* iin = (const int*)in;
  const int* iinEnd = (const int*)inEnd;
  int* iout = (int*)out;
  for (; iin < iinEnd; iin += 1, iout += 1)
  {
    *iout = *iin & im;
  }

  // const unsigned char m=this->Mask[this->MaskId];
  // for (;in<inEnd; in+=4, out+=4)
  //   {
  //   out[0]=in[0]&m;
  //   out[1]=in[1]&m;
  //   out[2]=in[2]&m;
  //   out[3]=in[3];
  //   }
}

//-----------------------------------------------------------------------------
inline void vtkZlibCompressorImageConditioner::MaskRGBStripA(
  const unsigned char* in, const unsigned char* inEnd, unsigned char* out)
{
  // Mask rgb and strip alpha
  const unsigned char m = this->Mask[this->MaskId];
  for (; in < inEnd; in += 4, out += 3)
  {
    out[0] = in[0] & m;
    out[1] = in[1] & m;
    out[2] = in[2] & m;
  }
}

//-----------------------------------------------------------------------------
inline void vtkZlibCompressorImageConditioner::CopyRGBStripA(
  const unsigned char* in, const unsigned char* inEnd, unsigned char* out)
{
  // Copy rgb and strip alpha
  for (; in < inEnd; in += 4, out += 3)
  {
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
  }
}

//-----------------------------------------------------------------------------
inline void vtkZlibCompressorImageConditioner::CopyRGBRestoreA(
  const unsigned char* in, const unsigned char* inEnd, unsigned char* out)
{
  for (; in < inEnd; in += 3, out += 4)
  {
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
    out[3] = 0xff;
  }
}

//-----------------------------------------------------------------------------
void vtkZlibCompressorImageConditioner::PreProcess(vtkUnsignedCharArray* input, unsigned char*& out,
  int& nCompsOut, vtkIdType& outSize, int& freeOut)
{
  const unsigned char* in = input->GetPointer(0);
  const int nCompsIn = input->GetNumberOfComponents();
  const vtkIdType nTupsIn = input->GetNumberOfTuples();
  const vtkIdType inSize = nCompsIn * nTupsIn;
  const unsigned char* inEnd = in + inSize;

  const int stripAlpha = this->StripAlpha;
  const int RGBAInput = (nCompsIn == 4);
  const int applyMask = (!this->LossLessMode && this->MaskId);

  if (RGBAInput && stripAlpha && applyMask)
  {
    // mask rgb strip alpha.
    freeOut = 1;
    nCompsOut = 3;
    outSize = nTupsIn * 3;
    out = static_cast<unsigned char*>(malloc(outSize));
    this->MaskRGBStripA(in, inEnd, out);
  }
  else if (RGBAInput && !stripAlpha && applyMask)
  {
    // mask rgb pass alpha.
    freeOut = 1;
    nCompsOut = 4;
    outSize = nTupsIn * 4;
    out = static_cast<unsigned char*>(malloc(outSize));
    this->MaskRGBA(in, inEnd, out);
  }
  else if (RGBAInput && stripAlpha && !applyMask)
  {
    // copy rgb strip alpha.
    freeOut = 1;
    nCompsOut = 3;
    outSize = nTupsIn * 3;
    out = static_cast<unsigned char*>(malloc(outSize));
    this->CopyRGBStripA(in, inEnd, out);
  }
  else if (!RGBAInput && applyMask)
  {
    // mask rgb no alpha
    freeOut = 1;
    nCompsOut = 3;
    outSize = nTupsIn * 3;
    out = static_cast<unsigned char*>(malloc(outSize));
    this->MaskRGB(in, inEnd, out);
  }
  else
  {
    // pass on unmodified
    freeOut = 0;
    nCompsOut = nCompsIn;
    outSize = inSize;
    out = const_cast<unsigned char*>(in);
  }
}

//-----------------------------------------------------------------------------
void vtkZlibCompressorImageConditioner::PostProcess(
  const unsigned char* in, unsigned char const* inEnd, int inComps, vtkUnsignedCharArray* output)
{
  // restore alpha
  const int outComps = output->GetNumberOfComponents();
  const int restoreAlpha = (inComps == 3 && outComps == 4);
  if (restoreAlpha)
  {
    vtkIdType outSize = output->GetNumberOfTuples() * outComps;
    unsigned char* out = (unsigned char*)malloc(outSize);
    this->CopyRGBRestoreA(in, inEnd, out);
    output->SetArray(out, outSize, 0);
  }
}

//-----------------------------------------------------------------------------
void vtkZlibCompressorImageConditioner::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "LossLessMode: " << this->LossLessMode << endl
     << indent << "MaskId: " << this->MaskId << endl
     << indent << "Mask: " << hex << (int)this->Mask[0] << "," << hex << (int)this->Mask[1] << ","
     << hex << (int)this->Mask[2] << "," << hex << (int)this->Mask[3] << "," << hex
     << (int)this->Mask[4] << "," << hex << (int)this->Mask[5] << "," << hex << (int)this->Mask[6]
     << endl
     << indent << "StripAlpha: " << this->StripAlpha << endl;
}

//-----------------------------------------------------------------------------
vtkZlibImageCompressor::vtkZlibImageCompressor()
  : Conditioner(nullptr)
  , CompressionLevel(1)
{
  this->Conditioner = new vtkZlibCompressorImageConditioner;
  this->Conditioner->SetMaskId(0);
  this->Conditioner->SetStripAlpha(0);
}

//-----------------------------------------------------------------------------
vtkZlibImageCompressor::~vtkZlibImageCompressor()
{
  delete this->Conditioner;
}

//-----------------------------------------------------------------------------
void vtkZlibImageCompressor::SetLossLessMode(int mode)
{
  this->LossLessMode = mode;
  this->Conditioner->SetLossLessMode(mode);
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkZlibImageCompressor::GetColorSpace()
{
  return this->Conditioner->GetMaskId();
}

//-----------------------------------------------------------------------------
void vtkZlibImageCompressor::SetColorSpace(int csId)
{
  if (csId < 0 || csId > 5)
  {
    vtkWarningMacro(<< "Invalid ColorSpace " << csId << "."
                    << "The valid range is [0 5].");
    return;
  }
  this->Conditioner->SetMaskId(csId);
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkZlibImageCompressor::GetStripAlpha()
{
  return this->Conditioner->GetStripAlpha();
}

//-----------------------------------------------------------------------------
void vtkZlibImageCompressor::SetStripAlpha(int status)
{
  this->Conditioner->SetStripAlpha(status);
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkZlibImageCompressor::Compress()
{
  if (!(this->Input && this->Output))
  {
    vtkWarningMacro("Cannot compress empty input or output detected.");
    return VTK_ERROR;
  }

  // Reduce color space and strip alpha if requested.
  unsigned char* inImage;
  int freeInImage;
  vtkIdType inImageSize;
  int inImageComps;
  this->Conditioner->PreProcess(this->Input, inImage, inImageComps, inImageSize, freeInImage);

  // Compress
  uLongf outImageSize = static_cast<uLongf>(1.001 * inImageSize + 17);
  // zlib requires 100.1% + 16, 1 byte for strip alpha
  unsigned char* outImage = static_cast<unsigned char*>(malloc(outImageSize));
  outImage[0] = inImageComps;
  compress2((Bytef*)(outImage + 1), &outImageSize, (const Bytef*)inImage, inImageSize,
    this->CompressionLevel);

  // Package compressed data in a vtk object.
  this->Output->SetArray(outImage, outImageSize + 1, 0);
  this->Output->SetNumberOfComponents(1);
  this->Output->SetNumberOfTuples(outImageSize + 1);

  // Clean up after pre-proccesosor.
  if (freeInImage)
  {
    free(inImage);
  }

  return VTK_OK;
}

//-----------------------------------------------------------------------------
int vtkZlibImageCompressor::Decompress()
{
  if (!(this->Input && this->Output))
  {
    vtkWarningMacro("Cannot decompress empty input or output detected.");
    return VTK_ERROR;
  }

  // size input.
  unsigned char* compIm = this->Input->GetPointer(1);
  const vtkIdType compImSize = this->Input->GetNumberOfTuples() - 1;

  // decompress.
  uLongf decompImSize =
    static_cast<uLongf>(this->Output->GetNumberOfComponents() * this->Output->GetNumberOfTuples());
  unsigned char* decompIm = this->Output->GetPointer(0);
  uncompress((Bytef*)decompIm, &decompImSize, (const Bytef*)compIm, compImSize);

  // undo pre-proccssing.
  const int decompImComps = (this->GetStripAlpha() ? 3 : 4);
  unsigned char const* decompImEnd = decompIm + decompImSize;
  this->Conditioner->PostProcess(decompIm, decompImEnd, decompImComps, this->Output);

  return VTK_OK;
}

//-----------------------------------------------------------------------------
void vtkZlibImageCompressor::SaveConfiguration(vtkMultiProcessStream* stream)
{
  vtkImageCompressor::SaveConfiguration(stream);
  *stream << this->CompressionLevel << this->GetColorSpace() << this->GetStripAlpha();
}

//-----------------------------------------------------------------------------
bool vtkZlibImageCompressor::RestoreConfiguration(vtkMultiProcessStream* stream)
{
  if (vtkImageCompressor::RestoreConfiguration(stream))
  {
    int colorSpace;
    int stripAlpha;
    *stream >> this->CompressionLevel >> colorSpace >> stripAlpha;
    this->SetColorSpace(colorSpace);
    this->SetStripAlpha(stripAlpha);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
const char* vtkZlibImageCompressor::SaveConfiguration()
{
  std::ostringstream oss;
  oss << vtkImageCompressor::SaveConfiguration() << " " << this->CompressionLevel << " "
      << this->GetColorSpace() << " " << this->GetStripAlpha();

  this->SetConfiguration(oss.str().c_str());

  return this->Configuration;
}

//-----------------------------------------------------------------------------
const char* vtkZlibImageCompressor::RestoreConfiguration(const char* stream)
{
  stream = vtkImageCompressor::RestoreConfiguration(stream);
  if (stream)
  {
    std::istringstream iss(stream);
    int colorSpace;
    int stripAlpha;
    iss >> this->CompressionLevel >> colorSpace >> stripAlpha;
    this->SetColorSpace(colorSpace);
    this->SetStripAlpha(stripAlpha);
    return stream + iss.tellg();
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void vtkZlibImageCompressor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CompressionLevel: " << this->CompressionLevel << endl;

  this->Conditioner->PrintSelf(os, indent.GetNextIndent());
}
