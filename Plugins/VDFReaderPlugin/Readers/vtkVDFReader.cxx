// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) 2006 University Corporation for Atmospheric Research
// SPDX-License-Identifier: BSD-3-Clause AND LicenseRef-EULA-VAPOR
#include "vtkVDFReader.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkImageAlgorithm.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVInformationKeys.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

using namespace VAPoR;

vtkStandardNewMacro(vtkVDFReader);

//-----------------------------------------------------------------------------
vtkVDFReader::vtkVDFReader()
{

  this->FileName = 0;
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->Refinement = 0;
  this->nTimeSteps = 0;
  this->TimeSteps = nullptr;
  this->TimeStep = 0;
  this->vdc_md = nullptr;
  this->data_mgr = nullptr;
  this->vdfiobase = nullptr;

  this->CacheSize = 1000;
  this->height_factor = 4;
  this->RefinementRange[0] = 0;
  this->RefinementRange[1] = 5;
}

//-----------------------------------------------------------------------------
vtkVDFReader::~vtkVDFReader()
{
  this->SetFileName(0);
  delete data_mgr;
  delete vdfiobase;
  delete vdc_md;
  delete TimeSteps;
}

//-----------------------------------------------------------------------------
int vtkVDFReader::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // extract data objects
  vtkImageData* image = vtkImageData::GetData(outputVector);
  vtkInformation* info = outputVector->GetInformationObject(0);

  // setup initial boundaries
  int ext[6] = { 0, (int)ext_p[0], 0, (int)ext_p[1], 0, (int)ext_p[2] };
  int ext_minus_one[6] = { 0, (int)ext_p[0] - 1, 0, (int)ext_p[1] - 1, 0, (int)ext_p[2] - 1 };
  int *updateExt, front_pad[] = { 0, 0, 0 };

  size_t bdim[3], udim[3];
  GetVarDims(udim, bdim);
  const size_t* bmsize = vdc_md->GetBlockSize();

  size_t data_size[3] = { bmsize[0] * bdim[0], bmsize[1] * bdim[1], bmsize[2] * bdim[2] };
  size_t v_min[3] = { 0, 0, 0 };
  size_t v_max[3] = { bdim[0] - 1, bdim[1] - 1, bdim[2] - 1 };

  // update boundaries to only load the needed data from disk
  updateExt = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  if (true /*! extentsMatch(updateExt, ext)*/)
  {
    // printf("Received sub-block of extents: %d %d %d %d %d %d\n",
    //	updateExt[0], updateExt[1], updateExt[2],
    //	updateExt[3], updateExt[4], updateExt[5]);

    size_t v0[] = { updateExt[0], updateExt[2], updateExt[4] };
    size_t v1[] = { updateExt[1], updateExt[3], updateExt[5] };
    vdfiobase->MapVoxToBlk(v0, v_min);
    vdfiobase->MapVoxToBlk(v1, v_max);

    for (int i = 0; i < 6; i += 2)
    {
      ext_minus_one[i] = updateExt[i];
      ext_minus_one[i + 1] = updateExt[i + 1];
      ext[i] = updateExt[i];
      ext[i + 1] = updateExt[i + 1] + 1;
      data_size[i / 2] = (1 + v_max[i / 2] - v_min[i / 2]) * bmsize[i / 2];
      front_pad[i / 2] = v_min[i / 2] * bmsize[i / 2];
    }
  }

  // setup image memory
  image->SetExtent(ext_minus_one);
  image->SetScalarTypeToFloat();
  image->AllocateScalars();

  // get the correct timestep
  if (info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    double TimeStepsReq = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    TimeStep = (int)floor(TimeStepsReq);
    SetExtents(info);
    // printf("Received updated timestep: %d\n", TimeStep);
  }

  // load data for all enabled point arrays
  for (std::map<std::string, int>::iterator it = data.begin(); it != data.end(); ++it)
  {
    if (it->second == 0)
    {
      continue;
    }

    vtkFloatArray* scalars = vtkFloatArray::New();
    scalars->SetName(it->first.c_str());
    scalars->SetNumberOfValues((ext[1] - ext[0]) * (ext[3] - ext[2]) * (ext[5] - ext[4]));

    /*printf("Fetching %s data blocks [%d %d %d]-[%d %d %d], ext(vox): "
      "%d %d %d %d %d %d, data_size: %d %d %d, front_pad: %d %d %d,"
      " ts: %d\n", it->first.c_str(), v_min[0],v_min[1],v_min[2],
      v_max[0], v_max[1],v_max[2],ext[0],ext[1],ext[2],ext[3],ext[4],
      ext[5], data_size[0], data_size[1], data_size[2], front_pad[0],
      front_pad[1], front_pad[2], TimeStep);*/

    SetProgressText("Loading VDC Data");
    // check that data exists on disk for this var/ref/time
    if (!vdfiobase->VariableExists(TimeStep, it->first.c_str(), this->Refinement))
    {
      vtkErrorMacro(<< it->first << " data does not exist at time: " << TimeStep);
      return 0;
    }
    // load data into memory
    float* vapor_data =
      data_mgr->GetRegion(TimeStep, it->first.c_str(), this->Refinement, v_min, v_max);
    // verify data pointer
    if (vapor_data == nullptr)
    {
      vtkErrorMacro(<< "Data manager returned NULL pointer.");
      return 0;
    }

    // copy data to return memory (stripping block-padding)
    vtkIdType iReal = 0;
    for (vtkIdType z = ext[4]; z < ext[5]; z++)
    {
      for (vtkIdType y = ext[2]; y < ext[3]; y++)
      {
        for (vtkIdType x = ext[0]; x < ext[1]; x++)
        {
          scalars->SetValue(iReal,
            vapor_data[(z - front_pad[2]) * data_size[1] * data_size[0] +
              (y - front_pad[1]) * data_size[0] + (x - front_pad[0])]);
          iReal++;
        }
      }
      this->UpdateProgress((float)z / (1 + ext[5]));
    }

    image->GetPointData()->AddArray(scalars);
    if (!image->GetPointData()->GetScalars())
    {
      image->GetPointData()->SetScalars(scalars);
    }
    scalars->Delete();
  }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkVDFReader::RequestInformation(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  // vtkImageData *output = vtkImageData::GetData(outInfo);

  // only perform the initial allocations once
  if (vdc_md == nullptr)
  {
    // spap all '\' with '/'
    char* filename_ptr = FileName;
    while (*filename_ptr)
    {
      if (*filename_ptr == '\\')
      {
        *filename_ptr = '/';
      }
      filename_ptr++;
    }

    vdc_md = new Metadata(FileName);
    vdfiobase = new WaveletBlock3DRegionReader(vdc_md);
    data_mgr = new DataMgr(vdc_md, this->CacheSize);

    is_layered = vdc_md->GetGridType().compare("layered") == 0;
    RefinementRange[1] = vdc_md->GetNumTransforms();

    const std::vector<std::string> NamesAll = vdc_md->GetVariableNames();
    for (size_t i = 0; i < NamesAll.size(); i++)
    {
      data[NamesAll[i]] = 0;
    }

    // Find valid timesteps, communicate through pipeline
    FillTimeSteps();
    double TimeRange[2] = { TimeSteps[0], TimeSteps[nTimeSteps - 1] };
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), TimeSteps, nTimeSteps);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), TimeRange, 2);
  }

  SetExtents(outInfo);

  return 1;
}

//-----------------------------------------------------------------------------
void vtkVDFReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(NULL)") << endl;

  os << indent << "VariableType " << this->VariableType << endl;
  os << indent << "Refinement " << this->Refinement << endl;
  os << indent << "CacheSize " << this->CacheSize << endl;
}

//-----------------------------------------------------------------------------
int vtkVDFReader::CanReadFile(const char* fname)
{
  // TODO: Do some quick check on whether the file actually is in VDF format
  return 1;
}

//-----------------------------------------------------------------------------
void vtkVDFReader::SetRefinement(int newref)
{
  if (newref == this->Refinement)
  {
    return;
  }

  // printf("refinement level changed to: %d\n", newref);
  this->Refinement = newref;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkVDFReader::SetCacheSize(int newSize)
{
  if (newSize == this->CacheSize)
  {
    return;
  }

  // printf("cache size changed to: %d\n", newSize);
  this->CacheSize = newSize;

  if (data_mgr != nullptr)
  {
    delete data_mgr;
    data_mgr = new DataMgr(vdc_md, this->CacheSize);
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkVDFReader::SetVariableType(int newtype)
{
  if (newtype == this->VariableType)
  {
    return;
  }

  // printf("var type changed to: %d\n", newtype);
  this->VariableType = newtype;
  data.clear();
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkVDFReader::FillTimeSteps()
{
  // get list of all possible variables in metadata
  const std::vector<std::string> nms = vdc_md->GetVariableNames();

  // get all possible timesteps
  int tmp_nTimeSteps = vdc_md->GetNumTimeSteps();
  TimeSteps = new double[tmp_nTimeSteps];

  // number of valid timesteps, start a 0
  nTimeSteps = 0;
  for (int i = 0; i < tmp_nTimeSteps; i++)
  {
    bool good = false;
    for (size_t j = 0; j < nms.size() && !good; j++)
    {
      // find at least one variable existing to validate timestep
      if (vdfiobase->VariableExists(i, nms[j].c_str()) != 0)
      {
        TimeSteps[nTimeSteps++] = (double)i;
        good = true; // exit inner loop on first valid var
      }
    }
  }

  // set current timestep to first valid time
  TimeStep = (int)TimeSteps[0];

  return nTimeSteps;
}

//-----------------------------------------------------------------------------
bool vtkVDFReader::extentsMatch(int* a, int* b)
{
  for (int i = 0; i < 6; i++)
  {
    if (a[i] != b[i])
      return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
int vtkVDFReader::GetNumberOfPointArrays()
{
  return current_var_list.size();
}

//-----------------------------------------------------------------------------
const char* vtkVDFReader::GetPointArrayName(int index)
{
  return current_var_list[index].c_str();
}

//-----------------------------------------------------------------------------
int vtkVDFReader::GetPointArrayStatus(const char* name)
{
  return data[std::string(name)];
}

//-----------------------------------------------------------------------------
void vtkVDFReader::SetPointArrayStatus(const char* name, int en)
{
  if (en != data[std::string(name)])
  {
    data[std::string(name)] = en;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkVDFReader::GetVarDims(size_t* udims, size_t* bdims)
{
  // GetDim is the same for both types?
  vdfiobase->GetDim(udims, this->Refinement);

  // cast datamgr to correct subtype for block dimensions
  /*if (is_layered)
    {
    ((VAPoR::DataMgrLayered *)data_mgr)->GetDimBlk(bdims, this->Refinement);
    }
    else
    {
    ((VAPoR::DataMgrWB *)data_mgr)->GetDimBlk(bdims, this->Refinement);
    }*/
  vdfiobase->GetDimBlk(bdims, this->Refinement);
}

//-----------------------------------------------------------------------------
void vtkVDFReader::SetExtents(vtkInformation* outInfo)
{

  const std::vector<double> glExt = vdc_md->GetExtents(), tsExt = vdc_md->GetTSExtents(TimeStep);
  uExt = (std::vector<double>)(tsExt.empty() ? glExt : tsExt);

  vdfiobase->GetDim(ext_p, this->Refinement);
  int ext[6] = { 0, (int)ext_p[0] - 1, 0, (int)ext_p[1] - 1, 0, (int)ext_p[2] - 1 };

  std::vector<std::string> var_list;
  switch (this->VariableType)
  {
    case 0:
      var_list = (std::vector<std::string>)vdc_md->GetVariables3D();
      break;
    case 1:
      var_list = (std::vector<std::string>)vdc_md->GetVariables2DXY();
      ext[5] = 0;
      break;
    case 2:
      var_list = (std::vector<std::string>)vdc_md->GetVariables2DXZ();
      ext[3] = 0;
      break;
    case 3:
      var_list = (std::vector<std::string>)vdc_md->GetVariables2DYZ();
      ext[1] = 0;
      break;
    default:
      vtkErrorMacro(<< " FATAL: bad type: " << this->VariableType);
      break;
  }

  double origin[3] = { uExt[0], uExt[1], uExt[2] };
  // double bbox[6] = {uExt[0], uExt[3], uExt[1], uExt[4], uExt[2], uExt[5]};
  double spacing[3] = { (uExt[3] - uExt[0]) / (ext[1] + 1), (uExt[4] - uExt[1]) / (ext[3] + 1),
    (uExt[5] - uExt[2]) / (ext[5] + 1) };

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);
  // outInfo->Set(vtkPVInformationKeys::WHOLE_BOUNDING_BOX(),
  // bbox, 6);
  outInfo->Set(vtkDataObject::ORIGIN(), origin, 3);
  outInfo->Set(vtkDataObject::SPACING(), spacing, 3);

  // copy list of vars, excluding those for which no data exists:
  current_var_list.clear();
  for (size_t i = 0; i < var_list.size(); ++i)
  {
    if (vdfiobase->VariableExists(TimeStep, var_list[i].c_str(), this->Refinement) != 0)
    {
      current_var_list.push_back(var_list[i]);
    }
  }
}
