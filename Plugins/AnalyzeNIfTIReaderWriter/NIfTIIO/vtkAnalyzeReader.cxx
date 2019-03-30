/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAnalyzeReader.cxx

  Copyright (c) Joseph Hennessey
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAnalyzeReader.h"

#include "vtkByteSwap.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtk_zlib.h"
#include "vtknifti1.h"
#include "vtknifti1_io.h"
#include "vtkznzlib.h"

#include "vtkBitArray.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkUnsignedCharArray.h"

#include "vtkStringArray.h"
#define NAME_ARRAY "Name"
#define DEFAULT_NAME ""

vtkStandardNewMacro(vtkAnalyzeReader);

//----------------------------------------------------------------------------
vtkAnalyzeReader::vtkAnalyzeReader()
{
  this->analyzeHeader = 0;
  this->analyzeHeaderUnsignedCharArray = 0;
  this->analyzeHeaderSize = 348;

  this->fixFlipError = true;
}

//----------------------------------------------------------------------------
vtkAnalyzeReader::~vtkAnalyzeReader()
{
  if (this->analyzeHeader)
  {
    this->analyzeHeader->Delete();
    this->analyzeHeader = 0;
  }
  if (this->analyzeHeaderUnsignedCharArray)
  {
    delete[] this->analyzeHeaderUnsignedCharArray;
    this->analyzeHeaderUnsignedCharArray = 0;
  }
}

// GetExtension from uiig library.
static std::string GetExtension(const std::string& filename)
{

  // This assumes that the final '.' in a file name is the delimiter
  // for the file's extension type
  const std::string::size_type it = filename.find_last_of(".");

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
  ImageFileName += ".hdr";
  return (ImageFileName);
}

// Returns the base image filename.
static std::string GetImageFileName(const std::string& filename)
{
  // Why do we add ".img" here?  Look in fileutils.h
  std::string fileExt = GetExtension(filename);
  std::string ImageFileName = GetRootName(filename);
  if (!fileExt.compare("gz"))
  {
    // First strip both extensions off
    ImageFileName = GetRootName(GetRootName(filename));
    ImageFileName += ".img.gz";
  }
  else if (!fileExt.compare("img") || !fileExt.compare("hdr"))
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
void vtkAnalyzeReader::ExecuteInformation()
{

  nifti_image* m_NiftiImage;
  dataTypeSize = 1.0;
  unsigned int numComponents = 1;
  nifti_1_header niftiHeader;
  unsigned char* analyzeHeaderUnsignedCharArrayPtr = (unsigned char*)&niftiHeader;

  this->analyzeHeaderUnsignedCharArray = new unsigned char[this->analyzeHeaderSize];

  m_NiftiImage = vtknifti1_io::nifti_image_read(this->GetFileName(), false);
  if (m_NiftiImage == NULL)
  {
    vtkErrorMacro("Read failed");
    return;
  }

  Type = m_NiftiImage->datatype;

  if (Type == DT_BINARY)
  {

    int alignmentSize = 8;

    binaryOnDiskWidth = m_NiftiImage->nx;
    int tempWidth = m_NiftiImage->nx;
    double tempBitWidthDouble = tempWidth / (alignmentSize * 1.0);
    int tempBitWidthInt = (int)tempBitWidthDouble;
    if (tempBitWidthInt != tempBitWidthDouble)
    {
      tempBitWidthInt++;
      tempWidth = tempBitWidthInt * alignmentSize;
      m_NiftiImage->nx = tempWidth;
      m_NiftiImage->dim[1] = m_NiftiImage->nx;
    }

    binaryOnDiskHeight = m_NiftiImage->ny;
    int tempHeight = m_NiftiImage->ny;
    double tempBitHeightDouble = tempHeight / (alignmentSize * 1.0);
    int tempBitHeightInt = (int)tempBitHeightDouble;
    if (tempBitHeightInt != tempBitHeightDouble)
    {
      tempBitHeightInt++;
      tempHeight = tempBitHeightInt * alignmentSize;
      m_NiftiImage->ny = tempHeight;
      m_NiftiImage->dim[2] = m_NiftiImage->ny;
    }

    binaryOnDiskDepth = m_NiftiImage->nz;
    int tempDepth = m_NiftiImage->nz;
    double tempBitDepthDouble = tempDepth / (alignmentSize * 1.0);
    int tempBitDepthInt = (int)tempBitDepthDouble;
    if (tempBitDepthInt != tempBitDepthDouble)
    {
      tempBitDepthInt++;
      tempDepth = tempBitDepthInt * alignmentSize;
      m_NiftiImage->dim[3] = m_NiftiImage->nz;
      m_NiftiImage->nz = tempDepth;
    }
  }

  niftiHeader = vtknifti1_io::nifti_convert_nim2nhdr(m_NiftiImage);

  int count;

  for (count = 0; count < this->analyzeHeaderSize; count++)
  {
    this->analyzeHeaderUnsignedCharArray[count] = analyzeHeaderUnsignedCharArrayPtr[count];
  }
  // fix orient
  int orientPosition = 252;
  this->analyzeHeaderUnsignedCharArray[orientPosition] = m_NiftiImage->analyze75_orient;
  /*
    a75_transverse_unflipped = 0,
    a75_coronal_unflipped = 1,
    a75_sagittal_unflipped = 2,
    a75_transverse_flipped = 3,
    a75_coronal_flipped = 4,
    a75_sagittal_flipped = 5,
    a75_orient_unknown = 6

hist.orient    Mayo name          Voxel[Index0, Index1, Index2]
                    Index0   Index1   Index2
0 (default)   transverse unflipped   R-L   P-A   I-S
1         coronal unflipped     R-L   I-S   P-A
2         sagittal unflipped     P-A   I-S   R-L
3         transverse flipped     R-L   A-P   I-S
4         coronal flipped     R-L   S-I   P-A
5         sagittal flipped     P-A   S-I   R-L
Note: Index0 is fastest-varying (innermost-nested) index, Index2 the outermost.
Index0..Index2 are often called X, Y, Z, but I am trying to avoid confusion with spatial
coordinates.
2004-04-18: See later section for comparison to NIfTI

hist.orient    Mayo name          Voxel[Index0, Index1, Index2]    NIfTI (2004-4-15)
                    Index0   Index1   Index2       Index0..2
0 (default)   transverse unflipped   R-L   P-A   I-S       R?L P?A I?S
1         coronal unflipped     R-L   I-S   P-A       R?L I?S P?A
2         sagittal unflipped     P-A   I-S   R-L       P?A I?S R?L
3         transverse flipped     R-L   A-P   I-S       R?L A?P I?S
4         coronal flipped     R-L   S-I   P-A       R?L S?I P?A
5         sagittal flipped     P-A   S-I   R-L       P?A I?S L?R
Note: Index0 is fastest-varying (innermost-nested) index, Index2 the outermost.
2004-04-15: NIfTI's interpretation of "sagittal flipped" looks off to me. See further notes below.
  */

  orientation = m_NiftiImage->analyze75_orient;

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
    case DT_SIGNED_SHORT:
      this->SetDataScalarTypeToShort();
      dataTypeSize = 2;
      break;
    case DT_SIGNED_INT:
      this->SetDataScalarTypeToInt();
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
    case DT_RGB:
      // DEBUG -- Assuming this is a triple, not quad
      // image.setDataType( uiig::DATA_RGBQUAD );
      break;
    default:
      break;
  }
  //
  // set up the dimension stuff
  // for(dim = 0; dim < this->GetNumberOfDimensions(); dim++)
  //{
  // this->SetDimensions(dim,this->m_hdr.dime.dim[dim+1]);
  // this->SetSpacing(dim,this->m_hdr.dime.pixdim[dim+1]);
  //}

  this->SetNumberOfScalarComponents(numComponents);

  this->diskDimensions[0] = m_NiftiImage->dim[1];
  this->diskDimensions[1] = m_NiftiImage->dim[2];
  this->diskDimensions[2] = m_NiftiImage->dim[3];

  this->diskExtent[0] = 0;
  this->diskExtent[1] = m_NiftiImage->dim[1] - 1;
  this->diskExtent[2] = 0;
  this->diskExtent[3] = m_NiftiImage->dim[2] - 1;
  this->diskExtent[4] = 0;
  this->diskExtent[5] = m_NiftiImage->dim[3] - 1;

  this->diskSpacing[0] = m_NiftiImage->pixdim[1];
  this->diskSpacing[1] = m_NiftiImage->pixdim[2];
  this->diskSpacing[2] = m_NiftiImage->pixdim[3];

  // this->DataOrigin[0] = -128.5;
  // this->DataOrigin[1] = -127.5;
  // this->DataOrigin[2] = -128.5;

  imageSizeInBytes = (int)(numElts * dataTypeSize);
  if (Type == DT_BINARY)
  {
    double tempSize = numElts / m_NiftiImage->nz;
    double tempSliceSize = tempSize * dataTypeSize;
    int tempSliceSizeInt = (int)tempSliceSize;
    if (tempSliceSizeInt < tempSliceSize)
    {
      tempSliceSizeInt++;
    }
    imageSizeInBytes = tempSliceSizeInt * m_NiftiImage->nz;
  }

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

  int InPlaceFilteredAxes[3];

  InPlaceFilteredAxes[0] = 0;
  InPlaceFilteredAxes[1] = 1;
  InPlaceFilteredAxes[2] = 2;

  switch (orientation)
  {
    case 0:
    {
    }
    break;
    case 1:
    {
      InPlaceFilteredAxes[0] = 0;
      InPlaceFilteredAxes[1] = 2;
      InPlaceFilteredAxes[2] = 1;
    }
    break;
    case 2:
    {
      InPlaceFilteredAxes[0] = 2;
      InPlaceFilteredAxes[1] = 0;
      InPlaceFilteredAxes[2] = 1;
    }
    break;
    case 3:
    {
    }
    break;
    case 4:
    {
      InPlaceFilteredAxes[0] = 0;
      InPlaceFilteredAxes[1] = 2;
      InPlaceFilteredAxes[2] = 1;
    }
    break;
    case 5:
    {
      InPlaceFilteredAxes[0] = 2;
      InPlaceFilteredAxes[1] = 0;
      InPlaceFilteredAxes[2] = 1;
    }
    break;
    case 6:
    {
    }
    break;
    case 255:
    {
    }
    break;
    default:
    {
    }
    break;
  }

  this->voxelDimensions[0] = this->diskDimensions[InPlaceFilteredAxes[0]];
  this->voxelDimensions[1] = this->diskDimensions[InPlaceFilteredAxes[1]];
  this->voxelDimensions[2] = this->diskDimensions[InPlaceFilteredAxes[2]];

  this->DataExtent[0] = this->diskExtent[(InPlaceFilteredAxes[0] * 2)];
  this->DataExtent[1] = this->diskExtent[(InPlaceFilteredAxes[0] * 2) + 1];
  this->DataExtent[2] = this->diskExtent[(InPlaceFilteredAxes[1] * 2)];
  this->DataExtent[3] = this->diskExtent[(InPlaceFilteredAxes[1] * 2) + 1];
  this->DataExtent[4] = this->diskExtent[(InPlaceFilteredAxes[2] * 2)];
  this->DataExtent[5] = this->diskExtent[(InPlaceFilteredAxes[2] * 2) + 1];

  this->DataSpacing[0] = this->diskSpacing[InPlaceFilteredAxes[0]];
  this->DataSpacing[1] = this->diskSpacing[InPlaceFilteredAxes[1]];
  this->DataSpacing[2] = this->diskSpacing[InPlaceFilteredAxes[2]];

  this->vtkImageReader::ExecuteInformation();
}

//----------------------------------------------------------------------------
// This function reads in one data of data.
// templated to handle different data types.
template <class OT>
void vtkAnalyzeReaderUpdate2(vtkAnalyzeReader* self, vtkImageData* vtkNotUsed(data), OT* outPtr)
{
  // unsigned int dim;
  // char * const p = static_cast<char *>(outPtr);
  char* p = (char*)(outPtr);
  // 4 cases to handle
  // 1: given .hdr and image is .img
  // 2: given .img
  // 3: given .img.gz
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
  if (file_p == NULL)
  {
    /* Do a separate check to take care of case #4 */
    ImageFileName += ".gz";
    file_p = ::gzopen(ImageFileName.c_str(), "rb");
    if (file_p == NULL)
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
  int analyzeHeaderSize = 0;
  ::gzseek(file_p, analyzeHeaderSize, SEEK_SET);
  ::gzread(file_p, p, self->getImageSizeInBytes());
  gzclose(file_p);
  // SwapBytesIfNecessary( buffer, numberOfPixels );
}

void vtkAnalyzeReader::vtkAnalyzeReaderUpdateVTKBit(vtkImageData* vtkNotUsed(data), void* outPtr)
{
  // unsigned int dim;
  // char * const p = static_cast<char *>(outPtr);

  // vtkDataArray* tempScalars = data->GetPointData()->GetScalars();

  // vtkBitArray *bitData = vtkAbstractArray::SafeDownCast(tempScalars);
  // vtkBitArray *bitData = static_cast<vtkBitArray*>(tempScalars);

  int count;
  unsigned char tempByte;
  unsigned char tempBit;
  unsigned char newByte;
  unsigned char shiftedBit;
  int bitCount;
  // int binaryOnDiskWidth = width;
  double tempSize = binaryOnDiskWidth * binaryOnDiskHeight;
  double onDiskDoubleSliceSizeInBytes = tempSize * dataTypeSize;
  int onDiskIntSliceSizeInBytes = (int)onDiskDoubleSliceSizeInBytes;
  if (onDiskIntSliceSizeInBytes < onDiskDoubleSliceSizeInBytes)
  {
    onDiskIntSliceSizeInBytes++;
  }
  int onDiskImageSizeInBytes = onDiskIntSliceSizeInBytes * binaryOnDiskDepth;
  // int scalarSizeInBits = (int) (dataTypeSize * 8);

  int inDim[3];
  inDim[0] = this->voxelDimensions[0];
  inDim[1] = this->voxelDimensions[1];
  inDim[2] = this->voxelDimensions[2];

  double rowDoubleMemorySizeInBytes = inDim[0] * dataTypeSize;
  int rowIntMemorySizeInBytes = (int)rowDoubleMemorySizeInBytes;
  if (rowIntMemorySizeInBytes < rowDoubleMemorySizeInBytes)
  {
    rowIntMemorySizeInBytes++;
  }

  double totalDoubleMemorySizeInBytes = inDim[2] * inDim[1] * inDim[0] * dataTypeSize;
  // double totalDoubleMemorySizeInBytes = inDim[2] * inDim[1] * rowIntMemorySizeInBytes;
  int totalIntMemorySizeInBytes = (int)totalDoubleMemorySizeInBytes;
  if (totalIntMemorySizeInBytes < totalDoubleMemorySizeInBytes)
  {
    totalIntMemorySizeInBytes++;
  }

  // char *  outP = (char *)(outPtr);
  char* p = new char[onDiskImageSizeInBytes];

  unsigned char* unsignedOutP = (unsigned char*)(outPtr);
  unsigned char* unsignedP = (unsigned char*)(p);
  // 4 cases to handle
  // 1: given .hdr and image is .img
  // 2: given .img
  // 3: given .img.gz
  // 4: given .hdr and image is .img.gz
  //   Special processing needed for this case only
  // NOT NEEDED const std::string fileExt = GetExtension(m_FileName);

  /* Returns proper name for cases 1,2,3 */
  std::string ImageFileName = GetImageFileName(GetFileName());
  // NOTE: gzFile operations act just like FILE * operations when the files
  // are not in gzip format.
  // This greatly simplifies the following code, and gzFile types are used
  // everywhere.
  // In addition, it has the added benefit of reading gzip compressed image
  // files that do not have a .gz ending.
  gzFile file_p = ::gzopen(ImageFileName.c_str(), "rb");
  if (file_p == NULL)
  {
    /* Do a separate check to take care of case #4 */
    ImageFileName += ".gz";
    file_p = ::gzopen(ImageFileName.c_str(), "rb");
    if (file_p == NULL)
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
  //::gzread( file_p, p, self->getImageSizeInBytes());
  int tempAnalyzeHeaderSize = 0;
  ::gzseek(file_p, tempAnalyzeHeaderSize, SEEK_SET);
  ::gzread(file_p, p, onDiskImageSizeInBytes);

  gzclose(file_p);
  // SwapBytesIfNecessary( buffer, numberOfPixels );

  for (count = 0; count < onDiskImageSizeInBytes; count++)
  {
    tempByte = unsignedP[count];

    // swap tempByte
    newByte = 0;
    for (bitCount = 0; bitCount < 8; bitCount++)
    {
      tempBit = (tempByte >> bitCount) & 0x01;
      shiftedBit = tempBit << (bitCount); //(7 - bitCount)
      newByte += shiftedBit;
    }

    unsignedP[count] = newByte;
  }

  // Loop through input voxels
  int totalBitCount = 0;
  int byteBitCount = 0;
  int byteCount = 0;
  int inIndex[3];
  int inSliceBitNumber;
  int inSliceByteNumber;
  unsigned char tempInByteValue = 0;
  unsigned char tempInBitValue = 0;
  int inSliceByteOffset;
  int inSliceBitOffset;
  int inByteBitNumber = 0;
  int inTotalBitNumber = 0;
  int inTotalByteNumber = 0;
  unsigned char shiftedBitValue = 0;
  unsigned char zeroValue = 0;

  for (count = 0; count < totalIntMemorySizeInBytes; count++)
  {
    unsignedOutP[count] = zeroValue;
  }
  for (inIndex[2] = 0; inIndex[2] < binaryOnDiskDepth; inIndex[2]++)
  {
    inSliceByteOffset = onDiskIntSliceSizeInBytes * inIndex[2];
    inSliceBitOffset = inSliceByteOffset * 8;
    for (inIndex[1] = 0; inIndex[1] < binaryOnDiskHeight; inIndex[1]++)
    {
      for (inIndex[0] = 0; inIndex[0] < binaryOnDiskWidth; inIndex[0]++)
      {
        inSliceBitNumber = (inIndex[1] * binaryOnDiskWidth) + inIndex[0];
        inSliceByteNumber = (int)(inSliceBitNumber / 8);

        inTotalBitNumber = inSliceBitOffset + inSliceBitNumber;
        inTotalByteNumber = inSliceByteOffset + inSliceByteNumber;

        inByteBitNumber = inTotalBitNumber % 8;

        tempInByteValue = unsignedP[inTotalByteNumber];
        tempInBitValue = (tempInByteValue >> inByteBitNumber) & 0x01;
        byteBitCount = totalBitCount % 8;
        byteCount = (int)(totalBitCount / 8);

        // set values
        shiftedBitValue = tempInBitValue << (byteBitCount); //(7 - byteBitCount)
        unsignedOutP[byteCount] += shiftedBitValue;

        totalBitCount++;
      }
      for (inIndex[0] = binaryOnDiskWidth; inIndex[0] < inDim[0]; inIndex[0]++)
      {
        tempInBitValue = 0x00;

        byteBitCount = totalBitCount % 8;
        byteCount = (int)(totalBitCount / 8);

        // set values
        shiftedBitValue = tempInBitValue << (byteBitCount); //(7 - byteBitCount)
        unsignedOutP[byteCount] += shiftedBitValue;

        totalBitCount++;
      }
    }
    for (inIndex[1] = binaryOnDiskHeight; inIndex[1] < inDim[1]; inIndex[1]++)
    {
      for (inIndex[0] = 0; inIndex[0] < inDim[0]; inIndex[0]++)
      {
        tempInBitValue = 0x00;

        byteBitCount = totalBitCount % 8;
        byteCount = (int)(totalBitCount / 8);

        // set values
        shiftedBitValue = tempInBitValue << (byteBitCount); //(7 - byteBitCount)
        unsignedOutP[byteCount] += shiftedBitValue;

        totalBitCount++;
      }
    }
  }
  for (inIndex[2] = binaryOnDiskDepth; inIndex[2] < inDim[2]; inIndex[2]++)
  {
    for (inIndex[1] = 0; inIndex[1] < inDim[1]; inIndex[1]++)
    {
      for (inIndex[0] = 0; inIndex[0] < inDim[0]; inIndex[0]++)
      {
        tempInBitValue = 0x00;

        byteBitCount = totalBitCount % 8;
        byteCount = (int)(totalBitCount / 8);

        // set values
        shiftedBitValue = tempInBitValue << (byteBitCount); //(7 - byteBitCount)
        unsignedOutP[byteCount] += shiftedBitValue;

        totalBitCount++;
      }
    }
  }

  for (count = 0; count < totalIntMemorySizeInBytes; count++)
  {
    tempByte = unsignedOutP[count];

    // if(tempByte==252){
    // tempByte=tempByte;
    //}
    // if(tempByte!=0){
    // tempByte=tempByte;
    //}
    // swap tempByte
    newByte = 0;
    for (bitCount = 0; bitCount < 8; bitCount++)
    {
      tempBit = (tempByte >> bitCount) & 0x01;
      shiftedBit = tempBit << (7 - bitCount); //(7 - bitCount)
      newByte += shiftedBit;
    }

    unsignedOutP[count] = newByte;
  }
  if (p != NULL)
  {
    delete[] p;
    p = NULL;
    unsignedP = NULL;
  }
}

//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkAnalyzeReader::ExecuteDataWithInformation(vtkDataObject* output, vtkInformation* outInfo)
{
  vtkImageData* data = this->AllocateOutputData(output, outInfo);

  if (this->UpdateExtentIsEmpty(outInfo, output))
  {
    return;
  }
  if (this->GetFileName() == NULL)
  {
    vtkErrorMacro(<< "Either a FileName or FilePrefix must be specified.");
    return;
  }

  data->GetPointData()->GetScalars()->SetName("AnalyzeImage");

  // Call the correct templated function for the output
  void* outPtr;

  // Call the correct templated function for the input
  outPtr = data->GetScalarPointer();
  int tempScalarTypeValue = data->GetScalarType();
  switch (tempScalarTypeValue)
  {
    vtkTemplateMacro(vtkAnalyzeReaderUpdate2(this, data, static_cast<VTK_TT*>(outPtr)));
    default:
      if (tempScalarTypeValue != 1)
      {
        vtkErrorMacro(<< "Execute: Unknown data type");
      }
      else
      {
        vtkAnalyzeReaderUpdateVTKBit(data, outPtr);
      }
  }

  /*
    a75_transverse_unflipped = 0,
    a75_coronal_unflipped = 1,
    a75_sagittal_unflipped = 2,
    a75_transverse_flipped = 3,
    a75_coronal_flipped = 4,
    a75_sagittal_flipped = 5,
    a75_orient_unknown = 6

hist.orient    Mayo name          Voxel[Index0, Index1, Index2]
                    Index0   Index1   Index2
0 (default)   transverse unflipped   R-L   P-A   I-S
1         coronal unflipped     R-L   I-S   P-A
2         sagittal unflipped     P-A   I-S   R-L
3         transverse flipped     R-L   A-P   I-S
4         coronal flipped     R-L   S-I   P-A
5         sagittal flipped     P-A   S-I   R-L
Note: Index0 is fastest-varying (innermost-nested) index, Index2 the outermost.
Index0..Index2 are often called X, Y, Z, but I am trying to avoid confusion with spatial
coordinates.
2004-04-18: See later section for comparison to NIfTI

hist.orient    Mayo name          Voxel[Index0, Index1, Index2]    NIfTI (2004-4-15)
                    Index0   Index1   Index2       Index0..2
0 (default)   transverse unflipped   R-L   P-A   I-S       R?L P?A I?S
1         coronal unflipped     R-L   I-S   P-A       R?L I?S P?A
2         sagittal unflipped     P-A   I-S   R-L       P?A I?S R?L
3         transverse flipped     R-L   A-P   I-S       R?L A?P I?S
4         coronal flipped     R-L   S-I   P-A       R?L S?I P?A
5         sagittal flipped     P-A   S-I   R-L       P?A I?S L?R
Note: Index0 is fastest-varying (innermost-nested) index, Index2 the outermost.
2004-04-15: NIfTI's interpretation of "sagittal flipped" looks off to me. See further notes below.
  */
  int scalarSize = (int)dataTypeSize;
  int inIndex[3];
  int inDim[3];
  int outDim[3];
  int count;
  int inExtent[6];
  int outExtent[6];
  int inStride[3];
  int outStride[3];

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

  // now flip if historic incorrect behavior is desired
  if (fixFlipError == false)
  {
    flipAxis[0] = 1;
    flipAxis[1] = 1;
    flipAxis[2] = 1;
  }

  switch (orientation)
  {
    case 0:
    {
      // flipAxis[0] = 1;
      // flipAxis[1] = 1;
      // flipAxis[2] = 1;
    }
    break;
    case 1:
    {
      InPlaceFilteredAxes[0] = 0;
      InPlaceFilteredAxes[1] = 2;
      InPlaceFilteredAxes[2] = 1;
    }
    break;
    case 2:
    {
      InPlaceFilteredAxes[0] = 2;
      InPlaceFilteredAxes[1] = 0;
      InPlaceFilteredAxes[2] = 1;
    }
    break;
    case 3:
    {
      flipAxis[1] = 1;
    }
    break;
    case 4:
    {
      InPlaceFilteredAxes[0] = 0;
      InPlaceFilteredAxes[1] = 2;
      InPlaceFilteredAxes[2] = 1;
      flipAxis[2] = 1;
    }
    break;
    case 5:
    {
      InPlaceFilteredAxes[0] = 2;
      InPlaceFilteredAxes[1] = 0;
      InPlaceFilteredAxes[2] = 1;
      flipAxis[2] = 1;
    }
    break;
    case 6:
    {
    }
    break;
    case 255:
    {
    }
    break;
    default:
    {
    }
    break;
  }

  if (tempScalarTypeValue == 1)
  {
    scalarSize = 1;
  }

  for (count = 0; count < 3; count++)
  {
    inDim[count] = (this->diskExtent[(count * 2) + 1] - this->diskExtent[count * 2]) + 1;
    inExtent[count * 2] = this->diskExtent[count * 2];
    inExtent[(count * 2) + 1] = this->diskExtent[(count * 2) + 1];
  }

  inStride[0] = scalarSize;
  inStride[1] = inDim[0] * scalarSize;
  inStride[2] = inDim[1] * inDim[0] * scalarSize;

  for (count = 0; count < 3; count++)
  {
    outDim[count] = inDim[InPlaceFilteredAxes[count]];
    outStride[count] = inStride[InPlaceFilteredAxes[count]];
    outExtent[count * 2] = inExtent[InPlaceFilteredAxes[count] * 2];
    outExtent[(count * 2) + 1] = inExtent[(InPlaceFilteredAxes[count] * 2) + 1];
  }

  if (tempScalarTypeValue != 1)
  {
    for (count = 0; count < 3; count++)
    {
      this->DataExtent[count * 2] = outExtent[count * 2];
      this->DataExtent[(count * 2) + 1] = outExtent[(count * 2) + 1];
    }
  }

  unsigned char* tempUnsignedCharData = NULL;

  unsigned char* outUnsignedCharPtr = (unsigned char*)outPtr;

  int tempSizeInt = outDim[0] * outDim[1] * outDim[2] * scalarSize;
  tempUnsignedCharData = new unsigned char[tempSizeInt];
  double tempSizeDouble;
  if (tempScalarTypeValue == 1)
  {
    tempSizeDouble = outDim[0] * outDim[1] * outDim[2] * dataTypeSize;
    tempSizeInt = (int)tempSizeDouble;
    if (tempSizeInt != tempSizeDouble)
    {
      tempSizeInt++;
    }
    if (tempUnsignedCharData != NULL)
    {
      delete[] tempUnsignedCharData;
      tempUnsignedCharData = NULL;
    }
    tempUnsignedCharData = new unsigned char[tempSizeInt];
  }

  int idSize;
  int idZ, idY, idX;
  long outRowOffset;
  long outSliceOffset;
  long outOffset;
  long charOutOffset;
  long outSliceSize;
  long outRowSize;

  int outBitCount;
  int outByteCount;
  int inOffsetByte;
  int inOffsetBit;
  unsigned char inBitValue;
  unsigned char inByteValue;
  unsigned char shiftedBitValue;
  // permute

  unsigned char zeroValue = 0;

  if (tempScalarTypeValue == 1)
  {
    for (count = 0; count < tempSizeInt; count++)
    {
      tempUnsignedCharData[count] = zeroValue;
    }
  }

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
        // inOffset = (inIndex[2] * inStride[2]) + (inIndex[1] * inStride[1]) + (inIndex[0] *
        // inStride[0]);
        if (tempScalarTypeValue != 1)
        {
          for (idSize = 0; idSize < scalarSize; idSize++)
          {
            charInOffset = inOffset + idSize;
            tempUnsignedCharData[count++] = outUnsignedCharPtr[charInOffset];
          }
        }
        else
        {
          inOffsetByte = inOffset / 8;
          inOffsetBit = inOffset % 8;

          inByteValue = outUnsignedCharPtr[inOffsetByte];
          inBitValue = (inByteValue >> inOffsetBit) & 0x01;

          outBitCount = count % 8;
          outByteCount = count / 8;
          shiftedBitValue = inBitValue << (outBitCount);
          if (outByteCount >= tempSizeInt)
          {
            outByteCount = tempSizeInt - 1;
          }

          tempUnsignedCharData[outByteCount] += shiftedBitValue;
          count++;
        }
      }
    }
  }

  outSliceSize = outDim[0] * outDim[1] * scalarSize;
  outRowSize = outDim[0] * scalarSize;

  if (tempScalarTypeValue == 1)
  {
    for (count = 0; count < tempSizeInt; count++)
    {
      // outUnsignedCharPtr[count] = zeroValue;
      outUnsignedCharPtr[count] = tempUnsignedCharData[count];
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
        if (tempScalarTypeValue != 1)
        {
          outOffset = outSliceOffset + outRowOffset + (idX * scalarSize);
          for (idSize = 0; idSize < scalarSize; idSize++)
          {
            charOutOffset = outOffset + idSize;
            outUnsignedCharPtr[charOutOffset] = tempUnsignedCharData[count++];
          }
        }
        else
        {
          outOffset = (idZ * outDim[0] * outDim[1]) + (idY * outDim[0]) + idX;

          inOffsetByte = count / 8;
          inOffsetBit = count % 8;

          inByteValue = tempUnsignedCharData[inOffsetByte];
          inBitValue = (inByteValue >> inOffsetBit) & 0x01;

          outBitCount = outOffset % 8;
          outByteCount = outOffset / 8;
          shiftedBitValue = inBitValue << (outBitCount);
          if (outByteCount >= tempSizeInt)
          {
            outByteCount = tempSizeInt - 1;
          }
          count++;
        }
      }
    }
  }

  // now flip
  // flipAxis[0] = 0;
  // flipAxis[1] = 0;
  // flipAxis[2] = 1;

  if (tempScalarTypeValue == 1)
  {
    for (count = 0; count < tempSizeInt; count++)
    {
      tempUnsignedCharData[count] = zeroValue;
      // tempUnsignedCharData[count] = outUnsignedCharPtr[count];
    }
  }

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
        if (tempScalarTypeValue != 1)
        {
          inOffset = (flipIndex[2] * outSliceSize) + (flipIndex[1] * outRowSize) +
            (flipIndex[0] * scalarSize);
          for (idSize = 0; idSize < scalarSize; idSize++)
          {
            charInOffset = inOffset + idSize;
            tempUnsignedCharData[count++] = outUnsignedCharPtr[charInOffset];
          }
        }
        else
        {
          inOffset =
            (flipIndex[2] * outDim[0] * outDim[1]) + (flipIndex[1] * outDim[0]) + flipIndex[0];

          inOffsetByte = inOffset / 8;
          inOffsetBit = inOffset % 8;

          inByteValue = outUnsignedCharPtr[inOffsetByte];
          inBitValue = (inByteValue >> (inOffsetBit)) & 0x01;

          outBitCount = count % 8;
          outByteCount = count / 8;
          shiftedBitValue = inBitValue << (outBitCount);
          tempUnsignedCharData[outByteCount] += shiftedBitValue;
          count++;
        }
      }
    }
  }

  if (tempScalarTypeValue == 1)
  {
    for (count = 0; count < tempSizeInt; count++)
    {
      outUnsignedCharPtr[count] = tempUnsignedCharData[count];
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
        if (tempScalarTypeValue != 1)
        {
          outOffset = outSliceOffset + outRowOffset + (idX * scalarSize);
          for (idSize = 0; idSize < scalarSize; idSize++)
          {
            charOutOffset = outOffset + idSize;
            outUnsignedCharPtr[charOutOffset] = tempUnsignedCharData[count++];
          }
        }
        else
        {
          outOffset = (idZ * outDim[0] * outDim[1]) + (idY * outDim[0]) + idX;

          inOffsetByte = count / 8;
          inOffsetBit = count % 8;

          inByteValue = tempUnsignedCharData[inOffsetByte];
          inBitValue = (inByteValue >> inOffsetBit) & 0x01;

          outBitCount = outOffset % 8;
          outByteCount = outOffset / 8;
          shiftedBitValue = inBitValue << (outBitCount);
          if (outByteCount >= tempSizeInt)
          {
            outByteCount = tempSizeInt - 1;
          }
          count++;
        }
      }
    }
  }

  int endianess = this->GetDataByteOrder();
  if (endianess == VTK_FILE_BYTE_ORDER_BIG_ENDIAN)
  {
    if (tempScalarTypeValue == VTK_SHORT)
    {
      unsigned char tempValue1;
      unsigned char tempValue2;
      for (count = 0; count < tempSizeInt; count = count + 2)
      {
        tempValue1 = tempUnsignedCharData[count];
        tempValue2 = tempUnsignedCharData[count + 1];
        outUnsignedCharPtr[count] = tempValue2;
        outUnsignedCharPtr[count + 1] = tempValue1;
      }
    }
    else if (tempScalarTypeValue == VTK_INT)
    {
      unsigned char tempValue1;
      unsigned char tempValue2;
      unsigned char tempValue3;
      unsigned char tempValue4;
      for (count = 0; count < tempSizeInt; count = count + 4)
      {
        tempValue1 = tempUnsignedCharData[count];
        tempValue2 = tempUnsignedCharData[count + 1];
        tempValue3 = tempUnsignedCharData[count + 2];
        tempValue4 = tempUnsignedCharData[count + 3];
        outUnsignedCharPtr[count] = tempValue4;
        outUnsignedCharPtr[count + 1] = tempValue3;
        outUnsignedCharPtr[count + 2] = tempValue2;
        outUnsignedCharPtr[count + 3] = tempValue1;
      }
    }
  }
  if (tempUnsignedCharData != NULL)
  {
    delete[] tempUnsignedCharData;
    tempUnsignedCharData = NULL;
  }

  vtkFieldData* fa = data->GetFieldData();

  if (!fa)
  {
    fa = vtkFieldData::New();
    data->SetFieldData(fa);
    fa->Delete();
    fa = data->GetFieldData();
  }

  vtkDataArray* validDataArray = fa->GetArray(ANALYZE_HEADER_ARRAY);
  if (!validDataArray)
  {
    this->analyzeHeader = vtkUnsignedCharArray::New();
    this->analyzeHeader->SetName(ANALYZE_HEADER_ARRAY);
    this->analyzeHeader->SetNumberOfValues(this->analyzeHeaderSize);
    fa->AddArray(this->analyzeHeader);
    validDataArray = fa->GetArray(ANALYZE_HEADER_ARRAY);
  }
  this->analyzeHeader = vtkUnsignedCharArray::SafeDownCast(validDataArray);

  for (count = 0; count < this->analyzeHeaderSize; count++)
  {
    this->analyzeHeader->SetValue(count, analyzeHeaderUnsignedCharArray[count]);
  }

  vtkDataArray* tempVolumeOriginDoubleArray = fa->GetArray(VOLUME_ORIGIN_DOUBLE_ARRAY);
  if (!tempVolumeOriginDoubleArray)
  {
    vtkDoubleArray* volumeOriginDoubleArray = NULL;
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
    vtkDoubleArray* volumeSpacingDoubleArray = NULL;
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
    size_t position = fileName.rfind("/");
    if (position != std::string::npos)
    {
      fileName.erase(0, position + 1);
    }
    position = fileName.rfind("\\");
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
}

//----------------------------------------------------------------------------
void vtkAnalyzeReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkAnalyzeReader::CanReadFile(const char* fname)
{
  std::string filename(fname);

  // we check that the correction extension is given by the user
  std::string filenameext = GetExtension(filename);
  if (filenameext != std::string("hdr") && filenameext != std::string("img.gz") &&
    filenameext != std::string("img"))
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
  if (ext != std::string("hdr") && ext != std::string("img"))
  {
    return false;
  }

  ifstream local_InputStream;
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

  // if the machine and file endianess are different
  // perform the byte swapping on it
  // this->m_ByteOrder = this->CheckAnalyzeEndian(this->m_hdr);
  // this->SwapHeaderBytesIfNecessary( &(this->m_hdr) );

  // The final check is to make sure that it is not a nifti version of the analyze file.
  // Eventually the entire class will be subsumed by the nifti reader.
  int tempNiftiValue = vtknifti1_io::is_nifti_file(fname);
  if (tempNiftiValue == 0)
  { // format = 2 anaylze files
    return 1;
  }
  else if (tempNiftiValue == 1)
  { // format = 1 nifti file
    return 0;
  }
  else if (tempNiftiValue == 2)
  { // format = 2 nifti files
    return 0;
  }
  else
  {
    return 0;
  }
  // return vtknifti1_io::is_nifti_file(fname) == 0;
}
