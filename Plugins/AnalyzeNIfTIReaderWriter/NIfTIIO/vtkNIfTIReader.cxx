/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNIfTIReader.cxx

  Copyright (c) Joseph Hennessey
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkNIfTIReader.h"

#include "ThirdParty/vtknifti1.h"
#include "ThirdParty/vtknifti1_io.h"
#include "ThirdParty/vtkznzlib.h"
#include "vtkByteSwap.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtk_zlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "vtkStringArray.h"
#define NAME_ARRAY "Name"
#define DEFAULT_NAME ""

#include "vtksys/FStream.hxx"

vtkStandardNewMacro(vtkNIfTIReader);

//----------------------------------------------------------------------------
vtkNIfTIReader::vtkNIfTIReader()
{
  q = new double*[4];
  s = new double*[4];
  int count;
  for (count = 0; count < 4; count++)
  {
    q[count] = new double[4];
    s[count] = new double[4];
  }
  this->niftiHeader = nullptr;
  this->niftiHeaderUnsignedCharArray = nullptr;
  this->niftiHeaderSize = 348;
  this->niftiType = 0;
}

//----------------------------------------------------------------------------
vtkNIfTIReader::~vtkNIfTIReader()
{
  int count;
  for (count = 0; count < 4; count++)
  {
    delete[] q[count];
    q[count] = nullptr;
    delete[] s[count];
    s[count] = nullptr;
  }
  delete[] q;
  delete[] s;
  q = nullptr;
  s = nullptr;
  if (this->niftiHeader)
  {
    this->niftiHeader->Delete();
    this->niftiHeader = nullptr;
  }
  if (this->niftiHeaderUnsignedCharArray)
  {
    delete this->niftiHeaderUnsignedCharArray;
    this->niftiHeaderUnsignedCharArray = nullptr;
  }
}

// GetExtension from uiig library.
static std::string GetExtension(const std::string& filename)
{

  // This assumes that the final '.' in a file name is the delimiter
  // for the file's extension type
  const std::string::size_type it = filename.find_last_of('.');

  // This determines the file's type by creating a new std::string
  // who's value is the extension of the input filename
  // eg. "myimage.gif" has an extension of "gif"
  std::string fileExt(filename, it + 1, filename.length());

  return (fileExt);
}

// GetRootName from uiig library.
static std::string GetRootName(const std::string& filename)
{
  const std::string fileExt = GetExtension(filename);

  // Create a base filename
  // i.e Image.hdr --> Image
  if (fileExt.length() > 0)
  {
    const std::string::size_type it = filename.find_last_of(fileExt);
    std::string baseName(filename, 0, it - fileExt.length());
    return (baseName);
  }
  // Default to return same as input when the extension is nothing (Analyze)
  return (filename);
}

static std::string GetHeaderFileName(const std::string& filename)
{
  std::string ImageFileName = GetRootName(filename);
  std::string fileExt = GetExtension(filename);
  // If file was named xxx.img.gz then remove both the gz and the img endings.
  if (!fileExt.compare("gz"))
  {
    ImageFileName = GetRootName(GetRootName(filename));
  }
  else if (!fileExt.compare("nii"))
  {
    ImageFileName += ".nii";
  }
  else if (!fileExt.compare("hdr"))
  {
    ImageFileName += ".hdr";
  }
  else if (!fileExt.compare("img"))
  {
    ImageFileName += ".hdr";
  }
  return (ImageFileName);
}

// Returns the base image filename.
static std::string GetImageFileName(const std::string& filename)
{
  // Why do we add ".nii" here?  Look in fileutils.h
  std::string fileExt = GetExtension(filename);
  std::string ImageFileName = GetRootName(filename);
  if (!fileExt.compare("gz"))
  {
    // First strip both extensions off
    ImageFileName = GetRootName(GetRootName(filename));
    ImageFileName += ".nii.gz";
  }
  else if (!fileExt.compare("nii"))
  {
    ImageFileName += ".nii";
  }
  else if (!fileExt.compare("img"))
  {
    ImageFileName += ".img";
  }
  else if (!fileExt.compare("hdr"))
  {
    ImageFileName += ".img";
  }
  else
  {
    // uiig::Reporter* reporter = uiig::Reporter::getReporter();
    // std::string temp="Error, Can not determine compressed file image name. ";
    // temp+=filename;
    // reporter->setMessage( temp );
    return ("");
  }
  return (ImageFileName);
}

static bool ReadBufferAsBinary(istream& is, void* buffer, unsigned int num)
{

  const unsigned int numberOfBytesToBeRead = num;

  is.read(static_cast<char*>(buffer), numberOfBytesToBeRead);

  const unsigned int numberOfBytesRead = is.gcount();

#ifdef __APPLE_CC__
  // fail() is broken in the Mac. It returns true when reaches eof().
  if (numberOfBytesRead != numberOfBytesToBeRead)
#else
  if ((numberOfBytesRead != numberOfBytesToBeRead) || is.fail())
#endif
  {
    return false; // read failed
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkNIfTIReader::ExecuteInformation()
{

  nifti_image* m_NiftiImage;
  dataTypeSize = 1.0;
  unsigned int numComponents = 1;
  nifti_1_header tempNiftiHeader;
  unsigned char* niftiHeaderUnsignedCharArrayPtr = (unsigned char*)&tempNiftiHeader;

  this->niftiHeaderUnsignedCharArray = new unsigned char[this->niftiHeaderSize];

  CanReadFile(this->GetFileName());

  m_NiftiImage = vtknifti1_io::nifti_image_read(this->GetFileName(), true);
  if (m_NiftiImage == nullptr)
  {
    vtkErrorMacro("Read failed");
    return;
  }

  tempNiftiHeader = vtknifti1_io::nifti_convert_nim2nhdr(m_NiftiImage);

  int count;

  for (count = 0; count < this->niftiHeaderSize; count++)
  {
    this->niftiHeaderUnsignedCharArray[count] = niftiHeaderUnsignedCharArrayPtr[count];
  }

  const int dims = m_NiftiImage->ndim;
  size_t numElts = 1;

  switch (dims)
  {
    case 7:
      numElts *= m_NiftiImage->nw;
      VTK_FALLTHROUGH;
    case 6:
      numElts *= m_NiftiImage->nv;
      VTK_FALLTHROUGH;
    case 5:
      numElts *= m_NiftiImage->nu;
      VTK_FALLTHROUGH;
    case 4:
      numElts *= m_NiftiImage->nt;
      VTK_FALLTHROUGH;
    case 3:
      numElts *= m_NiftiImage->nz;
      VTK_FALLTHROUGH;
    case 2:
      numElts *= m_NiftiImage->ny;
      VTK_FALLTHROUGH;
    case 1:
      numElts *= m_NiftiImage->nx;
      break;
    default:
      numElts = 0;
  }

  Type = m_NiftiImage->datatype;

  switch (Type)
  {
    case DT_BINARY:
      this->SetDataScalarType(VTK_BIT);
      dataTypeSize = 0.125;
      break;
    case DT_UNSIGNED_CHAR:
      this->SetDataScalarTypeToUnsignedChar();
      dataTypeSize = 1;
      break;
    case DT_INT8:
      this->SetDataScalarTypeToSignedChar();
      dataTypeSize = 1;
      break;
    case DT_SIGNED_SHORT:
      this->SetDataScalarTypeToShort();
      dataTypeSize = 2;
      break;
    case DT_UINT16:
      this->SetDataScalarTypeToUnsignedShort();
      dataTypeSize = 2;
      break;
    case DT_SIGNED_INT:
      this->SetDataScalarTypeToInt();
      dataTypeSize = 4;
      break;
    case DT_UINT32:
      this->SetDataScalarTypeToUnsignedInt();
      dataTypeSize = 4;
      break;
    case DT_FLOAT:
      this->SetDataScalarTypeToFloat();
      dataTypeSize = 4;
      break;
    case DT_DOUBLE:
      this->SetDataScalarTypeToDouble();
      dataTypeSize = 8;
      break;
    case DT_INT64:
      this->SetDataScalarType(VTK_LONG);
      dataTypeSize = 8;
      break;
    case DT_UINT64:
      this->SetDataScalarType(VTK_UNSIGNED_LONG);
      dataTypeSize = 8;
      break;
    case DT_RGB:
      this->SetDataScalarTypeToUnsignedChar();
      numComponents = 3;
      dataTypeSize = 3;
      break;
    case DT_RGBA32:
      this->SetDataScalarTypeToUnsignedChar();
      numComponents = 4;
      dataTypeSize = 4;
      break;
    default:
      vtkErrorMacro("cannot handle this NIfTI type yet.");
      break;
  }
  //
  // set up the dimension stuff

  this->SetNumberOfScalarComponents(numComponents);

  width = m_NiftiImage->dim[1];
  height = m_NiftiImage->dim[2];
  depth = m_NiftiImage->dim[3];

  this->DataExtent[0] = 0;
  this->DataExtent[1] = m_NiftiImage->dim[1] - 1;
  this->DataExtent[2] = 0;
  this->DataExtent[3] = m_NiftiImage->dim[2] - 1;
  this->DataExtent[4] = 0;
  this->DataExtent[5] = m_NiftiImage->dim[3] - 1;

  this->DataSpacing[0] = m_NiftiImage->pixdim[1];
  this->DataSpacing[1] = m_NiftiImage->pixdim[2];
  this->DataSpacing[2] = m_NiftiImage->pixdim[3];

  // set origin offset

  qform_code = m_NiftiImage->qform_code;
  sform_code = m_NiftiImage->sform_code;

  int row, col;

  for (row = 0; row < 4; row++)
  {
    for (col = 0; col < 4; col++)
    {
      s[row][col] = m_NiftiImage->sto_xyz.m[row][col];
      q[row][col] = m_NiftiImage->qto_xyz.m[row][col];
    }
  }

  int inDim[3];
  double inOriginOffset[3];
  double flippedOriginOffset[3];
  double outNoFlipOriginOffset[3];
  double outOriginOffset[3];

  int InPlaceFilteredAxes[3];
  int flipAxis[3];

  flipAxis[0] = 0;
  flipAxis[1] = 0;
  flipAxis[2] = 0;

  InPlaceFilteredAxes[0] = 0;
  InPlaceFilteredAxes[1] = 1;
  InPlaceFilteredAxes[2] = 2;

  /*if(sform_code>0){
  inOriginOffset[0] = s[0][3];
  inOriginOffset[1] = s[1][3];
  inOriginOffset[2] = s[2][3];
  } else */
  if (qform_code > 0)
  {
    inOriginOffset[0] = q[0][3];
    inOriginOffset[1] = q[1][3];
    inOriginOffset[2] = q[2][3];
  }
  else
  {
    inOriginOffset[0] = 0.0;
    inOriginOffset[1] = 0.0;
    inOriginOffset[2] = 0.0;
  }

  if (sform_code > 0)
  {
    if (s[0][0] >= 1.0)
    {
      InPlaceFilteredAxes[0] = 0;
      flipAxis[0] = 0;
    }
    else if (s[0][0] <= -1.0)
    {
      InPlaceFilteredAxes[0] = 0;
      flipAxis[0] = 1;
    }
    if (s[0][1] >= 1.0)
    {
      InPlaceFilteredAxes[0] = 1;
      flipAxis[0] = 0;
    }
    else if (s[0][1] <= -1.0)
    {
      InPlaceFilteredAxes[0] = 1;
      flipAxis[0] = 1;
    }
    if (s[0][2] >= 1.0)
    {
      InPlaceFilteredAxes[0] = 2;
      flipAxis[0] = 0;
    }
    else if (s[0][2] <= -1.0)
    {
      InPlaceFilteredAxes[0] = 2;
      flipAxis[0] = 1;
    }
    if (s[1][0] >= 1.0)
    {
      InPlaceFilteredAxes[1] = 0;
      flipAxis[1] = 0;
    }
    else if (s[1][0] <= -1.0)
    {
      InPlaceFilteredAxes[1] = 0;
      flipAxis[1] = 1;
    }
    if (s[1][1] >= 1.0)
    {
      InPlaceFilteredAxes[1] = 1;
      flipAxis[1] = 0;
    }
    else if (s[1][1] <= -1.0)
    {
      InPlaceFilteredAxes[1] = 1;
      flipAxis[1] = 1;
    }
    if (s[1][2] >= 1.0)
    {
      InPlaceFilteredAxes[1] = 2;
      flipAxis[1] = 0;
    }
    else if (s[1][2] <= -1.0)
    {
      InPlaceFilteredAxes[1] = 2;
      flipAxis[1] = 1;
    }
    if (s[2][0] >= 1.0)
    {
      InPlaceFilteredAxes[2] = 0;
      flipAxis[2] = 0;
    }
    else if (s[2][0] <= -1.0)
    {
      InPlaceFilteredAxes[2] = 0;
      flipAxis[2] = 1;
    }
    if (s[2][1] >= 1.0)
    {
      InPlaceFilteredAxes[2] = 1;
      flipAxis[2] = 0;
    }
    else if (s[2][1] <= -1.0)
    {
      InPlaceFilteredAxes[2] = 1;
      flipAxis[2] = 1;
    }
    if (s[2][2] >= 1.0)
    {
      InPlaceFilteredAxes[2] = 2;
      flipAxis[2] = 0;
    }
    else if (s[2][2] <= -1.0)
    {
      InPlaceFilteredAxes[2] = 2;
      flipAxis[2] = 1;
    }
  }
  else if (qform_code > 0)
  {
    if (q[0][0] >= 1.0)
    {
      InPlaceFilteredAxes[0] = 0;
      flipAxis[0] = 0;
    }
    else if (q[0][0] <= -1.0)
    {
      InPlaceFilteredAxes[0] = 0;
      flipAxis[0] = 1;
    }
    if (q[0][1] >= 1.0)
    {
      InPlaceFilteredAxes[0] = 1;
      flipAxis[0] = 0;
    }
    else if (q[0][1] <= -1.0)
    {
      InPlaceFilteredAxes[0] = 1;
      flipAxis[0] = 1;
    }
    if (q[0][2] >= 1.0)
    {
      InPlaceFilteredAxes[0] = 2;
      flipAxis[0] = 0;
    }
    else if (q[0][2] <= -1.0)
    {
      InPlaceFilteredAxes[0] = 2;
      flipAxis[0] = 1;
    }
    if (q[1][0] >= 1.0)
    {
      InPlaceFilteredAxes[1] = 0;
      flipAxis[1] = 0;
    }
    else if (q[1][0] <= -1.0)
    {
      InPlaceFilteredAxes[1] = 0;
      flipAxis[1] = 1;
    }
    if (q[1][1] >= 1.0)
    {
      InPlaceFilteredAxes[1] = 1;
      flipAxis[1] = 0;
    }
    else if (q[1][1] <= -1.0)
    {
      InPlaceFilteredAxes[1] = 1;
      flipAxis[1] = 1;
    }
    if (q[1][2] >= 1.0)
    {
      InPlaceFilteredAxes[1] = 2;
      flipAxis[1] = 0;
    }
    else if (q[1][2] <= -1.0)
    {
      InPlaceFilteredAxes[1] = 2;
      flipAxis[1] = 1;
    }
    if (q[2][0] >= 1.0)
    {
      InPlaceFilteredAxes[2] = 0;
      flipAxis[2] = 0;
    }
    else if (q[2][0] <= -1.0)
    {
      InPlaceFilteredAxes[2] = 0;
      flipAxis[2] = 1;
    }
    if (q[2][1] >= 1.0)
    {
      InPlaceFilteredAxes[2] = 1;
      flipAxis[2] = 0;
    }
    else if (q[2][1] <= -1.0)
    {
      InPlaceFilteredAxes[2] = 1;
      flipAxis[2] = 1;
    }
    if (q[2][2] >= 1.0)
    {
      InPlaceFilteredAxes[2] = 2;
      flipAxis[2] = 0;
    }
    else if (q[2][2] <= -1.0)
    {
      InPlaceFilteredAxes[2] = 2;
      flipAxis[2] = 1;
    }
  }

  if ((InPlaceFilteredAxes[0] == InPlaceFilteredAxes[1]) ||
    (InPlaceFilteredAxes[0] == InPlaceFilteredAxes[2]) ||
    (InPlaceFilteredAxes[1] == InPlaceFilteredAxes[2]))
  {
    flipAxis[0] = 0;
    flipAxis[1] = 0;
    flipAxis[2] = 0;
    InPlaceFilteredAxes[0] = 0;
    InPlaceFilteredAxes[1] = 1;
    InPlaceFilteredAxes[2] = 2;
  }

  for (count = 0; count < 3; count++)
  {
    inDim[count] = (this->DataExtent[(count * 2) + 1] - this->DataExtent[count * 2]) + 1;
  }

  for (count = 0; count < 3; count++)
  {
    if (flipAxis[count])
    {
      flippedOriginOffset[count] = inOriginOffset[count] - inDim[count];
    }
    else
    {
      flippedOriginOffset[count] = inOriginOffset[count];
    }
  }

  for (count = 0; count < 3; count++)
  {
    outOriginOffset[count] = flippedOriginOffset[InPlaceFilteredAxes[count]];
    outNoFlipOriginOffset[count] = inOriginOffset[InPlaceFilteredAxes[count]];
  }

  for (count = 0; count < 3; count++)
  {
    if (qform_code > 0)
    {
      this->DataOrigin[count] = outOriginOffset[count];
      // this->DataOrigin[count]       = outNoFlipOriginOffset[count];
    }
    else
    {
      this->DataOrigin[count] = outNoFlipOriginOffset[count];
    }
  }

  imageSizeInBytes = (int)(numElts * dataTypeSize);

#define LSB_FIRST 1
#define MSB_FIRST 2

  if (m_NiftiImage->byteorder == MSB_FIRST)
  {
    this->SetDataByteOrderToBigEndian();
  }
  else
  {
    this->SetDataByteOrderToLittleEndian();
  }

  this->vtkImageReader::ExecuteInformation();
}

//----------------------------------------------------------------------------
// This function reads in one data of data.
// templated to handle different data types.
template <class OT>
void vtkNIfTIReaderUpdate2(
  vtkNIfTIReader* self, vtkImageData* vtkNotUsed(data), OT* outPtr, long offset)
{
  // unsigned int dim;
  // char * const p = static_cast<char *>(outPtr);
  char* p = (char*)(outPtr);
  // 4 cases to handle
  // 1: given .hdr and image is .img
  // 2: given .nii
  // 3: given .nii.gz
  // 4: given .hdr and image is .img.gz
  //   Special processing needed for this case only
  // NOT NEEDED const std::string fileExt = GetExtension(m_FileName);

  /* Returns proper name for cases 1,2,3 */
  std::string ImageFileName = GetImageFileName(self->GetFileName());
  // NOTE: gzFile operations act just like FILE * operations when the files
  // are not in gzip format.
  // This greatly simplifies the following code, and gzFile types are used
  // everywhere.
  // In addition, it has the added benefit of reading gzip compressed image
  // files that do not have a .gz ending.
  gzFile file_p = ::gzopen(ImageFileName.c_str(), "rb");
  if (file_p == nullptr)
  {
    /* Do a separate check to take care of case #4 */
    ImageFileName += ".gz";
    file_p = ::gzopen(ImageFileName.c_str(), "rb");
    if (file_p == nullptr)
    {
      // vtkErrorMacro( << "File cannot be read");
    }
  }

  // Seek through the file to the correct position, This is only necessary
  // when readin in sub-volumes
  // const long int total_offset = static_cast<long int>(tempX * tempY *
  //                                start_slice * m_dataSize)
  //    + static_cast<long int>(tempX * tempY * total_z * start_time *
  //          m_dataSize);
  // ::gzseek( file_p, total_offset, SEEK_SET );

  // read image in
  ::gzseek(file_p, offset, SEEK_SET);
  ::gzread(file_p, p, self->getImageSizeInBytes());
  gzclose(file_p);
  // SwapBytesIfNecessary( buffer, numberOfPixels );
}

//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkNIfTIReader::ExecuteDataWithInformation(vtkDataObject* output, vtkInformation* outInfo)
{
  vtkImageData* data = this->AllocateOutputData(output, outInfo);

  if (this->UpdateExtentIsEmpty(outInfo, output))
  {
    return;
  }
  if (this->GetFileName() == nullptr)
  {
    vtkErrorMacro(<< "Either a FileName or FilePrefix must be specified.");
    return;
  }

  data->GetPointData()->GetScalars()->SetName("NIfTIImage");

  vtkFieldData* fa = data->GetFieldData();

  if (!fa)
  {
    fa = vtkFieldData::New();
    data->SetFieldData(fa);
    fa->Delete();
    fa = data->GetFieldData();
  }

  vtkDataArray* validDataArray = fa->GetArray(NIFTI_HEADER_ARRAY);
  if (!validDataArray)
  {
    this->niftiHeader = vtkUnsignedCharArray::New();
    this->niftiHeader->SetName(NIFTI_HEADER_ARRAY);
    this->niftiHeader->SetNumberOfValues(this->niftiHeaderSize);
    fa->AddArray(this->niftiHeader);
    validDataArray = fa->GetArray(NIFTI_HEADER_ARRAY);
  }
  this->niftiHeader = vtkUnsignedCharArray::SafeDownCast(validDataArray);

  int count;

  for (count = 0; count < this->niftiHeaderSize; count++)
  {
    this->niftiHeader->SetValue(count, niftiHeaderUnsignedCharArray[count]);
  }

  vtkDataArray* tempVolumeOriginDoubleArray = fa->GetArray(VOLUME_ORIGIN_DOUBLE_ARRAY);
  if (!tempVolumeOriginDoubleArray)
  {
    vtkDoubleArray* volumeOriginDoubleArray = nullptr;
    volumeOriginDoubleArray = vtkDoubleArray::New();
    volumeOriginDoubleArray->SetName(VOLUME_ORIGIN_DOUBLE_ARRAY);
    volumeOriginDoubleArray->SetNumberOfValues(3);
    volumeOriginDoubleArray->SetValue(0, this->DataOrigin[0]);
    volumeOriginDoubleArray->SetValue(1, this->DataOrigin[1]);
    volumeOriginDoubleArray->SetValue(2, this->DataOrigin[2]);
    fa->AddArray(volumeOriginDoubleArray);
    volumeOriginDoubleArray->Delete();
    tempVolumeOriginDoubleArray = fa->GetArray(VOLUME_ORIGIN_DOUBLE_ARRAY);
  }

  vtkDataArray* tempVolumeSpacingDoubleArray = fa->GetArray(VOLUME_SPACING_DOUBLE_ARRAY);
  if (!tempVolumeSpacingDoubleArray)
  {
    vtkDoubleArray* volumeSpacingDoubleArray = nullptr;
    volumeSpacingDoubleArray = vtkDoubleArray::New();
    volumeSpacingDoubleArray->SetName(VOLUME_SPACING_DOUBLE_ARRAY);
    volumeSpacingDoubleArray->SetNumberOfValues(3);
    volumeSpacingDoubleArray->SetValue(0, this->DataSpacing[0]);
    volumeSpacingDoubleArray->SetValue(1, this->DataSpacing[1]);
    volumeSpacingDoubleArray->SetValue(2, this->DataSpacing[2]);
    fa->AddArray(volumeSpacingDoubleArray);
    volumeSpacingDoubleArray->Delete();
    tempVolumeSpacingDoubleArray = fa->GetArray(VOLUME_SPACING_DOUBLE_ARRAY);
  }

  vtkStringArray* nameArray;
  vtkAbstractArray* nameAbstractArray = fa->GetAbstractArray(NAME_ARRAY);
  if (!nameAbstractArray)
  {
    nameArray = vtkStringArray::New();
    nameArray->SetName(NAME_ARRAY);
    nameArray->SetNumberOfValues(1);
    std::string fileName = this->GetFileName();

    // Remove directory part
    size_t position = fileName.rfind('/');
    if (position != std::string::npos)
    {
      fileName.erase(0, position + 1);
    }
    position = fileName.rfind('\\');
    if (position != std::string::npos)
    {
      fileName.erase(0, position + 1);
    }

    nameArray->SetValue(0, fileName);
    fa->AddArray(nameArray);
    nameArray->Delete();
    nameAbstractArray = fa->GetAbstractArray(NAME_ARRAY);
  }
  nameArray = vtkStringArray::SafeDownCast(nameAbstractArray);

  // Call the correct templated function for the output
  void* outPtr;
  long offset = 348;

  if (niftiType == 2)
  {
    offset = 0;
  }

  // Call the correct templated function for the input
  outPtr = data->GetScalarPointer();
  size_t tempDataSize = data->GetPointData()->GetScalars()->GetDataSize();
  size_t tempDataTypeSize = data->GetPointData()->GetScalars()->GetDataTypeSize();
  size_t outPtrSize = tempDataSize * tempDataTypeSize;
  nifti_1_header* niftiPointer = (nifti_1_header*)niftiHeaderUnsignedCharArray;
  offset = (long)(niftiPointer->vox_offset);
  switch (data->GetScalarType())
  {
    vtkTemplateMacro(vtkNIfTIReaderUpdate2(this, data, static_cast<VTK_TT*>(outPtr), offset));
    default:
      vtkErrorMacro(<< "Execute: Unknown data type");
  }

  // start of variables

  unsigned char* outUnsignedCharPtr = (unsigned char*)outPtr;

  int inDim[3];
  int outDim[3];
  int inIndex[3];
  int inStride[3];
  int outStride[3];
  int inIncrements[3];
  int outIncrements[3];
  double inOriginOffset[3];
  double inSpacing[3];
  double outSpacing[3];
  double inOrigin[3];
  double outOrigin[3];
  double inExtent[6];
  double outExtent[6];
  int scalarSize = (int)dataTypeSize;

  int InPlaceFilteredAxes[3];
  long inOffset;
  long charInOffset;
  int flipAxis[3];
  int flipIndex[3];

  flipAxis[0] = 0;
  flipAxis[1] = 0;
  flipAxis[2] = 0;

  InPlaceFilteredAxes[0] = 0;
  InPlaceFilteredAxes[1] = 1;
  InPlaceFilteredAxes[2] = 2;

  inOriginOffset[0] = s[0][3];
  inOriginOffset[1] = s[1][3];
  inOriginOffset[2] = s[2][3];

  double epsilon = 0.0001;

  if (sform_code > 0)
  {
    if ((s[0][0] - 1.0) >= -epsilon)
    {
      InPlaceFilteredAxes[0] = 0;
      flipAxis[0] = 0;
    }
    else if ((s[0][0] + 1.0) <= epsilon)
    {
      InPlaceFilteredAxes[0] = 0;
      flipAxis[0] = 1;
    }
    if ((s[0][1] - 1.0) >= -epsilon)
    {
      InPlaceFilteredAxes[0] = 1;
      flipAxis[0] = 0;
    }
    else if ((s[0][1] + 1.0) <= epsilon)
    {
      InPlaceFilteredAxes[0] = 1;
      flipAxis[0] = 1;
    }
    if ((s[0][2] - 1.0) >= -epsilon)
    {
      InPlaceFilteredAxes[0] = 2;
      flipAxis[0] = 0;
    }
    else if ((s[0][2] + 1.0) <= epsilon)
    {
      InPlaceFilteredAxes[0] = 2;
      flipAxis[0] = 1;
    }
    if ((s[1][0] - 1.0) >= -epsilon)
    {
      InPlaceFilteredAxes[1] = 0;
      flipAxis[1] = 0;
    }
    else if ((s[1][0] + 1.0) <= epsilon)
    {
      InPlaceFilteredAxes[1] = 0;
      flipAxis[1] = 1;
    }
    if ((s[1][1] - 1.0) >= -epsilon)
    {
      InPlaceFilteredAxes[1] = 1;
      flipAxis[1] = 0;
    }
    else if ((s[1][1] + 1.0) <= epsilon)
    {
      InPlaceFilteredAxes[1] = 1;
      flipAxis[1] = 1;
    }
    if ((s[1][2] - 1.0) >= -epsilon)
    {
      InPlaceFilteredAxes[1] = 2;
      flipAxis[1] = 0;
    }
    else if ((s[1][2] + 1.0) <= epsilon)
    {
      InPlaceFilteredAxes[1] = 2;
      flipAxis[1] = 1;
    }
    if ((s[2][0] - 1.0) >= -epsilon)
    {
      InPlaceFilteredAxes[2] = 0;
      flipAxis[2] = 0;
    }
    else if ((s[2][0] + 1.0) <= epsilon)
    {
      InPlaceFilteredAxes[2] = 0;
      flipAxis[2] = 1;
    }
    if ((s[2][1] - 1.0) >= -epsilon)
    {
      InPlaceFilteredAxes[2] = 1;
      flipAxis[2] = 0;
    }
    else if ((s[2][1] + 1.0) <= epsilon)
    {
      InPlaceFilteredAxes[2] = 1;
      flipAxis[2] = 1;
    }
    if ((s[2][2] - 1.0) >= -epsilon)
    {
      InPlaceFilteredAxes[2] = 2;
      flipAxis[2] = 0;
    }
    else if ((s[2][2] + 1.0) <= epsilon)
    {
      InPlaceFilteredAxes[2] = 2;
      flipAxis[2] = 1;
    }
  }
  else if (qform_code > 0)
  {
    if ((q[0][0] - 1.0) >= -epsilon)
    {
      InPlaceFilteredAxes[0] = 0;
      flipAxis[0] = 0;
    }
    else if ((q[0][0] + 1.0) <= epsilon)
    {
      InPlaceFilteredAxes[0] = 0;
      flipAxis[0] = 1;
    }
    if ((q[0][1] - 1.0) >= -epsilon)
    {
      InPlaceFilteredAxes[0] = 1;
      flipAxis[0] = 0;
    }
    else if ((q[0][1] + 1.0) <= epsilon)
    {
      InPlaceFilteredAxes[0] = 1;
      flipAxis[0] = 1;
    }
    if ((q[0][2] - 1.0) >= -epsilon)
    {
      InPlaceFilteredAxes[0] = 2;
      flipAxis[0] = 0;
    }
    else if ((q[0][2] + 1.0) <= epsilon)
    {
      InPlaceFilteredAxes[0] = 2;
      flipAxis[0] = 1;
    }
    if ((q[1][0] - 1.0) >= -epsilon)
    {
      InPlaceFilteredAxes[1] = 0;
      flipAxis[1] = 0;
    }
    else if ((q[1][0] + 1.0) <= epsilon)
    {
      InPlaceFilteredAxes[1] = 0;
      flipAxis[1] = 1;
    }
    if ((q[1][1] - 1.0) >= -epsilon)
    {
      InPlaceFilteredAxes[1] = 1;
      flipAxis[1] = 0;
    }
    else if ((q[1][1] + 1.0) <= epsilon)
    {
      InPlaceFilteredAxes[1] = 1;
      flipAxis[1] = 1;
    }
    if ((q[1][2] - 1.0) >= -epsilon)
    {
      InPlaceFilteredAxes[1] = 2;
      flipAxis[1] = 0;
    }
    else if ((q[1][2] + 1.0) <= epsilon)
    {
      InPlaceFilteredAxes[1] = 2;
      flipAxis[1] = 1;
    }
    if ((q[2][0] - 1.0) >= -epsilon)
    {
      InPlaceFilteredAxes[2] = 0;
      flipAxis[2] = 0;
    }
    else if ((q[2][0] + 1.0) <= epsilon)
    {
      InPlaceFilteredAxes[2] = 0;
      flipAxis[2] = 1;
    }
    if ((q[2][1] - 1.0) >= -epsilon)
    {
      InPlaceFilteredAxes[2] = 1;
      flipAxis[2] = 0;
    }
    else if ((q[2][1] + 1.0) <= epsilon)
    {
      InPlaceFilteredAxes[2] = 1;
      flipAxis[2] = 1;
    }
    if ((q[2][2] - 1.0) >= -epsilon)
    {
      InPlaceFilteredAxes[2] = 2;
      flipAxis[2] = 0;
    }
    else if ((q[2][2] + 1.0) <= epsilon)
    {
      InPlaceFilteredAxes[2] = 2;
      flipAxis[2] = 1;
    }
  }

  if ((InPlaceFilteredAxes[0] == InPlaceFilteredAxes[1]) ||
    (InPlaceFilteredAxes[0] == InPlaceFilteredAxes[2]) ||
    (InPlaceFilteredAxes[1] == InPlaceFilteredAxes[2]))
  {
    flipAxis[0] = 0;
    flipAxis[1] = 0;
    flipAxis[2] = 0;
    InPlaceFilteredAxes[0] = 0;
    InPlaceFilteredAxes[1] = 1;
    InPlaceFilteredAxes[2] = 2;
  }

  /*for (count=0;count<3;count++){
    if(flipAxis[count]){
    inOriginOffset[count] = -1 * inOriginOffset[count];
    } else {
    inOriginOffset[count] = inOriginOffset[count];
    }
  }*/

  for (count = 0; count < 3; count++)
  {
    inDim[count] = (this->DataExtent[(count * 2) + 1] - this->DataExtent[count * 2]) + 1;
    inSpacing[count] = this->DataSpacing[count];
    inExtent[count * 2] = this->DataExtent[count * 2];
    inExtent[(count * 2) + 1] = this->DataExtent[(count * 2) + 1];
    inIncrements[count] = this->DataIncrements[count];
    inOrigin[count] = this->DataOrigin[count];
  }

  inOrigin[0] = -128.5;
  inOrigin[1] = -128.5;
  inOrigin[2] = -128.5;

  inStride[0] = scalarSize;
  inStride[1] = inDim[0] * scalarSize;            // 0
  inStride[2] = inDim[1] * inDim[0] * scalarSize; // 1 0

  for (count = 0; count < 3; count++)
  {
    if (flipAxis[count])
    {
      inOriginOffset[count] = inOriginOffset[count] - inDim[count];
    }
  }

  for (count = 0; count < 3; count++)
  {
    outDim[count] = inDim[InPlaceFilteredAxes[count]];
    outStride[count] = inStride[InPlaceFilteredAxes[count]];
    outIncrements[count] = inIncrements[InPlaceFilteredAxes[count]];
    outSpacing[count] = inSpacing[InPlaceFilteredAxes[count]];
    outExtent[count * 2] = inExtent[InPlaceFilteredAxes[count] * 2];
    outExtent[(count * 2) + 1] = inExtent[(InPlaceFilteredAxes[count] * 2) + 1];
    outOrigin[count] = inOrigin[InPlaceFilteredAxes[count]];
  }

  for (count = 0; count < 3; count++)
  {
    this->DataIncrements[count] = outIncrements[count];
    this->DataSpacing[count] = outSpacing[count];
    this->DataExtent[count * 2] = (int)(outExtent[count * 2]);
    this->DataExtent[(count * 2) + 1] = (int)(outExtent[(count * 2) + 1]);
    this->DataOrigin[count] = outOrigin[count];
  }

  unsigned char* tempUnsignedCharData = nullptr;

  // permute

  size_t tempUnsignedCharDataSize = outDim[0] * outDim[1] * outDim[2] * scalarSize;

  tempUnsignedCharData = new unsigned char[tempUnsignedCharDataSize];

  int idSize;
  int idZ, idY, idX;

  // Loop through input voxels
  count = 0;
  for (inIndex[2] = 0; inIndex[2] < outDim[2]; inIndex[2]++)
  {
    for (inIndex[1] = 0; inIndex[1] < outDim[1]; inIndex[1]++)
    {
      for (inIndex[0] = 0; inIndex[0] < outDim[0]; inIndex[0]++)
      {
        inOffset =
          (inIndex[2] * outStride[2]) + (inIndex[1] * outStride[1]) + (inIndex[0] * outStride[0]);

        for (idSize = 0; idSize < scalarSize; idSize++)
        {
          charInOffset = inOffset + idSize;
          tempUnsignedCharData[count++] = outUnsignedCharPtr[charInOffset];
        }
      }
    }
  }

  long outSliceSize = outDim[0] * outDim[1] * scalarSize;
  long outRowSize = outDim[0] * scalarSize;
  long outRowOffset;
  long outSliceOffset;
  long outOffset;
  long charOutOffset;

  // Loop through output voxels
  count = 0;
  for (idZ = 0; idZ < outDim[2]; idZ++)
  {
    outSliceOffset = idZ * outSliceSize;
    for (idY = 0; idY < outDim[1]; idY++)
    {
      outRowOffset = idY * outRowSize;
      for (idX = 0; idX < outDim[0]; idX++)
      {
        outOffset = outSliceOffset + outRowOffset + (idX * scalarSize);
        for (idSize = 0; idSize < scalarSize; idSize++)
        {
          charOutOffset = outOffset + idSize;
          outUnsignedCharPtr[charOutOffset] = tempUnsignedCharData[count++];
        }
      }
    }
  }

  // now flip

  // Loop through input voxels
  count = 0;
  for (inIndex[2] = 0; inIndex[2] < outDim[2]; inIndex[2]++)
  {
    if (flipAxis[2] == 1)
    {
      flipIndex[2] = ((outDim[2] - 1) - inIndex[2]);
    }
    else
    {
      flipIndex[2] = inIndex[2];
    }
    for (inIndex[1] = 0; inIndex[1] < outDim[1]; inIndex[1]++)
    {
      if (flipAxis[1] == 1)
      {
        flipIndex[1] = ((outDim[1] - 1) - inIndex[1]);
      }
      else
      {
        flipIndex[1] = inIndex[1];
      }
      for (inIndex[0] = 0; inIndex[0] < outDim[0]; inIndex[0]++)
      {
        if (flipAxis[0] == 1)
        {
          flipIndex[0] = ((outDim[0] - 1) - inIndex[0]);
        }
        else
        {
          flipIndex[0] = inIndex[0];
        }
        inOffset =
          (flipIndex[2] * outSliceSize) + (flipIndex[1] * outRowSize) + (flipIndex[0] * scalarSize);
        for (idSize = 0; idSize < scalarSize; idSize++)
        {
          charInOffset = inOffset + idSize;
          tempUnsignedCharData[count++] = outUnsignedCharPtr[charInOffset];
        }
      }
    }
  }

  // Loop through output voxels
  count = 0;
  for (idZ = 0; idZ < outDim[2]; idZ++)
  {
    outSliceOffset = idZ * outSliceSize;
    for (idY = 0; idY < outDim[1]; idY++)
    {
      outRowOffset = idY * outRowSize;
      for (idX = 0; idX < outDim[0]; idX++)
      {
        outOffset = outSliceOffset + outRowOffset + (idX * scalarSize);
        for (idSize = 0; idSize < scalarSize; idSize++)
        {
          charOutOffset = outOffset + idSize;
          if ((count >= 0) && (static_cast<size_t>(count) < outPtrSize) && (charOutOffset >= 0) &&
            (static_cast<size_t>(charOutOffset) < tempUnsignedCharDataSize))
          {
            outUnsignedCharPtr[charOutOffset] = tempUnsignedCharData[count];
          }
          count++;
        }
      }
    }
  }

  delete tempUnsignedCharData;
  tempUnsignedCharData = nullptr;
}

//----------------------------------------------------------------------------
void vtkNIfTIReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkNIfTIReader::CanReadFile(const char* fname)
{

  std::string filename(fname);

  // we check that the correction extension is given by the user
  std::string filenameext = GetExtension(filename);
  if (filenameext != std::string("hdr") && filenameext != std::string("img.gz") &&
    filenameext != std::string("img") && filenameext != std::string("nii") &&
    filenameext != std::string("nii.gz"))
  {
    return false;
  }

  const std::string HeaderFileName = GetHeaderFileName(filename);
  //
  // only try to read HDR files
  std::string ext = GetExtension(HeaderFileName);

  if (ext == std::string("gz"))
  {
    ext = GetExtension(GetRootName(HeaderFileName));
  }
  if (ext != std::string("hdr") && ext != std::string("img") && ext != std::string("nii"))
  {
    return false;
  }

  vtksys::ifstream local_InputStream;
  local_InputStream.open(HeaderFileName.c_str(), ios::in | ios::binary);
  if (local_InputStream.fail())
  {
    return false;
  }

  struct nifti_1_header m_hdr;
  if (!ReadBufferAsBinary(local_InputStream, (void*)&(m_hdr), sizeof(struct nifti_1_header)))
  {
    return false;
  }
  local_InputStream.close();

  // if the machine and file endianness are different
  // perform the byte swapping on it
  // this->m_ByteOrder = this->CheckAnalyzeEndian(this->m_hdr);
  // this->SwapHeaderBytesIfNecessary( &(this->m_hdr) );

  // The final check is to make sure that it is a nifti  file.
  niftiType = vtknifti1_io::is_nifti_file(fname);
  return ((niftiType == 1) || (niftiType == 2));
}
