/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNIfTIWriter.cxx

  Copyright (c) Joseph Hennessey
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkNIfTIWriter.h"

#include "ThirdParty/vtknifti1_io.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include "vtkFieldData.h"
#include "vtkUnsignedCharArray.h"

#define ANALYZE_HEADER_ARRAY "vtkAnalyzeReaderHeaderArray"

vtkStandardNewMacro(vtkNIfTIWriter);

vtkNIfTIWriter::vtkNIfTIWriter()
{
  q = new double*[4];
  s = new double*[4];
  int count;
  for (count = 0; count < 4; count++)
  {
    q[count] = new double[4];
    s[count] = new double[4];
  }
  this->FileLowerLeft = 1;
  this->FileType = 0;
  this->FileDimensionality = 3;
  this->iname_offset = 352;
}

vtkNIfTIWriter::~vtkNIfTIWriter()
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
}

void vtkNIfTIWriter::SetFileType(int inValue)
{
  FileType = inValue;
}

int vtkNIfTIWriter::getFileType()
{
  return FileType;
}

/* return number of extensions written, or -1 on error */
static int nifti_write_extensions(znzFile fp, nifti_image* nim)
{
  nifti1_extension* list;
  char extdr[4] = { 0, 0, 0, 0 };
  int c, ok = 1;
  size_t size;

  if (znz_isnull(fp) || !nim || nim->num_ext < 0)
  {
    return -1;
  }

  // if invalid extension list, clear num_ext
  if (!vtknifti1_io::valid_nifti_extensions(nim))
    nim->num_ext = 0;

  // write out extender block
  if (nim->num_ext > 0)
    extdr[0] = 1;
  if (vtknifti1_io::nifti_write_buffer(fp, extdr, 4) != 4)
  {
    fprintf(stderr, "** failed to write extender\n");
    return -1;
  }

  list = nim->ext_list;
  for (c = 0; c < nim->num_ext; c++)
  {
    size = vtknifti1_io::nifti_write_buffer(fp, &list->esize, sizeof(int));
    ok = (size == (int)sizeof(int));
    if (ok)
    {
      size = vtknifti1_io::nifti_write_buffer(fp, &list->ecode, sizeof(int));
      ok = (size == (int)sizeof(int));
    }
    if (ok)
    {
      size = vtknifti1_io::nifti_write_buffer(fp, list->edata, list->esize - 8);
      ok = (((int)size) == list->esize - 8);
    }

    if (!ok)
    {
      fprintf(stderr, "** failed while writing extension #%d\n", c);
      return -1;
    }

    list++;
  }

  return nim->num_ext;
}

// GetExtension from uiig library.
static std::string GetExtension(const std::string& filename)
{

  // This assumes that the final '.' in a file name is the delimiter
  // for the file's extension type
  const std::string::size_type it = filename.find_last_of('.');

  // This determines the file's type by creating a new string
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

void vtkNIfTIWriter::WriteFileHeader(
  ostream* vtkNotUsed(file), vtkImageData* cache, int wholeExtent[6])
{

  struct nifti_1_header nhdr;
  znzFile fp = nullptr;
  size_t ss;
  int write_data, leave_open;
  znzFile imgfile = nullptr;
  const char* opts = "wb";
  int orientPosition = 252;

  write_data = 1;
  leave_open = 0;

  // Find the length of the rows to write.

  // Get the information from the input
  double spacing[3];
  double origin[3];
  int numComponents = cache->GetNumberOfScalarComponents();
  int imageDataType = cache->GetScalarType();
  cache->GetSpacing(spacing);
  cache->GetOrigin(origin);

  if (numComponents > 4)
  {
    vtkErrorMacro("cannot write data with more than 4 components yet.");
  }
  if (numComponents == 2)
  {
    vtkErrorMacro("cannot write data with 2 components yet.");
  }

  char* iname = this->GetFileName();
  std::string ImageFileName = GetImageFileName(iname);

  if (!vtknifti1_io::nifti_validfilename(ImageFileName.c_str()))
    vtkErrorMacro("bad fname input");

  vtkImageData* data = cache;
  vtkFieldData* fa = data->GetFieldData();
  int headerSize = 348;

  if (!fa)
  {
    fa = vtkFieldData::New();
    data->SetFieldData(fa);
    fa->Delete();
    fa = data->GetFieldData();
  }

  vtkDataArray* validNiftiDataArray = fa->GetArray(NIFTI_HEADER_ARRAY);
  vtkUnsignedCharArray* headerUnsignedCharArray = nullptr;
  foundNiftiHeader = true;
  foundAnalayzeHeader = false;
  if (!validNiftiDataArray)
  {
    headerUnsignedCharArray = vtkUnsignedCharArray::New();
    headerUnsignedCharArray->SetName(NIFTI_HEADER_ARRAY);
    headerUnsignedCharArray->SetNumberOfValues(headerSize);
    fa->AddArray(headerUnsignedCharArray);
    headerUnsignedCharArray->Delete();
    foundNiftiHeader = false;
    validNiftiDataArray = fa->GetArray(NIFTI_HEADER_ARRAY);
  }

  vtkDataArray* validAnalyzeDataArray = fa->GetArray(ANALYZE_HEADER_ARRAY);
  if (validAnalyzeDataArray)
  {
    foundAnalayzeHeader = true;
  }

  if (foundNiftiHeader)
  {
    headerUnsignedCharArray = vtkUnsignedCharArray::SafeDownCast(validNiftiDataArray);
  }
  else if (foundAnalayzeHeader)
  {
    headerUnsignedCharArray = vtkUnsignedCharArray::SafeDownCast(validAnalyzeDataArray);
  }

  nifti_1_header niftiHeader;
  unsigned char* headerUnsignedCharArrayPtr = (unsigned char*)&niftiHeader;
  int count;

  // this->headerUnsignedCharArray = new unsigned char[this->analyzeHeaderSize];

  nifti_image* m_NiftiImage = nullptr;

  if (foundNiftiHeader)
  {
    for (count = 0; count < headerSize; count++)
    {
      headerUnsignedCharArrayPtr[count] = headerUnsignedCharArray->GetValue(count);
    }
    m_NiftiImage = vtknifti1_io::nifti_convert_nhdr2nim(niftiHeader, ImageFileName.c_str());
  }
  else if (foundAnalayzeHeader)
  {
    for (count = 0; count < orientPosition; count++)
    {
      headerUnsignedCharArrayPtr[count] = headerUnsignedCharArray->GetValue(count);
    }
    for (count = orientPosition; count < headerSize; count++)
    {
      headerUnsignedCharArrayPtr[count] = 0;
    }
    m_NiftiImage = vtknifti1_io::nifti_convert_nhdr2nim(niftiHeader, ImageFileName.c_str());
  }
  else
  {
    for (count = 0; count < headerSize; count++)
    {
      headerUnsignedCharArrayPtr[count] = 0;
    }
    m_NiftiImage = vtknifti1_io::nifti_simple_init_nim();
  }

  m_NiftiImage->nifti_type = 1;

  int check = 0;
  int comp = 0;

  m_NiftiImage->fname =
    vtknifti1_io::nifti_makehdrname(ImageFileName.c_str(), m_NiftiImage->nifti_type, check, comp);
  m_NiftiImage->iname =
    vtknifti1_io::nifti_makeimgname(ImageFileName.c_str(), m_NiftiImage->nifti_type, check, comp);

  vtknifti1_io::nifti_set_iname_offset(m_NiftiImage);

  m_NiftiImage->dt = 0;

  m_NiftiImage->ndim = 3;
  m_NiftiImage->dim[1] = wholeExtent[1] + 1;
  m_NiftiImage->dim[2] = wholeExtent[3] + 1;
  m_NiftiImage->dim[3] = wholeExtent[5] + 1;
  m_NiftiImage->dim[4] = 1;
  m_NiftiImage->dim[5] = 1;
  m_NiftiImage->dim[6] = 1;
  m_NiftiImage->dim[7] = 1;
  m_NiftiImage->nx = m_NiftiImage->dim[1];
  m_NiftiImage->ny = m_NiftiImage->dim[2];
  m_NiftiImage->nz = m_NiftiImage->dim[3];
  m_NiftiImage->nt = m_NiftiImage->dim[4];
  m_NiftiImage->nu = m_NiftiImage->dim[5];
  m_NiftiImage->nv = m_NiftiImage->dim[6];
  m_NiftiImage->nw = m_NiftiImage->dim[7];

  // nhdr.pixdim[0] = 0.0 ;
  m_NiftiImage->pixdim[1] = spacing[0];
  m_NiftiImage->pixdim[2] = spacing[1];
  m_NiftiImage->pixdim[3] = spacing[2];
  m_NiftiImage->pixdim[4] = 0;
  m_NiftiImage->pixdim[5] = 1;
  m_NiftiImage->pixdim[6] = 1;
  m_NiftiImage->pixdim[7] = 1;
  m_NiftiImage->dx = m_NiftiImage->pixdim[1];
  m_NiftiImage->dy = m_NiftiImage->pixdim[2];
  m_NiftiImage->dz = m_NiftiImage->pixdim[3];
  m_NiftiImage->dt = m_NiftiImage->pixdim[4];
  m_NiftiImage->du = m_NiftiImage->pixdim[5];
  m_NiftiImage->dv = m_NiftiImage->pixdim[6];
  m_NiftiImage->dw = m_NiftiImage->pixdim[7];

  int numberOfVoxels = m_NiftiImage->nx;

  if (m_NiftiImage->ny > 0)
  {
    numberOfVoxels *= m_NiftiImage->ny;
  }
  if (m_NiftiImage->nz > 0)
  {
    numberOfVoxels *= m_NiftiImage->nz;
  }
  if (m_NiftiImage->nt > 0)
  {
    numberOfVoxels *= m_NiftiImage->nt;
  }
  if (m_NiftiImage->nu > 0)
  {
    numberOfVoxels *= m_NiftiImage->nu;
  }
  if (m_NiftiImage->nv > 0)
  {
    numberOfVoxels *= m_NiftiImage->nv;
  }
  if (m_NiftiImage->nw > 0)
  {
    numberOfVoxels *= m_NiftiImage->nw;
  }

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

  if (foundAnalayzeHeader)
  {
    qform_code = 1;
    for (row = 0; row < 4; row++)
    {
      for (col = 0; col < 4; col++)
      {
        q[row][col] = 0;
      }
    }
    q[0][0] = -1.0;
    q[1][1] = -1.0;
    q[2][2] = -1.0;

    mat44 tempMat44 = vtknifti1_io::nifti_make_orthog_mat44(
      q[0][0], q[0][1], q[0][2], q[1][0], q[1][1], q[1][2], q[2][0], q[2][1], q[2][2]);

    tempMat44.m[0][3] = m_NiftiImage->dim[1];
    tempMat44.m[1][3] = m_NiftiImage->dim[2];
    tempMat44.m[2][3] = m_NiftiImage->dim[3];

    vtknifti1_io::nifti_mat44_to_quatern(tempMat44, &(m_NiftiImage->quatern_b),
      &(m_NiftiImage->quatern_c), &(m_NiftiImage->quatern_d), &(m_NiftiImage->qoffset_x),
      &(m_NiftiImage->qoffset_y), &(m_NiftiImage->qoffset_z), &(m_NiftiImage->dx),
      &(m_NiftiImage->dy), &(m_NiftiImage->dz), &(m_NiftiImage->qfac));

    m_NiftiImage->qform_code = qform_code;
  }

  dataTypeSize = 1.0;

  m_NiftiImage->nvox = numberOfVoxels;

  if (numComponents == 1)
  {
    switch (imageDataType)
    {
      case VTK_BIT: // DT_BINARY:
        m_NiftiImage->datatype = DT_BINARY;
        m_NiftiImage->nbyper = 0;
        dataTypeSize = 0.125;
        break;
      case VTK_UNSIGNED_CHAR: // DT_UNSIGNED_CHAR:
        m_NiftiImage->datatype = DT_UNSIGNED_CHAR;
        m_NiftiImage->nbyper = 1;
        dataTypeSize = m_NiftiImage->nbyper;
        break;
      case VTK_SIGNED_CHAR: // DT_INT8:
        m_NiftiImage->datatype = DT_INT8;
        m_NiftiImage->nbyper = 1;
        dataTypeSize = m_NiftiImage->nbyper;
        break;
      case VTK_SHORT: // DT_SIGNED_SHORT:
        m_NiftiImage->datatype = DT_SIGNED_SHORT;
        m_NiftiImage->nbyper = 2;
        dataTypeSize = m_NiftiImage->nbyper;
        break;
      case VTK_UNSIGNED_SHORT: // DT_UINT16:
        m_NiftiImage->datatype = DT_UINT16;
        m_NiftiImage->nbyper = 2;
        dataTypeSize = m_NiftiImage->nbyper;
        break;
      case VTK_INT: // DT_SIGNED_INT:
        m_NiftiImage->datatype = DT_SIGNED_INT;
        m_NiftiImage->nbyper = 4;
        dataTypeSize = m_NiftiImage->nbyper;
        break;
      case VTK_UNSIGNED_INT: // DT_UINT32:
        m_NiftiImage->datatype = DT_UINT32;
        m_NiftiImage->nbyper = 4;
        dataTypeSize = m_NiftiImage->nbyper;
        break;
      case VTK_FLOAT: // DT_FLOAT:
        m_NiftiImage->datatype = DT_FLOAT;
        m_NiftiImage->nbyper = 4;
        dataTypeSize = m_NiftiImage->nbyper;
        break;
      case VTK_DOUBLE: // DT_DOUBLE:
        m_NiftiImage->datatype = DT_DOUBLE;
        m_NiftiImage->nbyper = 8;
        dataTypeSize = m_NiftiImage->nbyper;
        break;
      case VTK_LONG: // DT_INT64:
        m_NiftiImage->datatype = DT_INT64;
        m_NiftiImage->nbyper = 8;
        dataTypeSize = m_NiftiImage->nbyper;
        break;
      case VTK_UNSIGNED_LONG: // DT_UINT64:
        m_NiftiImage->datatype = DT_UINT64;
        m_NiftiImage->nbyper = 8;
        dataTypeSize = m_NiftiImage->nbyper;
        break;
      default:
        vtkErrorMacro("cannot handle this vtkType yet.");
        break;
    }
  }
  else if ((numComponents == 3) && (imageDataType == VTK_UNSIGNED_CHAR))
  {
    m_NiftiImage->datatype = DT_RGB;
    m_NiftiImage->nbyper = 3;
    dataTypeSize = m_NiftiImage->nbyper;
  }
  else if ((numComponents == 4) && (imageDataType == VTK_UNSIGNED_CHAR))
  {
    m_NiftiImage->datatype = DT_RGBA32;
    m_NiftiImage->nbyper = 4;
    dataTypeSize = m_NiftiImage->nbyper;
  }
  else
  {
    vtkErrorMacro("cannot handle this vtkType yet for multiple component data.");
  }

  imageSizeInBytes = (int)(numberOfVoxels * dataTypeSize);

  nhdr.datatype = m_NiftiImage->datatype;
  nhdr.bitpix = 8 * m_NiftiImage->nbyper;

  if (m_NiftiImage->cal_max > m_NiftiImage->cal_min)
  {
    nhdr.cal_max = m_NiftiImage->cal_max;
    nhdr.cal_min = m_NiftiImage->cal_min;
  }

  if (m_NiftiImage->scl_slope != 0.0)
  {
    nhdr.scl_slope = m_NiftiImage->scl_slope;
    nhdr.scl_inter = m_NiftiImage->scl_inter;
  }

  if (m_NiftiImage->descrip[0] != '\0')
  {
    memcpy(nhdr.descrip, m_NiftiImage->descrip, 79);
    nhdr.descrip[79] = '\0';
  }
  if (m_NiftiImage->aux_file[0] != '\0')
  {
    memcpy(nhdr.aux_file, m_NiftiImage->aux_file, 23);
    nhdr.aux_file[23] = '\0';
  }

  nhdr = vtknifti1_io::nifti_convert_nim2nhdr(m_NiftiImage); // create the nifti1_header struct

  // if writing to 2 files, make sure iname is set and different from fname
  if (m_NiftiImage->nifti_type != NIFTI_FTYPE_NIFTI1_1)
  {
    if (m_NiftiImage->iname && strcmp(m_NiftiImage->iname, m_NiftiImage->fname) == 0)
    {
      free(m_NiftiImage->iname);
      m_NiftiImage->iname = nullptr;
    }
    if (m_NiftiImage->iname == nullptr)
    { // then make a new one
      m_NiftiImage->iname =
        vtknifti1_io::nifti_makeimgname(m_NiftiImage->fname, m_NiftiImage->nifti_type, 0, 0);
      if (m_NiftiImage->iname == nullptr)
        return;
    }
  }

  // if we have an imgfile and will write the header there, use it
  if (!znz_isnull(imgfile) && m_NiftiImage->nifti_type == NIFTI_FTYPE_NIFTI1_1)
  {
    fp = imgfile;
  }
  else
  {
    fp = vtkznzlib::znzopen(
      m_NiftiImage->fname, opts, vtknifti1_io::nifti_is_gzfile(m_NiftiImage->fname));
    if (znz_isnull(fp))
    {
      vtkErrorMacro("cannot open output file");
      return;
    }
  }

  // write the header and extensions

  ss = vtkznzlib::znzwrite(&nhdr, 1, sizeof(nhdr), fp); // write header
  if (ss < sizeof(nhdr))
  {
    vtkErrorMacro("bad header write to output file");
    vtkznzlib::znzclose(fp);
    return;
  }

  // partial file exists, and errors have been printed, so ignore return
  if (m_NiftiImage->nifti_type != NIFTI_FTYPE_ANALYZE)
    (void)nifti_write_extensions(fp, m_NiftiImage);

  // if the header is all we want, we are done
  if (!write_data && !leave_open)
  {
    vtkznzlib::znzclose(fp);
    return;
  }

  if (m_NiftiImage->nifti_type != NIFTI_FTYPE_NIFTI1_1)
  {                          // get a new file pointer
    vtkznzlib::znzclose(fp); // first, close header file
    if (!znz_isnull(imgfile))
    {
      fp = imgfile;
    }
    else
    {
      fp = vtkznzlib::znzopen(
        m_NiftiImage->iname, opts, vtknifti1_io::nifti_is_gzfile(m_NiftiImage->iname));
      if (znz_isnull(fp))
        vtkErrorMacro("cannot open image file");
    }
  }

  vtkznzlib::znzseek(fp, m_NiftiImage->iname_offset, SEEK_SET); // in any case, seek to offset
  iname_offset = m_NiftiImage->iname_offset;

  if (write_data)
  {
    // nifti_write_all_data(fp,m_NiftiImage,NBL);
  }
  if (!leave_open)
    vtkznzlib::znzclose(fp);

  return;
}

void vtkNIfTIWriter::WriteFile(
  ostream* vtkNotUsed(file), vtkImageData* data, int extent[6], int wholeExtent[])
{
  (void)wholeExtent; // Not used
  // struct nifti_1_header nhdr ;
  znzFile fp = nullptr;
  // size_t                ss ;
  size_t numberOfBytes;
  int write_data, leave_open;
  znzFile imgfile = nullptr;
  const char* opts = "ab";
  char* p = (char*)data->GetScalarPointer();

  // reorient data
  unsigned char* outUnsignedCharPtr = (unsigned char*)p;
  int scalarSize = (int)dataTypeSize;
  int inIndex[3];
  int inDim[3];
  int outDim[3];
  int flipAxis[3];
  int flipIndex[3];
  int InPlaceFilteredAxes[3];
  int count;
  int inExtent[6];
  int outExtent[6];
  int inStride[3];
  int outStride[3];
  long inOffset;
  long charInOffset;

  flipAxis[0] = 0;
  flipAxis[1] = 0;
  flipAxis[2] = 0;

  InPlaceFilteredAxes[0] = 0;
  InPlaceFilteredAxes[1] = 1;
  InPlaceFilteredAxes[2] = 2;

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

  for (count = 0; count < 3; count++)
  {
    inDim[count] = (extent[(count * 2) + 1] - extent[count * 2]) + 1;
    inExtent[count * 2] = extent[count * 2];
    inExtent[(count * 2) + 1] = extent[(count * 2) + 1];
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
  (void)outExtent;

  unsigned char* tempUnsignedCharData = nullptr;
  unsigned char* tempOutUnsignedCharData = nullptr;

  tempUnsignedCharData = new unsigned char[outDim[0] * outDim[1] * outDim[2] * scalarSize];
  tempOutUnsignedCharData = new unsigned char[outDim[0] * outDim[1] * outDim[2] * scalarSize];

  char* out_p = (char*)tempOutUnsignedCharData;

  int idSize;
  int idZ, idY, idX;
  long outSliceSize = outDim[0] * outDim[1] * scalarSize;
  long outRowSize = outDim[0] * scalarSize;
  long outRowOffset;
  long outSliceOffset;
  long outOffset;
  long charOutOffset;

  // first flip

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
          tempOutUnsignedCharData[charOutOffset] = tempUnsignedCharData[count++];
        }
      }
    }
  }

  // then permute

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
          tempUnsignedCharData[count++] = tempOutUnsignedCharData[charInOffset];
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
          tempOutUnsignedCharData[charOutOffset] = tempUnsignedCharData[count++];
        }
      }
    }
  }

  delete tempUnsignedCharData;
  tempUnsignedCharData = nullptr;

  write_data = 1;
  leave_open = 0;

  char* iname = this->GetFileName();
  std::string ImageFileName = GetImageFileName(iname);

  if (!znz_isnull(imgfile))
  {
    fp = imgfile;
  }
  else
  {
    fp = vtkznzlib::znzopen(
      ImageFileName.c_str(), opts, vtknifti1_io::nifti_is_gzfile(ImageFileName.c_str()));
    if (znz_isnull(fp))
      vtkErrorMacro("cannot open image file");
  }
  numberOfBytes = this->getImageSizeInBytes();

  vtkznzlib::znzrewind(fp);
  vtkznzlib::znzseek(fp, iname_offset, SEEK_SET); // in any case, seek to offset
  if (write_data)
  {
    vtknifti1_io::nifti_write_buffer(fp, out_p, numberOfBytes);
  }
  if (!leave_open)
    vtkznzlib::znzclose(fp);

  delete tempOutUnsignedCharData;
  tempOutUnsignedCharData = nullptr;
  out_p = nullptr;
}

//----------------------------------------------------------------------------
void vtkNIfTIWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
