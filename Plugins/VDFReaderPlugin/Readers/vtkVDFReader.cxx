/*=========================================================================

  Visualization and Analysis Platform for Ocean, Atmosphere, and Solar
  Researchers (VAPOR)
  Terms of Use

  This VAPOR (the Software ) was developed by the University
  Corporation for Atmospheric Research.

  PLEASE READ THIS SOFTWARE LICENSE AGREEMENT ("AGREEMENT") CAREFULLY.
  INDICATE YOUR ACCEPTANCE OF THESE TERMS BY SELECTING THE  I ACCEPT
  BUTTON AT THE END OF THIS AGREEMENT. IF YOU DO NOT AGREE TO ALL OF THE
  TERMS OF THIS AGREEMENT, SELECT THE  I DON?T ACCEPT? BUTTON AND THE
  INSTALLATION PROCESS WILL NOT CONTINUE.

  1.      License.  The University Corporation for Atmospheric Research
  (UCAR) grants you a non-exclusive right to use, create derivative
  works, publish, distribute, disseminate, transfer, modify, revise and
  copy the Software.

  2.      Proprietary Rights.  Title, ownership rights, and intellectual
  property rights in the Software shall remain in UCAR.

  3.      Disclaimer of Warranty on Software. You expressly acknowledge
  and agree that use of the Software is at your sole risk. The Software
  is provided "AS IS" and without warranty of any kind and UCAR
  EXPRESSLY DISCLAIMS ALL WARRANTIES AND/OR CONDITIONS OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, ANY WARRANTIES OR
  CONDITIONS OF TITLE, NON-INFRINGEMENT OF A THIRD PARTY?S INTELLECTUAL
  PROPERTY, MERCHANTABILITY OR SATISFACTORY QUALITY AND FITNESS FOR A
  PARTICULAR PURPOSE. UCAR DOES NOT WARRANT THAT THE FUNCTIONS CONTAINED
  IN THE SOFTWARE WILL MEET YOUR REQUIREMENTS, OR THAT THE OPERATION OF
  THE SOFTWARE WILL BE UNINTERRUPTED OR ERROR-FREE, OR THAT DEFECTS IN
  THE SOFTWARE WILL BE CORRECTED. FURTHERMORE, UCAR DOES NOT WARRANT OR
  MAKE ANY REPRESENTATIONS AND YOU ASSUME ALL RISK REGARDING THE USE OR
  THE RESULTS OF THE USE OF THE SOFTWARE OR RELATED DOCUMENTATION IN
  TERMS OF THEIR CORRECTNESS, ACCURACY, RELIABILITY, OR OTHERWISE. THE
  PARTIES EXPRESSLY DISCLAIM THAT THE UNIFORM COMPUTER INFORMATION
  TRANSACTIONS ACT (UCITA) APPLIES TO OR GOVERNS THIS AGREEMENT.  No
  oral or written information or advice given by UCAR or a UCAR
  authorized representative shall create a warranty or in any way
  increase the scope of this warranty. Should the Software prove
  defective, you (and not UCAR or any UCAR representative) assume the
  cost of all necessary correction.

  4.      Limitation of Liability.  UNDER NO CIRCUMSTANCES, INCLUDING
  NEGLIGENCE, SHALL UCAR OR ITS COLLABORATORS, INCLUDING OHIO STATE
  UNIVERSITY, BE LIABLE FOR ANY DIRECT, INCIDENTAL, SPECIAL, INDIRECT OR
  CONSEQUENTIAL DAMAGES INCLUDING LOST REVENUE, PROFIT OR DATA, WHETHER
  IN AN ACTION IN CONTRACT OR TORT ARISING OUT OF OR RELATING TO THE USE
  OF OR INABILITY TO USE THE SOFTWARE, EVEN IF UCAR HAS BEEN ADVISED OF
  THE POSSIBILITY OF SUCH DAMAGES.

  5.      Compliance with Law. All Software and any technical data
  delivered under this Agreement are subject to U.S. export control laws
  and may be subject to export or import regulations in other countries.
  You agree to comply strictly with all applicable laws and regulations
  in connection with use and distribution of the Software, including
  export control laws, and you acknowledge that you have responsibility
  to obtain any required license to export, re-export, or import as may
  be required.

  6.      No Support/Modifications. The names UCAR/NCAR and SCD may not
  be used in any advertising or publicity to endorse or promote any
  products or commercial entity unless specific written permission is
  obtained from UCAR.  The Software is provided without any support or
  maintenance, and without any obligation to provide you with
  modifications, improvements, enhancements, or updates of the Software.

  7.      Controlling Law and Severability.  This Agreement shall be
  governed by the laws of the United States and the State of Colorado.
  If for any reason a court of competent jurisdiction finds any
  provision, or portion thereof, to be unenforceable, the remainder of
  this Agreement shall continue in full force and effect. This Agreement
  shall not be governed by the United Nations Convention on Contracts
  for the International Sale of Goods, the application of which is
  hereby expressly excluded.

  8.      Termination.  Your rights under this Agreement will terminate
  automatically without notice from UCAR if you fail to comply with any
  term(s) of this Agreement.   You may terminate this Agreement at any
  time by destroying the Software and any related documentation and any
  complete or partial copies thereof.  Upon termination, all rights
  granted under this Agreement shall terminate.  The following
  provisions shall survive termination: Sections 2, 3, 4, 7 and 10.

  9.      Complete Agreement. This Agreement constitutes the entire
  agreement between the parties with respect to the use of the Software
  and supersedes all prior or contemporaneous understandings regarding
  such subject matter. No amendment to or modification of this Agreement
  will be binding unless in a writing and signed by UCAR.

  10.  Notices and Additional Terms.  Each copy of the Software shall
  include a copy of this Agreement and the following notice:

  "The source of this material is the Science Computing Division of the
  National Center for Atmospheric Research, a program of the University
  Corporation for Atmospheric Research (UCAR) pursuant to a Cooperative
  Agreement with the National Science Foundation; Copyright (c)2006 University
  Corporation for Atmospheric Research. All Rights Reserved."

  This notice shall be displayed on any documents, media, printouts, and
  visualizations or on any other electronic or tangible expressions
  associated with, related to or derived from the Software or associated
  documentation.

  The Software includes certain copyrighted, segregable components
  listed below (the Third Party Code?), including software developed by
  Ohio State University.  For this reason, you must check the source
  identified below for additional notice requirements and terms of use
  that apply to your use of this Software.

--------------------------------------------------------------------------

  Program:   Visualization Toolkit
  Module:    vtkVDFReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
          scalars->SetValue(iReal, vapor_data[(z - front_pad[2]) * data_size[1] * data_size[0] +
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
