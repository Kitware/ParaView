/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRawStridedReader1.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRawStridedReader1.h"

#include "vtkByteSwap.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkExtentTranslator.h"
#include "vtkGridSampler1.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMetaInfoDatabase.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"

#include <iostream>
#include <fstream>
#include <time.h>
#include <sstream>
#include <string>
#include <vtkstd/vector>

vtkStandardNewMacro(vtkRawStridedReader1);

#define DEBUGPRINT_STRIDED_READER_DETAILS(arg)\
  ;
#define DEBUGPRINT_STRIDED_READER(arg)\
  ;
#define DEBUGPRINT_RESOLUTION(arg)\
  ;
#define DEBUGPRINT_METAINFORMATION(arg)\
  ;

//==============================================================================

class vtkRSRFileSkimmer1
{
 protected:

 public:
  // ctors and dtors
  vtkRSRFileSkimmer1();
  virtual ~vtkRSRFileSkimmer1();

  // Read a piece.  Extents, strides, and overall data dims must be set
  int read(ifstream& file, unsigned int* strides);

  // Grab the data array (output) pointer
  float* get_data() {return data_;}

  unsigned int get_data_size() {return data_size_;}

  void set_buffer_size(unsigned int buffer_size);
  void set_buffer_pointer(float *externalmem);
  void set_uExtents(unsigned int* extents);
  void set_dims(unsigned int* dims);
  void setup_timer() {use_timer_ = true;}
  void swap_endian();
 private:
  unsigned int alloc_data();
  // This prototype needs to change.  Just a placeholder
  unsigned int read_line(ifstream& file,
                         char* cache_buffer,
                         unsigned int buffer_size,
                         unsigned int stride,
                         unsigned int total_bytes,
                         unsigned int insert_at);

  bool SwapEndian_;
  unsigned int uExtents_[6];
  unsigned int stride_[3];
  unsigned int dims_[3];
  float* cache_buffer_;
  float* data_;
  float* buffer_pointer_;
  unsigned int output_dims_[3];
  unsigned int output_extents_[6];
  unsigned int buffer_size_;
  unsigned int data_size_;

  bool use_timer_;
  clock_t start;
  clock_t stop;
};


// Default ctor.  Some simple initialization
vtkRSRFileSkimmer1::vtkRSRFileSkimmer1() :
  SwapEndian_(false),
  cache_buffer_(NULL),
  data_(0),
  buffer_pointer_(NULL),
  use_timer_(false)
{ }

// dtor - handles data cleanup
vtkRSRFileSkimmer1::~vtkRSRFileSkimmer1()
{
  if (data_ && buffer_pointer_ != data_ )
  {
    delete[] data_;
    data_ = 0;
  }
  if(cache_buffer_ != NULL)
  {
    delete[] cache_buffer_;
    cache_buffer_ = NULL;
  }
}

void vtkRSRFileSkimmer1::swap_endian()
{
  if (SwapEndian_)
  {
    SwapEndian_ = false;
  }
  else
  {
    SwapEndian_ = true;
  }
}

void vtkRSRFileSkimmer1::set_buffer_size(unsigned int size)
{
  buffer_size_ = size;
}

void vtkRSRFileSkimmer1::set_buffer_pointer(float *external_mem)
{
  if (data_ && buffer_pointer_ != data_ )
    {
    delete[] data_;
    }
  buffer_pointer_ = external_mem;
  data_ = buffer_pointer_;
}

/*  Set the extents of the piece to grab, in scaled down coords.*/
void vtkRSRFileSkimmer1::set_uExtents(unsigned int* extents)
{
  for(int i = 0; i < 6; ++i)
    {
    uExtents_[i] = extents[i];
    }
}

/*  Set the dimensions of the dataset as a whole.  This is necessary
    to calculate the proper seeks and offsets that must be used */
void vtkRSRFileSkimmer1::set_dims(unsigned int* dims)
{
  for(int i = 0; i < 3; ++i)
    {
    dims_[i] = dims[i];
    }
}

/*  Allocate the final data array.  The data array will be the exact
    size necessary to accomodate all elements of the output.  Return
    the number of elements in the output (float) array. */
unsigned int vtkRSRFileSkimmer1::alloc_data()
{
  unsigned int i_span = uExtents_[1] - uExtents_[0] + 1;
  unsigned int j_span = uExtents_[3] - uExtents_[2] + 1;
  unsigned int k_span = uExtents_[5] - uExtents_[4] + 1;

  DEBUGPRINT_STRIDED_READER_DETAILS(
  cerr << "output dims: "
       << i_span << ", " << j_span << ", " << k_span << endl;
                           );

  data_size_ = i_span * j_span * k_span;
  if (data_ && buffer_pointer_ != data_ )
  {
    delete[] data_;
  }

  if (buffer_pointer_ != NULL)
    {
    data_ = buffer_pointer_;
    }
  else
    {
    data_ = new float[data_size_];
    if (data_ == 0)
      {
      cerr << "NEW FAILURE" << endl;
      }
    }

  if (cache_buffer_ != NULL)
    {
    //cerr << "Free old cache_buffer_" << endl;
    delete[] cache_buffer_;
    }
  cache_buffer_ = new float[buffer_size_/sizeof(float)];
  if (cache_buffer_ == 0)
    {
    cerr << "NEW FAILURE" << endl;
    }

  return data_size_;
}

/*  Read a scanline of the data in the dimension changing fastest. */
unsigned int vtkRSRFileSkimmer1::read_line(ifstream& file,
                              char* cache_buffer,
                              unsigned int buffer_size,
                              unsigned int stride,
                              unsigned int vtkNotUsed(bytes_to_read),
                              unsigned int insert_at)
{
  DEBUGPRINT_STRIDED_READER_DETAILS(
    cerr << "stride " << stride << " B2R " << bytes_to_read << endl;
                                    );

  unsigned int read_from = 0;
  unsigned int bytes_read = 0;
  unsigned int vals_read = 0;

  unsigned int vals_in_buffer = buffer_size / sizeof(float);//#bytes to #floats

  //adjust buffer down to hold a whole number of floats
  unsigned int adjusted_buffer_size = buffer_size;
  if (vals_in_buffer*sizeof(float) < buffer_size)
    {
    adjusted_buffer_size = vals_in_buffer*sizeof(float);
    DEBUGPRINT_STRIDED_READER_DETAILS(
    cerr << "Round " << buffer_size << " to " << adjusted_buffer_size << endl;
                                      );
    }
  unsigned int strided_vals_in_buffer = vals_in_buffer/stride;

  //check for case when stride is bigger than buffer.
  //In that case adjust truncated fraction from 0 to 1
  if (strided_vals_in_buffer <= 1)
    {
    DEBUGPRINT_STRIDED_READER_DETAILS(
    cerr << "Floor to " << 1 << endl;
                                      );
    strided_vals_in_buffer = 1;
    adjusted_buffer_size = strided_vals_in_buffer*stride*sizeof(float);
    vals_in_buffer = strided_vals_in_buffer*stride;
    }

  //check for case when buffer is bigger than the number of vals we want.
  //In that case, read only what we want.
  unsigned int sought_vals = (uExtents_[1]-uExtents_[0]+1);
  if (strided_vals_in_buffer > sought_vals)
    {
    DEBUGPRINT_STRIDED_READER_DETAILS(
    cerr << "Ceiling to " << sought_vals << endl;
                                      );
    strided_vals_in_buffer = sought_vals;
    adjusted_buffer_size = strided_vals_in_buffer*stride*sizeof(float);
    vals_in_buffer = strided_vals_in_buffer*stride;
    }

  // If the number of reads per buffer is 1, then we know our stride
  // is larger than the buffer size.  This means that the most efficient
  // way to read is to seek to the value and do a buffered read.
  if (strided_vals_in_buffer == 1)
    {
    DEBUGPRINT_STRIDED_READER_DETAILS(
    cerr << "single reads " << endl;
                                      );
    //ifstream::pos_type orig = file.tellg();
    //unsigned int c=orig;
    while(vals_read < sought_vals)
      {
      DEBUGPRINT_STRIDED_READER_DETAILS(
      cerr << "READ AT " << file.tellg();
                                        );
      file.read(cache_buffer, adjusted_buffer_size);
      if (file.bad())
        {
        cerr << "READ FAIL 1" << endl;
        }
      float* float_buffer = cache_buffer_;
      // Since we do a seek, the element will always be at location 0
      data_[insert_at] = float_buffer[0];
      DEBUGPRINT_STRIDED_READER_DETAILS(
      cerr << " " << data_[insert_at] << " ";
                                        );
      insert_at++;
      /*
      // Seek to the next value we want to read!
      c+=stride*sizeof(float);
      DEBUGPRINT_STRIDED_READER_DETAILS(
      cerr << " seek to " << c << endl;
                                        );
      */
      file.seekg(stride*sizeof(float), ios::cur);//c, ios::beg); //); was buggy
      if (file.bad())
        {
        cerr << "SEEK FAIL" << endl;
        }

      // The number of bytes we read now is not the size of the cache,
      // but it's the size of the stride (in bytes).
      bytes_read += sizeof(float);
      vals_read++;
      }
    }
  else
    {
    DEBUGPRINT_STRIDED_READER_DETAILS(
    cerr << "Read " << sought_vals << " values from "
         << adjusted_buffer_size << " byte buffers that hold "
         << strided_vals_in_buffer << endl;
                                      );
    //Like above we might have a buffer which doesn't cover a whole line,
    //so we loop, reading buffers until we get all the values for the line.
    while(vals_read < sought_vals)
      {
      //read a buffer
      DEBUGPRINT_STRIDED_READER_DETAILS(
      cerr << "READ AT " << file.tellg() << endl;
                                        );
      if (stride == 1)
        {
        DEBUGPRINT_STRIDED_READER_DETAILS(
        cerr << "SHORT CUT " << endl;
                                          );

        file.read((char*)&data_[insert_at], adjusted_buffer_size);
        if (file.bad())
          {
          cerr << "READ FAIL 2" << endl;
          }
        unsigned int neededVals = (sought_vals < vals_in_buffer)?sought_vals:vals_in_buffer;
        unsigned int neededBytes = neededVals*sizeof(float);
        insert_at += neededVals;
        read_from += neededVals;
        bytes_read += neededBytes;
        vals_read += neededVals;
        }
      else
        {
        file.read(cache_buffer, adjusted_buffer_size);
        if (file.bad())
          {
          cerr << "READ FAIL 3" << endl;
          }
        float* float_buffer = cache_buffer_;
        // Unlike above we now have more than 1 vals in each buffer,
        // so we need to step through and extract the values that lie on
        // the stride
        while(read_from < vals_in_buffer)
          {
          data_[insert_at] = float_buffer[read_from];
          DEBUGPRINT_STRIDED_READER_DETAILS(
                                            cerr << "fbuffer[" << read_from << "]="
                                            << data_[insert_at] << "-> " << insert_at << endl;
                                            );
          insert_at++;
          read_from+=stride;
          vals_read++;
          bytes_read += sizeof(float);
          //multiple small buffers can overrun, so detect and finish
          if (vals_read == sought_vals)
            {
            break;
            }
          }
        }
      //when loop, we careful to start at right place in buffer
      read_from = read_from % vals_in_buffer;
      }
    }

  DEBUGPRINT_STRIDED_READER_DETAILS(
  cerr << "Bytes read " << bytes_read << endl;
                                    );
  return insert_at;
}


/*  Read a piece off disk.  This must be done AFTER definiing the
    extents and dimensions.  The read also assumes that the data
    is organized on disk with the fastest changing dimension
    specified first, followed sequential by the 2 next fastest
    dimensions.  Here, it is organized as row-major, x,y,z format.
*/
int vtkRSRFileSkimmer1::read(ifstream& file,
                 unsigned int* strides)
{

  if(use_timer_)
  {
    start = clock();
  }

  for(int i = 0; i < 3; ++i)
  {
    if(strides[i] == 0)
      {
      cerr << "Cannot read a piece with a stride of 0." << endl;
      return 0;
      }
    stride_[i] = strides[i];
  }
  if (buffer_size_ < sizeof(float))
    {
    cerr << "buffer size must be a multiple of " << sizeof(float) << endl;
    return 0;
    }
  //double t0 = clock();
  alloc_data();
  //double t1 = clock();

  //cerr << "ALLOC T=" << (t1-t0)/CLOCKS_PER_SEC << endl;
  unsigned int insert_index = 0;

  // Size in floats!  We have to multiply by sizeof(float) to get the bytes.
  // But that will happen just a little bit later!
  unsigned int plane_size = dims_[1] * dims_[0];
  unsigned int row_size = dims_[0];
  unsigned int bytes_to_read = (uExtents_[1]-uExtents_[0]+1) * sizeof(float);
  DEBUGPRINT_STRIDED_READER_DETAILS(
  cerr << "plane size = " << plane_size << " row size = " << row_size << " b2r = " << bytes_to_read << endl;
                           );

  for(unsigned int k = uExtents_[4]; k <= uExtents_[5]; k++)
    {
    for(unsigned int j = uExtents_[2]; j <= uExtents_[3]; j++)
      {
      unsigned int i = uExtents_[0];
      unsigned int offset =
        k*strides[2]*plane_size*sizeof(float) +
        j*strides[1]*row_size*sizeof(float) +
        i*strides[0]*sizeof(float);

      DEBUGPRINT_STRIDED_READER_DETAILS(
        cerr << "read line "
        << k << "," << j << "," << i << " "
        << "(" << k*strides[2] <<","<<j*strides[1] <<","<<i*strides[0] << ")"
        << " from " << offset << endl;
                                        );
      // Seek to the beginning of the line we want to extract.
      // The file pointer is now at the first element of the
      // extent to extract.
      file.seekg(offset, ios::beg);
      if (file.bad())
        {
        cerr << "SEEK FAIL" << endl;
        return 0;
        }

      // Extract the line.  To do this we need to know the stride
      // and the last extent to grab, as well as the location
      // of the output array and the position to put stuff in there.
      insert_index =
        read_line(file,
                  (char*)cache_buffer_, buffer_size_,
                  strides[0],
                  bytes_to_read,
                  insert_index);
      }
    //double tn = clock();
    //cerr << "SLICET=" << (tn-t1)/CLOCKS_PER_SEC << endl;
    //t1 = tn;
    }
  DEBUGPRINT_STRIDED_READER_DETAILS(
    cerr << "Read " << insert_index << " floats total " << endl;
                                    );

  if(use_timer_)
    {
    stop = clock();
    double t = stop - start;
    double elapsed = t / CLOCKS_PER_SEC;
    cerr << "Took " << elapsed << " seconds to read." << endl;
    }

  if (SwapEndian_)
    {
    vtkByteSwap::SwapVoidRange(data_, insert_index , sizeof(float) );
    }
  return 1;
}

//============================================================================

vtkRawStridedReader1::vtkRawStridedReader1()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Skimmer = new vtkRSRFileSkimmer1();
  this->Filename = NULL;
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = 99;
  this->Dimensions[0] = this->WholeExtent[1] - this->WholeExtent[0] +1;
  this->Dimensions[1] = this->WholeExtent[3] - this->WholeExtent[2] +1;
  this->Dimensions[2] = this->WholeExtent[5] - this->WholeExtent[4] +1;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 1.0;
  this->Stride[0] = this->Stride[1] = this->Stride[2] = 1;
  this->BlockReadSize = 1*sizeof(float)*1024*1024;

  this->RangeKeeper = vtkMetaInfoDatabase::New();

  this->GridSampler = vtkGridSampler1::New();
  this->Resolution = 1.0;
  this->SI = 1;
  this->SJ = 1;
  this->SK = 1;

  this->PostPreProcess = false;
}

//----------------------------------------------------------------------------
vtkRawStridedReader1::~vtkRawStridedReader1()
{
  if (this->Filename)
    {
    delete[] this->Filename;
    }
  delete this->Skimmer;
  this->RangeKeeper->Delete();
  this->GridSampler->Delete();
}

//----------------------------------------------------------------------------
void vtkRawStridedReader1::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


//----------------------------------------------------------------------------
void vtkRawStridedReader1::SwapDataByteOrder(int i)
{
  if (i == 1)
    {
    this->Skimmer->swap_endian();
    }
}

//----------------------------------------------------------------------------
//RequestInformation supplies global meta information
// Global Extents  (integer count range of point count in x,y,z)
// Global Origin
// Global Spacing (should be the stride value * original)

int vtkRawStridedReader1::RequestInformation(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkDataObject::ORIGIN(),this->Origin,3);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent ,6);
  int sWholeExtent[6];
  sWholeExtent[0] = this->WholeExtent[0];
  sWholeExtent[1] = this->WholeExtent[1];
  sWholeExtent[2] = this->WholeExtent[2];
  sWholeExtent[3] = this->WholeExtent[3];
  sWholeExtent[4] = this->WholeExtent[4];
  sWholeExtent[5] = this->WholeExtent[5];

  this->Dimensions[0] = this->WholeExtent[1] - this->WholeExtent[0]+1;
  this->Dimensions[1] = this->WholeExtent[3] - this->WholeExtent[2]+1;
  this->Dimensions[2] = this->WholeExtent[5] - this->WholeExtent[4]+1;

  outInfo->Set(vtkDataObject::SPACING(), this->Spacing, 3);
  double sSpacing[3];
  sSpacing[0] = this->Spacing[0];
  sSpacing[1] = this->Spacing[1];
  sSpacing[2] = this->Spacing[2];
/*
  this->SI = 1;
  this->SJ = 1;
  this->SK = 1;
*/
  this->Resolution = 1.0;

  DEBUGPRINT_RESOLUTION(
  cerr << "PRE GRID\t";
  {for (int i = 0; i < 3; i++) cerr << this->Spacing[i] << " ";}
  {for (int i = 0; i < 6; i++) cerr << this->WholeExtent[i] << " ";}
  cerr << endl;
  );

  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION()))
    {
    double rRes =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
    int strides[3];
    double aRes;
    int pathLen;
    int *splitPath;

    this->GridSampler->SetWholeExtent(sWholeExtent);
    vtkIntArray *ia = this->GridSampler->GetSplitPath();
    pathLen = ia->GetNumberOfTuples();
    splitPath = ia->GetPointer(0);
    DEBUGPRINT_RESOLUTION(
    cerr << "pathlen = " << pathLen << endl;
    cerr << "SP = " << splitPath << endl;
    for (int i = 0; i <40 && i < pathLen; i++)
      {
      cerr << splitPath[i] << " ";
      }
    cerr << endl;
    );
    //save split path in translator
    vtkImageData *outData = vtkImageData::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));
    vtkExtentTranslator *et = outData->GetExtentTranslator();
    et->SetSplitPath(pathLen, splitPath);

    this->GridSampler->SetSpacing(sSpacing);
    this->GridSampler->ComputeAtResolution(rRes);

    this->GridSampler->GetStridedExtent(sWholeExtent);
    this->GridSampler->GetStridedSpacing(sSpacing);
    this->GridSampler->GetStrides(strides);
    aRes = this->GridSampler->GetStridedResolution();

    DEBUGPRINT_RESOLUTION(
    cerr << "PST GRID\t";
    {for (int i = 0; i < 3; i++) cerr << sSpacing[i] << " ";}
    {for (int i = 0; i < 6; i++) cerr << sWholeExtent[i] << " ";}
    cerr << endl;
    );

    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), sWholeExtent, 6);
    outInfo->Set(vtkDataObject::SPACING(), sSpacing, 3);

    this->Resolution = aRes;
    this->SI = strides[0];
    this->SJ = strides[1];
    this->SK = strides[2];
    DEBUGPRINT_RESOLUTION(
    cerr << "RI SET STRIDE ";
    {for (int i = 0; i < 3; i++) cerr << strides[i] << " ";}
    cerr << endl;
    );
  }

  double bounds[6];
  bounds[0] = this->Origin[0] + sSpacing[0] * sWholeExtent[0];
  bounds[1] = this->Origin[0] + sSpacing[0] * sWholeExtent[1];
  bounds[2] = this->Origin[1] + sSpacing[1] * sWholeExtent[2];
  bounds[3] = this->Origin[1] + sSpacing[1] * sWholeExtent[3];
  bounds[4] = this->Origin[2] + sSpacing[2] * sWholeExtent[4];
  bounds[5] = this->Origin[2] + sSpacing[2] * sWholeExtent[5];
/*
  {for (int i = 0; i < 6; i++) cerr << bounds[i] << " ";}
  cerr << endl;
*/
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),
               bounds, 6);

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);
  return 1;
}

//----------------------------------------------------------------------------
// Here unlike the RequestInformation we getting, not setting
int vtkRawStridedReader1::RequestUpdateExtent(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
               this->UpdateExtent);
  DEBUGPRINT_STRIDED_READER(
  int P = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int NP = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  cerr << "RSR(" << this << ") Strided uExt "
       << P << "/" << NP << " = "
       << this->UpdateExtent[0] << ".." << this->UpdateExtent[1] << ","
       << this->UpdateExtent[2] << ".." << this->UpdateExtent[3] << ","
       << this->UpdateExtent[4] << ".." << this->UpdateExtent[5] << endl;
                           );
  return 1;
}

//----------------------------------------------------------------------------
int vtkRawStridedReader1::RequestData(
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector),
    vtkInformationVector* outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *outData = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!outData)
    {
    cerr << "Wrong output type" << endl;
    return 0;
    }
  if (!this->Filename)
    {
    cerr << "Must specify filename" << endl;
    return 0;
    }
  outData->Initialize();

  //double start = clock();

  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION()))
    {
    this->Resolution = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
    }

  vtkInformation *dInfo = outData->GetInformation();
  dInfo->Set(vtkDataObject::DATA_RESOLUTION(), this->Resolution);

  //DEBUGPRINT_RESOLUTION(
  //{
  //double res = 1.0;
  //if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION()))
  //  {
  //  res = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
  //  }
  int P = outInfo->Get(
                       vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int NP = outInfo->Get(
                        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  //cerr << P << "/" << NP << "@" << res
  //     << "->" << this->SI << " " << this->SJ << " " << this->SK << endl;
  //}
  //);

  //prepping to produce real data and thus allocate real amounts of space
  int *uext = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  outData->SetExtent(uext);

  //Todo We need to be able to have user definable data type
  //Todo also multiple arrays/multiple scalarComponents
  outData->AllocateScalars();
  outData->GetPointData()->GetScalars()->SetName("PointCenteredData");
  float *myfloats = (float*)outData->GetScalarPointer();
//  double c_alloc = clock();

  if (uext[1] < uext[0] ||
      uext[3] < uext[2] ||
      uext[5] < uext[4])
    {
    return 1;
    }

  char pfilename[256];
  sprintf(pfilename, "%s.%d_%d_%d_%d_%d_%d_%d_%d_%f.block", this->Filename,
          uext[0], uext[1], uext[2], uext[3], uext[4], uext[5], P, NP,
          this->Resolution);
  ifstream pfile(pfilename, ios::in|ios::binary);
  if(!pfile.is_open())
  {
    ifstream file(this->Filename, ios::in|ios::binary);
    if(!file.is_open())
    {
      cerr << "Could not open file: " << this->Filename << endl;
      return 0;
    }
    if (file.bad())
    {
      cerr << "OPEN FAIL" << endl;
      return 0;
    }

    this->Skimmer->set_uExtents((unsigned int*)uext);
    this->Skimmer->set_dims((unsigned int*)this->Dimensions);
    this->Skimmer->set_buffer_size((unsigned int)this->BlockReadSize);
    this->Skimmer->set_buffer_pointer(myfloats);

    unsigned int stride[3];
    stride[0] = this->SI;
    stride[1] = this->SJ;
    stride[2] = this->SK;

    //double c_prep = clock();

    if (!this->Skimmer->read(file, stride))
      {
      cerr << "READ FAIL 3" << endl;
      return 0;
    }

    DEBUGPRINT_STRIDED_READER
      ({
      unsigned int memsize = this->Skimmer->get_data_size();
      cerr << "memsize " << memsize << endl;
      });
    file.close();

    if (this->PostPreProcess)
      {
      //cerr << "creating " << pfilename << " " << endl;
      ofstream opfile(pfilename, ios::out|ios::binary);
      unsigned int memsize = (uext[1]-uext[0]+1)*(uext[3]-uext[2]+1)*(uext[5]-uext[4]+1);
      opfile.write((char*)myfloats, memsize*sizeof(float));
      opfile.close();
      }
  }
  else
  {
    //cerr << "reading from " << pfilename << " " << endl;
    unsigned int memsize = (uext[1]-uext[0]+1)*(uext[3]-uext[2]+1)*(uext[5]-uext[4]+1);
    pfile.read((char*)myfloats, memsize*sizeof(float));
    if (pfile.bad())
      {
      cerr << "READ FAIL 1" << endl;
      }
    pfile.close();
  }

//  double c_fileio = clock();

  double range[2];
  outData->GetPointData()->GetScalars()->GetRange(range);
  //int P = outInfo->Get(
  //  vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  //int NP = outInfo->Get(
  //  vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  DEBUGPRINT_METAINFORMATION(
  cerr << "RSR(" << this << ") Calculate range " << range[0] << ".." << range[1] << " for " << P << "/" << NP << endl;
                             );
  this->RangeKeeper->Insert(P, NP, uext, this->Resolution,
                            0, "PointCenteredData", 0,
                            range);

/*  double stop = clock();
  double talloc = (c_alloc-start) / CLOCKS_PER_SEC;
  double t_prep = (c_prep-c_alloc)/CLOCKS_PER_SEC;
  double t_file = (c_fileio-c_prep)/CLOCKS_PER_SEC;
  double t_meta = (stop-c_fileio)/CLOCKS_PER_SEC;
  double ttotal = stop - start;
  double elapsed = ttotal / CLOCKS_PER_SEC;
  cerr
    << P << "/" << NP << "=(" << uext[0] << "," << uext[1] << "," << uext[2] << "," << uext[3] << "," << uext[4] << "," << uext[5] << ")"
    << "@" << this->Resolution << "=(" << this->SI << "," << this->SJ << "," << this->SK << ")"
    << " " << talloc << ":" << t_prep << ":" << t_file << ":" << t_meta << " " << elapsed << " seconds" << endl;
*/

  char message[100];
  sprintf(message, "READ %d/%d@%f %d %d %d %ld KB",  P,NP, this->Resolution, this->SI, this->SJ, this->SK, outData->GetActualMemorySize() );
  vtkTimerLog::MarkEvent(message);

  return 1;
}

//----------------------------------------------------------------------------
int vtkRawStridedReader1::ProcessRequest(vtkInformation *request,
                   vtkInformationVector **inputVector,
                   vtkInformationVector *outputVector)
{
  DEBUGPRINT_STRIDED_READER(
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  int P = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int NP = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  double res = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
  cerr << "RSR(" << this << ") PR " << P << "/" << NP << "@" << res << "->"
     << this->SI << " " << this->SJ << " " << this->SK << endl;
  );

  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    DEBUGPRINT_STRIDED_READER(cerr << "RSR(" << this << ") RDO =====================================" << endl;);
    }

  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    DEBUGPRINT_STRIDED_READER(cerr << "RSR(" << this << ") RI =====================================" << endl;);
    }

  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    DEBUGPRINT_STRIDED_READER(cerr << "RSR(" << this << ") RUE =====================================" << endl;);
    }

  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_RESOLUTION_PROPAGATE()))
    {
    DEBUGPRINT_METAINFORMATION(cerr << "RSR(" << this << ") RRP =====================================" << endl;);
    }

  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT_INFORMATION()))
    {
    DEBUGPRINT_METAINFORMATION(
    cerr << "RSR(" << this << ") RUEI =====================================" << endl;
    );
    //create meta information for this piece
    double *origin;
    double *spacing;
    int *ext;
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    origin = outInfo->Get(vtkDataObject::ORIGIN());
    spacing = outInfo->Get(vtkDataObject::SPACING());
    ext = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    int P = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    int NP = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    double bounds[6];
    bounds[0] = origin[0] + spacing[0] * ext[0];
    bounds[1] = origin[0] + spacing[0] * ext[1];
    bounds[2] = origin[1] + spacing[1] * ext[2];
    bounds[3] = origin[1] + spacing[1] * ext[3];
    bounds[4] = origin[2] + spacing[2] * ext[4];
    bounds[5] = origin[2] + spacing[2] * ext[5];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::PIECE_BOUNDING_BOX(), bounds, 6);

    int ic = (ext[1]-ext[0]);
    if (ic < 1)
      {
      ic = 1;
      }
    int jc = (ext[3]-ext[2]);
    if (jc < 1)
      {
      jc = 1;
      }
    int kc = (ext[5]-ext[4]);
    if (kc < 1)
      {
      kc = 1;
      }
    outInfo->Set
      (vtkStreamingDemandDrivenPipeline::ORIGINAL_NUMBER_OF_CELLS(), ic*jc*kc);

/*
    cerr << P << "/" << NP << "\t";
    {for (int i = 0; i < 3; i++) cerr << spacing[i] << " ";}
    cerr << "\t";
    {for (int i = 0; i < 6; i++) cerr << ext[i] << " ";}
    cerr << "\t";
    {for (int i = 0; i < 6; i++) cerr << bounds[i] << " ";}
    cerr << endl;
*/
    vtkInformationVector *miv = outInfo->Get(vtkDataObject::POINT_DATA_VECTOR());
    vtkInformation *fInfo = miv->GetInformationObject(0);
    if (!fInfo)
      {
      fInfo = vtkInformation::New();
      miv->SetInformationObject(0, fInfo);
      fInfo->Delete();
      }
    const char *name = "PointCenteredData";
    double range[2];
    if (this->RangeKeeper->Search(P, NP, ext,
                                  0, name, 0,
                                  range))
      {
      DEBUGPRINT_METAINFORMATION
        (
         cerr << "Range for " << name << " "
         << P << "/" << NP << " "
         << ext[0] << "," << ext[1] << ","
         << ext[2] << "," << ext[3] << ","
         << ext[4] << "," << ext[5] << " is "
         << range[0] << " .. " << range[1] << endl;
         );
      fInfo->Set(vtkDataObject::FIELD_ARRAY_NAME(), name);
      fInfo->Set(vtkDataObject::PIECE_FIELD_RANGE(), range, 2);
      }
    else
      {
      DEBUGPRINT_METAINFORMATION
        (
         cerr << "No range for " << name << " "
         << ext[0] << "," << ext[1] << ","
         << ext[2] << "," << ext[3] << ","
         << ext[4] << "," << ext[5] << " yet" << endl;
         );
      fInfo->Remove(vtkDataObject::FIELD_ARRAY_NAME());
      fInfo->Remove(vtkDataObject::PIECE_FIELD_RANGE());
      }
    }

  //This is overridden just to intercept requests for debugging purposes.
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    DEBUGPRINT_STRIDED_READER(cerr << "RSR(" << this << ") RD =====================================" << endl;);

    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    int updateExtent[6];
    int wholeExtent[6];
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      updateExtent);
    outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
      wholeExtent);
    double res = this->Resolution;
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION()))
      {
      res = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
      }
    bool match = true;
    for (int i = 0; i< 6; i++)
      {
        if (updateExtent[i] != wholeExtent[i])
          {
          match = false;
          }
      }
    if (match && (res == 1.0))
      {
      vtkErrorMacro("Whole extent requested, streaming is not working.");
      }
    }

  int rc = this->Superclass::ProcessRequest(request, inputVector, outputVector);
  return rc;
}
