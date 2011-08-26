/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWorldWarp.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWorldWarp.h"
#include "vtkObjectFactory.h"

#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInformationExecutivePortKey.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkWorldWarp);

//----------------------------------------------------------------------------
vtkWorldWarp::vtkWorldWarp()
{
  this->MapFileName = NULL;
  this->LonInput = 0;
  this->LatInput = 1;
  this->AltInput = 2;
  this->XScale = 1.0;
  this->XBias = 0.0;
  this->YScale = 1.0;
  this->YBias = 0.0;
  this->ZScale = 1.0;
  this->ZBias = 0.0;
  this->BaseAltitude = 6371000;
  this->AltitudeScale = 1.0;

  this->LonMap = NULL;
  this->LonMapSize = 0;
  this->LatMap = NULL;
  this->LatMapSize = 0;
  this->AltMap = NULL;
  this->AltMapSize = 0;

  this->GetInformation()->Set(vtkAlgorithm::MANAGES_METAINFORMATION(), 1);
}

//----------------------------------------------------------------------------
vtkWorldWarp::~vtkWorldWarp()
{
  this->SetMapFileName(NULL);
  delete[] this->LonMap;
  delete[] this->LatMap;
  delete[] this->AltMap;
}

//----------------------------------------------------------------------------
void vtkWorldWarp::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LonInput: " << this->LonInput << endl;
  os << indent << "LatInput: " << this->LatInput << endl;
  os << indent << "AltInput: " << this->AltInput << endl;
  os << indent << "XScale: " << this->XScale << endl;
  os << indent << "XBias: " << this->XBias << endl;
  os << indent << "YScale: " << this->YScale << endl;
  os << indent << "YBias: " << this->YBias << endl;
  os << indent << "ZScale: " << this->ZScale << endl;
  os << indent << "ZBias: " << this->ZBias << endl;
  os << indent << "BaseAltitude: " << this->BaseAltitude << endl;
  os << indent << "AltitudeScale: " << this->AltitudeScale << endl;
  os << indent << "MapFileName: "
     << (this->MapFileName ? this->MapFileName : "(none)") << endl;
}

//----------------------------------------------------------------------------
void vtkWorldWarp::SetMapFileName(const char *_arg)
{
  if ( this->MapFileName == NULL && _arg == NULL)
    {
    return;
    }
  if ( this->MapFileName && _arg && (!strcmp(this->MapFileName,_arg)))
    {
    return;
    }
  if (this->MapFileName)
    {
    delete [] this->MapFileName;
    }
  if (_arg)
    {
    size_t n = strlen(_arg) + 1;
    char *cp1 = new char[n];
    const char *cp2 = (_arg);
    this->MapFileName = cp1;
    do { *cp1++ = *cp2++; } while ( --n );
    }
   else
    {
    this->MapFileName = NULL;
    }

  delete[] this->LonMap;
  this->LonMap = NULL;
  this->LonMapSize = 0;
  delete[] this->LatMap;
  this->LatMap = NULL;
  this->LatMapSize = 0;
  delete[] this->AltMap;
  this->AltMap = NULL;
  this->AltMapSize = 0;

  if(this->MapFileName && strlen(this->MapFileName)>2)
    {
    //this parses text files as dumped by ncdump -c, with header manually
    //stripped. The contents look something like the following
    /*
    depth_t = 5.00622, 15.06873, 25.28343, 35.75849, 46.61269, 57.98099,
      70.02139, 82.92409, 96.92413, 112.3189, 129.4936, 148.9582, 171.4044,
      197.7919, 229.4842, 268.4617, 317.6501, 381.3864, 465.9132, 579.3073,
      729.3513, 918.3723, 1139.153, 1378.574, 1625.7, 1875.106, 2125.011, 2375,
      2624.999, 2874.999, 3124.999, 3374.999, 3624.999, 3874.999, 4124.999,
      4374.999, 4624.999, 4874.999, 5124.999, 5374.999, 5624.999, 5874.999 ;
    other_arrays_we_ignore =  ...
    */

    //TODO: allow different array names instead of these hardcoded ones
    ifstream file(this->MapFileName);
    std::string nextline;

    std::vector<double> values;
    int mode = ON_NONE;
    while(file)
      {
      std::getline(file, nextline);
      if (nextline.find("t_lon") != std::string::npos)
        {
        mode = ON_LON;
        }
      if (nextline.find("t_lat") != std::string::npos)
        {
        mode = ON_LAT;
        }
      if (nextline.find("depth_t") != std::string::npos)
        {
        mode = ON_ALT;
        }
      if (mode != ON_NONE)
        {
        size_t start = 0;
        size_t whereisequal = nextline.find("=");
        if (whereisequal != std::string::npos)
          {
          start = whereisequal+1;
          }
        while (start != std::string::npos &&
               (start < nextline.size()-2) )
          {
          size_t whereiscomma = nextline.find(",", start);
          size_t whereissemi = nextline.find(";", start);
          size_t end = (whereiscomma < whereissemi) ? whereiscomma : whereissemi;
          std::string nextnumber = nextline.substr(start,end-start);
          values.push_back(strtod(nextnumber.c_str(), NULL));
          start = end;
          if (start != std::string::npos)
            {
            start++;
            }
          }
        }
      if (nextline.find(";") != std::string::npos)
        {
        if (values.size() > 0)
          {
          double *dest = NULL;
          switch(mode)
            {
            case ON_LON:
              this->LonMap = new double[values.size()];
              this->LonMapSize = values.size() -1;
              dest = this->LonMap;
              break;
            case ON_LAT:
              this->LatMap = new double[values.size()];
              this->LatMapSize = values.size() -1;
              dest = this->LatMap;
              break;
            case ON_ALT:
              this->AltMap = new double[values.size()];
              this->AltMapSize = values.size() -1;
              dest = this->AltMap;
              break;
            }
          for (unsigned int i = 0; i < values.size(); i++)
            {
            *dest = values[i];
            dest++;
            }
          values.clear();
          }
        mode = ON_NONE;
        }
      }
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkWorldWarp::SwapPoint(double inPoint[3], double outPoint[3])
{
  //apply scaling and biasing
  inPoint[0] = (inPoint[0] * this->XScale) + this->XBias;
  inPoint[1] = (inPoint[1] * this->YScale) + this->YBias;
  inPoint[2] = (inPoint[2] * this->ZScale) + this->ZBias;

  //apply axis shifting
  double coord[3];
  coord[0] = inPoint[this->LonInput];
  coord[1] = inPoint[this->LatInput];
  coord[2] = inPoint[this->AltInput];

  //apply lon lat alt lookuptables, if present
  int idx;
  double lon, lat, alt;
  lon = coord[0];
  if (this->LonMap)
    {
    idx = (int)coord[0];
    if (idx < 0)
      {
      idx = 0;
      }
    if (idx > this->LonMapSize)
      {
      idx = this->LonMapSize;
      }
    lon = this->LonMap[idx];
    }
  lat = coord[1];
  if (this->LatMap)
    {
    idx = (int)coord[1];
    if (idx < 0)
      {
      idx = 0;
      }
    if (idx > this->LatMapSize)
      {
      idx = this->LatMapSize;
      }
    lat = this->LatMap[idx];
    }
  alt = coord[2];
  if (this->AltMap)
    {
    idx = (int)coord[2];
    if (idx < 0)
      {
      idx = 0;
      }
    if (idx > this->AltMapSize)
      {
      idx = this->AltMapSize;
      }
    alt = this->AltMap[idx];
    }

  //project from spherical to cartesian coordinates
  alt = this->BaseAltitude + alt * this->AltitudeScale;
  lon = lon*vtkMath::Pi()/180.0;
  lat = lat*vtkMath::Pi()/180.0;
  outPoint[0] = alt*cos(lon)*cos(lat);
  outPoint[1] = alt*sin(lon)*cos(lat);
  outPoint[2] = alt*sin(lat);
}

#define vtkWorldWarpFRACPOINT(res, a,b,frac)  \
  res[0] = a[0]+(b[0]-a[0])*frac; \
  res[1] = a[1]+(b[1]-a[1])*frac; \
  res[2] = a[2]+(b[2]-a[2])*frac; \

//----------------------------------------------------------------------------
int vtkWorldWarp::ProcessRequest(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  if(!request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_MANAGE_INFORMATION()))
    {
    return this->Superclass::ProcessRequest(request, inputVector, outputVector);
    }

  // copy attributes across unmodified
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  if (inInfo->Has(vtkDataObject::CELL_DATA_VECTOR()))
    {
    outInfo->CopyEntry(inInfo, vtkDataObject::CELL_DATA_VECTOR(), 1);
    }
  if (inInfo->Has(vtkDataObject::POINT_DATA_VECTOR()))
    {
    outInfo->CopyEntry(inInfo, vtkDataObject::POINT_DATA_VECTOR(), 1);
    }

  // update the piece bounding box based on the transformation
  vtkSmartPointer<vtkPoints> inPts = vtkSmartPointer<vtkPoints>::New();
  vtkIdType ptId, numPts;
  double inpt[3], outpt[3];

  double *pbounds = inInfo->Get(vtkStreamingDemandDrivenPipeline::PIECE_BOUNDING_BOX());
  //convert to 8 corner Points
  double pts[14][3];
  pts[0][0] = pbounds[0]; pts[0][1] = pbounds[2+0]; pts[0][2] = pbounds[4+0];
  pts[1][0] = pbounds[0]; pts[1][1] = pbounds[2+0]; pts[1][2] = pbounds[4+1];
  pts[2][0] = pbounds[0]; pts[2][1] = pbounds[2+1]; pts[2][2] = pbounds[4+0];
  pts[3][0] = pbounds[0]; pts[3][1] = pbounds[2+1]; pts[3][2] = pbounds[4+1];
  pts[4][0] = pbounds[1]; pts[4][1] = pbounds[2+0]; pts[4][2] = pbounds[4+0];
  pts[5][0] = pbounds[1]; pts[5][1] = pbounds[2+0]; pts[5][2] = pbounds[4+1];
  pts[6][0] = pbounds[1]; pts[6][1] = pbounds[2+1]; pts[6][2] = pbounds[4+0];
  pts[7][0] = pbounds[1]; pts[7][1] = pbounds[2+1]; pts[7][2] = pbounds[4+1];
  //mid point of each face
  vtkWorldWarpFRACPOINT(pts[8],  pts[0], pts[3], 0.5);
  vtkWorldWarpFRACPOINT(pts[9],  pts[0], pts[6], 0.5);
  vtkWorldWarpFRACPOINT(pts[10], pts[0], pts[5], 0.5);
  vtkWorldWarpFRACPOINT(pts[11], pts[4], pts[7], 0.5);
  vtkWorldWarpFRACPOINT(pts[12], pts[2], pts[7], 0.5);
  vtkWorldWarpFRACPOINT(pts[13], pts[1], pts[7], 0.5);
  numPts = 8;
  for (int i = 0; i < numPts; i++)
    {
    inPts->InsertNextPoint(pts[i][0], pts[i][1], pts[i][2]);
    }
  //todo: subdivide bounds further to get a more accurate estimate

  // Loop over points on bounding box, adjusting locations
  vtkBoundingBox bbox;
  for (ptId=0; ptId < numPts; ptId++)
    {
    inPts->GetPoint(ptId, inpt);
    this->SwapPoint(inpt, outpt);
    bbox.AddPoint(outpt);
    }

  // push the result on down the pipeline
  double obounds[6];
  bbox.GetBounds(obounds);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::PIECE_BOUNDING_BOX(), obounds, 6);

  //compute a normal for the piece so that pieces on back of sphere are rejected
  double midback[3];
  midback[0] = (pbounds[0]+pbounds[1])/2;
  midback[1] = (pbounds[2]+pbounds[3])/2;
  midback[2] = pbounds[5];

  double midfront[3];
  midfront[0] = (pbounds[0]+pbounds[1])/2;
  midfront[1] = (pbounds[2]+pbounds[3])/2;
  midfront[2] = pbounds[4];

  this->SwapPoint(midback, outpt);
  midback[0] = outpt[0];
  midback[1] = outpt[1];
  midback[2] = outpt[2];
  this->SwapPoint(midfront, outpt);
  midfront[0] = outpt[0];
  midfront[1] = outpt[1];
  midfront[2] = outpt[2];

  double pnorm[3];
  pnorm[0] = midfront[0] - midback[0];
  pnorm[1] = midfront[1] - midback[1];
  pnorm[2] = midfront[2] - midback[2];
  //cerr << pnorm[0] << "," << pnorm[1] << "," << pnorm[2] << endl;
  outInfo->Set(vtkStreamingDemandDrivenPipeline::PIECE_NORMAL(), pnorm, 3);

  return 1;
}

//----------------------------------------------------------------------------
int vtkWorldWarp::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPolyData* input = vtkPolyData::SafeDownCast
    (inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast
    (outInfo->Get(vtkDataObject::DATA_OBJECT()));

  //copy all the attributes across
  output->CopyStructure(input);
  output->GetFieldData()->ShallowCopy(input->GetFieldData());
  output->GetCellData()->ShallowCopy(input->GetCellData());
  output->GetPointData()->ShallowCopy(input->GetPointData());

  //iterate over geometry and warp every point
  //TODO: create lat,lon,alt arrays, they could be useful and
  //would be nice to see onscreen
  vtkPoints *opts = vtkPoints::New();
  vtkIdType npts = input->GetNumberOfPoints();
  opts->SetNumberOfPoints(npts);
  double nextipt[3];
  double nextopt[3];
  for(vtkIdType i = 0; i < npts; i++)
    {
    input->GetPoint(i, nextipt);
    this->SwapPoint(nextipt, nextopt);
    opts->SetPoint(i, nextopt);
    }
  output->SetPoints(opts);
  opts->Delete();
  return 1;
}
