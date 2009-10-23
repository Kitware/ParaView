/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRawStridedReader2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRawStridedReader2.h"

#include "vtkAdaptiveOptions.h"
#include "vtkByteSwap.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkExtentTranslator.h"
#include "vtkGridSampler2.h"
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

#ifndef _WIN32
#include <sys/mman.h>
#include <errno.h>
#endif

#include "vtkAdaptiveOptions.h"

vtkCxxRevisionMacro(vtkRawStridedReader2, "1.4");
vtkStandardNewMacro(vtkRawStridedReader2);

#if 0

#define DEBUGPRINT_STRIDED_READER_DETAILS(arg)\
  ;

#define DEBUGPRINT_STRIDED_READER(arg)\
  arg;

#define DEBUGPRINT_RESOLUTION(arg)\
  arg;

#define DEBUGPRINT_METAINFORMATION(arg)\
  arg;

#else

#define DEBUGPRINT_STRIDED_READER_DETAILS(arg)\
  if (false && vtkAdaptiveOptions::GetEnableStreamMessages()) \
    { \
      arg;\
    }

#define DEBUGPRINT_STRIDED_READER(arg)\
  if (vtkAdaptiveOptions::GetEnableStreamMessages()) \
    { \
      arg;\
    }

#define DEBUGPRINT_RESOLUTION(arg)\
  if (vtkAdaptiveOptions::GetEnableStreamMessages()) \
    { \
      arg;\
    }

#define DEBUGPRINT_METAINFORMATION(arg)\
  if (vtkAdaptiveOptions::GetEnableStreamMessages()) \
    { \
      arg;\
    }
#endif

#define MAPSIZE (1024 * 1024 * 1024) 

//==============================================================================

class vtkRSRFileSkimmer2
{
 protected:

 public:
  // ctors and dtors
  vtkRSRFileSkimmer2();
  virtual ~vtkRSRFileSkimmer2();

  // Read a piece.  Extents, strides, and overall data dims must be set
  int read(FILE* fp, unsigned int* strides, vtkRawStridedReader2* reader);

  // Grab the data array (output) pointer
  float* get_data() {return data_;}

  unsigned int get_data_size() {return data_size_;}
  
  void set_buffer_size(unsigned int buffer_size);
  void set_buffer_pointer(float *externalmem);
  void set_uExtents(unsigned int* extents);
  void set_sWholeExtent(int* extents);
  void set_dims(unsigned int* dims);
  void setup_timer() {use_timer_ = true;}
  void swap_endian();
 private:
  unsigned int alloc_data();
  
  bool SwapEndian_;
  unsigned int uExtents_[6];
  unsigned int sWholeExtent_[6];
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
vtkRSRFileSkimmer2::vtkRSRFileSkimmer2() :
  data_(0), 
  cache_buffer_(NULL), 
  use_timer_(false), 
  SwapEndian_(false), 
  buffer_pointer_(NULL)
{ }

// dtor - handles data cleanup
vtkRSRFileSkimmer2::~vtkRSRFileSkimmer2()
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

void vtkRSRFileSkimmer2::swap_endian()
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

void vtkRSRFileSkimmer2::set_buffer_size(unsigned int size)
{
  buffer_size_ = size;
}

void vtkRSRFileSkimmer2::set_buffer_pointer(float *external_mem)
{
  if (data_ && buffer_pointer_ != data_ )
    {
    delete[] data_;
    }
  buffer_pointer_ = external_mem;
  data_ = buffer_pointer_;
}

/*  Set the extents of the piece to grab, in scaled down coords.*/
void vtkRSRFileSkimmer2::set_uExtents(unsigned int* extents)
{
  for(int i = 0; i < 6; ++i)
    {
    uExtents_[i] = extents[i];
    }
}

void vtkRSRFileSkimmer2::set_sWholeExtent(int* extents)
{
  for(int i = 0; i < 6; ++i)
    {
    sWholeExtent_[i] = extents[i];
    }
}

/*  Set the dimensions of the dataset as a whole.  This is necessary
    to calculate the proper seeks and offsets that must be used */
void vtkRSRFileSkimmer2::set_dims(unsigned int* dims)
{
  for(int i = 0; i < 3; ++i)
    {
    dims_[i] = dims[i];
    }
}

/*  Allocate the final data array.  The data array will be the exact
    size necessary to accomodate all elements of the output.  Return
    the number of elements in the output (float) array. */
unsigned int vtkRSRFileSkimmer2::alloc_data()
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

/*  Read a piece off disk.  This must be done AFTER definiing the
    extents and dimensions.  The read also assumes that the data
    is organized on disk with the fastest changing dimension
    specified first, followed sequential by the 2 next fastest
    dimensions.  Here, it is organized as row-major, x,y,z format.
*/
int vtkRSRFileSkimmer2::read(FILE* fp,
                             unsigned int* strides, vtkRawStridedReader2* reader)
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
  DEBUGPRINT_STRIDED_READER_DETAILS
    (cerr << "plane size = " 
     << plane_size << " row size = " << row_size 
     << " b2r = " << bytes_to_read << endl;);
  
  size_t ir = uExtents_[1] - uExtents_[0] + 1;
  size_t jr = uExtents_[3] - uExtents_[2] + 1;
  size_t kr = uExtents_[5] - uExtents_[4] + 1;  
  
  size_t is = ir;
  size_t ijs = ir * jr;
  
  size_t js = (sWholeExtent_[1] - sWholeExtent_[0] + 1);
  size_t ks = (sWholeExtent_[1] - sWholeExtent_[0] + 1) * (sWholeExtent_[3] - sWholeExtent_[2] + 1);

#ifndef _WIN32
  float* map = reader->SetupMap(0);
  if(map != MAP_FAILED) {
    for(size_t k = 0; k < kr; k++) 
      {
      for(size_t j = 0; j < jr; j++) 
        {
        for(size_t i = 0; i < ir; i++) 
          {
          size_t di = i + j * is + k * ijs; 
          
          size_t index = (i + uExtents_[0]) +
            (j + uExtents_[2]) * js + (k + uExtents_[4]) * ks;
          
          // damn OS X Leopard bug doesn't allow mmaps bigger than 1 GB
          // so have to chunk the damn mmap into separate 1 GB segments
          map = reader->SetupMap(index / (MAPSIZE / sizeof(float)));
          if(map != MAP_FAILED) 
            {
            this->data_[di] = map[index % (MAPSIZE / sizeof(float))];
            }
          else 
            {
            fseek(fp, index * sizeof(float), SEEK_SET);
            fread(&(this->data_[di]), sizeof(float), 1, fp);
            }
          }
        }
      }
  }
  else 
  {
#endif
    for(size_t k = 0; k < kr; k++) 
      {
      for(size_t j = 0; j < jr; j++) 
        {
        size_t di = j * is + k * ijs; 
        fseek(fp, 
              (uExtents_[0] +
               (j + uExtents_[2]) * js + (k + uExtents_[4]) * ks) *
              sizeof(float), SEEK_SET);
        fread(&(this->data_[di]), sizeof(float), ir, fp);
        }
      }
#ifndef _WIN32
  }
#endif

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

void vtkRawStridedReader2::SetupFile() {
  // this is a static hack, this should be available in the pipeline
  int height = vtkAdaptiveOptions::GetHeight();

  int newfile = 1;

  // figure out the index
  vtkIdType stop = 
    (vtkIdType)(height * (1.0 - this->Resolution)+0.5);
  
  // try to set up the file
  // there may be a better way of detecting this in VTK, yes?
  // with the modified flag... basically want to keep the 
  // file and mmaps open until the filename changes
  if(this->lastname) {
    if(this->lastresolution != stop ||
       strcmp(this->lastname, this->Filename)) {
#ifndef _WIN32
      this->TearDownMap();
#endif
      this->TearDownFile();
    }
    else {
      newfile = 0;
    }
  }

  this->lastresolution = stop;

  if(newfile) {
    // copy the filename
    this->lastname = new char[strlen(this->Filename) + 255];
    if(stop > 0) {
      sprintf(this->lastname, "%s-%d", this->Filename, stop);
    }
    else {
      strcpy(this->lastname, this->Filename);
    }

    // open the files
    this->fp = fopen(this->lastname, "r");
    
    // remember the base filename
    strcpy(this->lastname, this->Filename);

    if(!this->fp) {
      delete [] this->lastname;
      this->lastname = 0;

      return;
    }

    this->fd = fileno(this->fp);        
  }
}

void vtkRawStridedReader2::TearDownFile() {
  // clean up files
  if(this->fp) {
    fclose(this->fp);
  }

  if(this->lastname) {
    delete [] this->lastname;
  }

  this->lastname = 0;
  this->fp = 0;
  this->fd = -1;
}

#ifndef _WIN32
float* vtkRawStridedReader2::SetupMap(size_t which) {
  // make sure it's the right map
  if(which != this->chunk) {
    this->TearDownMap();

    this->chunk = which;
    
    // try to set up a memory map
    size_t pagesize = getpagesize();
    fseek(this->fp, 0, SEEK_END);
    size_t filesize = ftell(this->fp);
    fseek(this->fp, 0, SEEK_SET);
    
    filesize = filesize % pagesize > 0 ? filesize + 
      + (pagesize - filesize % pagesize) : filesize;
    
    // damn OS X Leopard bug, it won't let mmaps bigger than 1 GB on Intel
    // so gotta chop it into 1 GB pages
    if(filesize > MAPSIZE) {
      this->mapsize = MAPSIZE;
      this->map = (float*)mmap(0, MAPSIZE, PROT_READ, MAP_SHARED, 
                               fd, which * MAPSIZE);
    }
    else {
      this->mapsize = filesize;
      this->map = (float*)mmap(0, filesize, PROT_READ, MAP_SHARED, 
                               fd, 0);
    }    
    
    if(this->map == MAP_FAILED) {
      cerr << "Memory map failed: " << strerror(errno) << "." << endl;
      this->chunk = -1;
    }
  }

  return this->map;
}

void vtkRawStridedReader2::TearDownMap() {
  // cleanup memory maps
  if(this->map != MAP_FAILED) {
    if(munmap(this->map, this->mapsize)) {
      cerr << "Memory unmap failed: " << strerror(errno) << "." << endl;
    }
  }

  this->chunk = -1;
  this->map = (float*)MAP_FAILED;
}
#endif

//============================================================================

vtkRawStridedReader2::vtkRawStridedReader2()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  this->Skimmer = new vtkRSRFileSkimmer2();
  this->Filename = NULL;
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = 99;
  this->Dimensions[0] = this->WholeExtent[1] - this->WholeExtent[0] + 1;
  this->Dimensions[1] = this->WholeExtent[3] - this->WholeExtent[2] + 1;
  this->Dimensions[2] = this->WholeExtent[5] - this->WholeExtent[4] + 1;
  this->sWholeExtent[0] = this->sWholeExtent[2] = this->sWholeExtent[4] = 0;
  this->sWholeExtent[1] = this->sWholeExtent[3] = this->sWholeExtent[5] = 99;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 1.0;
  this->Stride[0] = this->Stride[1] = this->Stride[2] = 1;
  this->BlockReadSize = 1*sizeof(float)*1024*1024;

  this->RangeKeeper = vtkMetaInfoDatabase::New();

  this->GridSampler = vtkGridSampler2::New();
  this->Resolution = 1.0;
  this->SI = 1;
  this->SJ = 1;
  this->SK = 1;

  this->fp = 0;
  this->fd = -1;
  this->lastname = 0;

#ifndef _WIN32
  this->chunk = -1;
  this->map = (float*)MAP_FAILED;
#endif
}

//----------------------------------------------------------------------------
vtkRawStridedReader2::~vtkRawStridedReader2()
{
  if (this->Filename)
    {
    delete[] this->Filename;
    }
  delete this->Skimmer;
  this->RangeKeeper->Delete();
  this->GridSampler->Delete();

#ifndef _WIN32
  this->TearDownMap();
#endif
  this->TearDownFile();
}

//----------------------------------------------------------------------------
void vtkRawStridedReader2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


//----------------------------------------------------------------------------
void vtkRawStridedReader2::SwapDataByteOrder(int i)
{
  if (i == 1)
    {
    this->Skimmer->swap_endian();
    }
}

//------------------------------------------------------------------------------
int vtkRawStridedReader2::CanReadFile(const char* rawfile)
{
  int ret = 0;
  char *filename = new char[strlen(rawfile) + 10];
  sprintf(filename, "%s-1", rawfile);
  FILE *fp = fopen(filename, "r");
  if (fp)
    {
    ret = 1;
    fclose(fp);
    }
  delete filename;
  return ret;
}

//----------------------------------------------------------------------------
//RequestInformation supplies global meta information
// Global Extents  (integer count range of point count in x,y,z)
// Global Origin 
// Global Spacing (should be the stride value * original)

int vtkRawStridedReader2::RequestInformation(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{ 
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkDataObject::ORIGIN(),this->Origin,3);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent ,6);
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
int vtkRawStridedReader2::RequestUpdateExtent(
  vtkInformation* request,
  vtkInformationVector** inputVector,
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
int vtkRawStridedReader2::RequestData(
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

  DEBUGPRINT_RESOLUTION(
  {
  double res = 1.0;
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION()))
    {
    res = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_RESOLUTION());
    }
  int P = outInfo->Get
    (vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int NP = outInfo->Get
    (vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  cerr << P << "/" << NP << "@" << res 
       << "->" << this->SI << " " << this->SJ << " " << this->SK << endl;
  }
  );

  //prepping to produce real data and thus allocate real amounts of space
  int *uext = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  outData->SetExtent(uext);

  //Todo We need to be able to have user definable data type
  //Todo also multiple arrays/multiple scalarComponents
  outData->AllocateScalars();
  outData->GetPointData()->GetScalars()->SetName("PointCenteredData");
  float *myfloats = (float*)outData->GetScalarPointer();

//  double c_alloc = clock();

  // file stuff
  this->SetupFile();

  if(!this->fp) {
    cerr << "Could not open file: " << this->Filename << endl;

    return 0;
  }

  /*
  ifstream file(fd); //this->Filename, ios::in|ios::binary);
  if(file.is_open())
  {
    cerr << "Could not open file: " << this->Filename << endl;
    return 0;
  }
  if (file.bad())
    {
    cerr << "OPEN FAIL" << endl;
    return 0;
    }
  */

  this->Skimmer->set_uExtents((unsigned int*)uext);
  this->Skimmer->set_dims((unsigned int*)this->Dimensions);
  this->Skimmer->set_buffer_size((unsigned int)this->BlockReadSize);
  this->Skimmer->set_buffer_pointer(myfloats);
  this->Skimmer->set_sWholeExtent(sWholeExtent);
  
  //cerr << ">-INTERNAL--------------------------" << endl;
  unsigned int stride[3];
  stride[0] = this->SI;
  stride[1] = this->SJ;
  stride[2] = this->SK;

//  double c_prep = clock();

  if (!this->Skimmer->read(this->fp, stride, this))
    {
    cerr << "READ FAIL 3" << endl;
    return 0;
    }

  DEBUGPRINT_STRIDED_READER(
  unsigned int memsize = this->Skimmer->get_data_size(); 
  cerr << "memsize " << memsize << endl;
                           );

//  double c_fileio = clock();

  double range[2];
  outData->GetPointData()->GetScalars()->GetRange(range);
  int P = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int NP = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  DEBUGPRINT_METAINFORMATION(
  cerr << "RSR(" << this << ") Calculate range " << range[0] << ".." << range[1] << " for " << P << "/" << NP << endl;
                             );
  this->RangeKeeper->Insert(P, NP, uext, range, this->Resolution);

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
int vtkRawStridedReader2::ProcessRequest(vtkInformation *request,
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
/*
    cerr << P << "/" << NP << "\t";
    {for (int i = 0; i < 3; i++) cerr << spacing[i] << " ";}
    cerr << "\t";
    {for (int i = 0; i < 6; i++) cerr << ext[i] << " ";}
    cerr << "\t";
    {for (int i = 0; i < 6; i++) cerr << bounds[i] << " ";}
    cerr << endl;
*/
    double range[2];
    if (this->RangeKeeper->Search(P, NP, ext, range))
      {
      DEBUGPRINT_METAINFORMATION(
      cerr << "Range for " 
           << P << "/" << NP << " "
           << ext[0] << "," << ext[1] << ","
           << ext[2] << "," << ext[3] << ","
           << ext[4] << "," << ext[5] << " is " 
           << range[0] << " .. " << range[1] << endl;
                                 );
      vtkInformation *fInfo = 
        vtkDataObject::GetActiveFieldInformation
        (outInfo, vtkDataObject::FIELD_ASSOCIATION_POINTS, 
         vtkDataSetAttributes::SCALARS);
      if (fInfo)
        {
        fInfo->Set(vtkDataObject::PIECE_FIELD_RANGE(), range, 2);
        }
      }
    else
      {
      DEBUGPRINT_METAINFORMATION(
      cerr << "No range for " 
           << ext[0] << "," << ext[1] << ","
           << ext[2] << "," << ext[3] << ","
           << ext[4] << "," << ext[5] << " yet" << endl;        
                                 );
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
    double res = 1.0;
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
