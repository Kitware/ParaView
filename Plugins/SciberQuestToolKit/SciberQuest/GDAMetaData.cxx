/* ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "GDAMetaData.h"
#include "GDAMetaDataKeys.h"

#include <iostream>
#include <ios>
#include <sstream>

#include "vtkInformation.h"
#include "vtkExecutive.h"

#include "PrintUtils.h"
#include "FsUtils.h"
#include "SQMacros.h"
#include "Tuple.hxx"

//-----------------------------------------------------------------------------
GDAMetaData::GDAMetaData()
{
  this->HasDipoleCenter=false;
  this->DipoleCenter[0]=
  this->DipoleCenter[1]=
  this->DipoleCenter[2]=-555.5;
}

//-----------------------------------------------------------------------------
GDAMetaData &GDAMetaData::operator=(const GDAMetaData &other)
{
  if (&other==this)
    {
    return *this;
    }

  this->BOVMetaData::operator=(other);

  this->HasDipoleCenter=other.HasDipoleCenter;
  this->DipoleCenter[0]=other.DipoleCenter[0];
  this->DipoleCenter[1]=other.DipoleCenter[1];
  this->DipoleCenter[2]=other.DipoleCenter[2];

  return *this;
}

//-----------------------------------------------------------------------------
int GDAMetaData::OpenDataset(const char *fileName, char mode)
{
    if (mode=='r')
      {
      return this->OpenDatasetForRead(fileName);
      }
    else
    if ((mode=='w')||(mode=='a'))
      {
      return this->OpenDatasetForWrite(fileName,mode);
      }
    else
      {
      sqErrorMacro(std::cerr,"Invalid mode " << mode << ".");
      }

    return 0;
}

//-----------------------------------------------------------------------------
int GDAMetaData::OpenDatasetForWrite(const char *fileName, char mode)
{
  this->Mode=mode;
  this->IsOpen=1;
  this->FileName=fileName;
  this->SetPathToBricks(StripFileNameFromPath(fileName).c_str());
  return 1;
}

//-----------------------------------------------------------------------------
int GDAMetaData::OpenDatasetForRead(const char *fileName)
{
  this->IsOpen=0;

  // Open
  std::ifstream metaFile(fileName);
  if (!metaFile.is_open())
    {
    sqErrorMacro(std::cerr,"Could not open " << fileName << ".");
    return 0;
    }

  // We expect the bricks to be in the same directory as the metadata file.
  this->SetPathToBricks(StripFileNameFromPath(fileName).c_str());
  const char *path=this->GetPathToBricks();

  // Read
  metaFile.seekg(0,ios::end);
  size_t metaFileLen=metaFile.tellg();
  metaFile.seekg(0,ios::beg);
  char *metaDataBuffer=static_cast<char *>(malloc(metaFileLen+1));
  metaDataBuffer[metaFileLen]='\0';
  metaFile.read(metaDataBuffer,metaFileLen);
  std::string metaData(metaDataBuffer);
  free(metaDataBuffer); // TODO use string's memory directly

  // Parse
  //ToLower(metaData);

  // get the brick file extension
  std::string ext;
  if (ParseValue(metaData,0,"ext=",ext)==std::string::npos)
    {
    ext="gda";
    }
  this->SetBrickFileExtension(ext);

  // We are expecting are nx,ny, and nz in the header for all
  // mesh types.
  int nx,ny,nz;
  if ( ParseValue(metaData,0,"nx=",nx)==std::string::npos
    || ParseValue(metaData,0,"ny=",ny)==std::string::npos
    || ParseValue(metaData,0,"nz=",nz)==std::string::npos )
    {
    sqErrorMacro(std::cerr,
         << "Parsing " << fileName
         << " dimensions not found. Expected nx=N, ny=M, nz=P.");
    return 0;
    }
  CartesianExtent domain(0,nx-1,0,ny-1,0,nz-1);
  this->SetDomain(domain);

  if ( Present(path,"x", ext.c_str())
    && Present(path,"y", ext.c_str())
    && Present(path,"z", ext.c_str()))
    {
    // mark as stretched mesh
    this->SetDataSetType("vtkRectilinearGrid");

    // read the coordinate arrays here. Even for large grids
    // this represents a small ammount of data thus it's
    // probably better to read on proc 0 and distribute over
    // the network.
    size_t n[3]={size_t(nx+1),size_t(ny+1),size_t(nz+1)};
    char coordId[]="xyz";
    std::ostringstream coordFn;

    for (int q=0; q<3; ++q)
      {
      // read coordinate array
      coordFn.str("");
      coordFn << path << PATH_SEP << coordId[q] << "." << ext;

      SharedArray<float> *coord=this->GetCoordinate(q);
      coord->Resize(n[q]);

      float *pCoord=coord->GetPointer();

      if (LoadBin(coordFn.str().c_str(),n[q],pCoord)==0)
        {
        sqErrorMacro(std::cerr,
          << "Error: Failed to read coordinate "
          << q << " from \"" << coordFn.str() << "\".");
        return 0;
        }

      // shift on to the dual grid
      size_t nc=n[q]-1;

      for (size_t i=0; i<nc; ++i)
        {
        pCoord[i]=(pCoord[i]+pCoord[i+1])/2.0f;
        }

      coord->Resize(nc);
      }
    }
  else
    {
    // mark as uniform grid
    this->SetDataSetType("vtkImageData");

    double x0,y0,z0;
    if ( ParseValue(metaData,0,"x0=",x0)==std::string::npos
      || ParseValue(metaData,0,"y0=",y0)==std::string::npos
      || ParseValue(metaData,0,"z0=",z0)==std::string::npos )
      {
      // if no origin provided assume 0,0,0
      x0=y0=z0=0.0;
      }
    double X0[3]={x0,y0,z0};
    this->SetOrigin(X0);

    double dx,dy,dz;
    if ( ParseValue(metaData,0,"dx=",dx)==std::string::npos
      || ParseValue(metaData,0,"dy=",dy)==std::string::npos
      || ParseValue(metaData,0,"dz=",dz)==std::string::npos )
      {
      // if no spacing is provided assume 1,1,1
      dx=dy=dz=1.0;
      }
    double dX[3]={dx,dy,dz};
    this->SetSpacing(dX);
    }

  double dt;
  if (ParseValue(metaData,0,"dt=",dt)==std::string::npos)
    {
    // no time step size provided assume 1
    dt=1;
    }
  this->SetDt(dt);

  // TODO
  // the following meta data enhancments are disabled until
  // I add the virtual pack/unpack methods, so that the process
  // that touches disk can actually stream them to the other
  // processes

  // // Look for the dipole center
  // double di_i,di_j,di_k;
  // if ( ParseValue(metaData,0,"i_dipole=",di_i)==std::string::npos
  //   || ParseValue(metaData,0,"j_dipole=",di_j)==std::string::npos
  //   || ParseValue(metaData,0,"k_dipole=",di_k)==std::string::npos)
  //   {
  //   // std::cerr << __LINE__ << " Warning: Parsing " << fileName
  //   //           << " dipole center not found." << std::endl;
  //   this->HasDipoleCenter=false;
  //   }
  // else
  //   {
  //   this->HasDipoleCenter=true;
  //   this->DipoleCenter[0]=di_i;
  //   this->DipoleCenter[1]=di_j;
  //   this->DipoleCenter[2]=di_k;
  //   }

  // double r_mp;
  // double r_obs_to_mp;
  // if ( ParseValue(metaData,0,"R_MP=",r_mp)==std::string::npos
  //   || ParseValue(metaData,0,"R_obstacle_to_MP=",r_obs_to_mp)==std::string::npos)
  //   {
  //   std::cerr << __LINE__ << " Warning: Parsing " << fileName
  //             << " magnetopause dimension not found." << std::endl;
  //   this->CellSizeRe=-1.0;
  //   }
  // else
  //   {
  //   this->CellSizeRe=r_mp*r_obs_to_mp/100.0;
  //   }
  // double
  //
  //     i_dipole=100,
  //     j_dipole=128,
  //     k_dipole=128,
  //     R_MP=16.,
  //     R_obstacle_to_MP=0.57732,

  int nArrays=0;
  // look for explicitly named arrays
  size_t at;
  at=0;
  while (at!=std::string::npos)
    {
    std::string scalar;
    at=ParseValue(metaData,at,"scalar:",scalar);
    if (at!=std::string::npos)
      {
      if (ScalarRepresented(path,scalar.c_str()))
        {
        this->AddScalar(scalar.c_str());
        ++nArrays;
        }
      else
        {
        sqErrorMacro(std::cerr,"Named scalar " << scalar << " not present.");
        }
      }
    }

  at=0;
  while (at!=std::string::npos)
    {
    std::string vector;
    at=ParseValue(metaData,at,"vector:",vector);
    if (at!=std::string::npos)
      {
      if (VectorRepresented(path,vector.c_str()))
        {
        this->AddVector(vector.c_str());
        ++nArrays;
        }
      else
        {
        sqErrorMacro(std::cerr,"Named vector " << vector << " not present.");
        }
      }
    }

  at=0;
  while (at!=std::string::npos)
    {
    std::string stensor;
    at=ParseValue(metaData,at,"stensor:",stensor);
    if (at!=std::string::npos)
      {
      if (SymetricTensorRepresented(path,stensor.c_str()))
        {
        this->AddSymetricTensor(stensor.c_str());
        ++nArrays;
        }
      else
        {
        sqErrorMacro(std::cerr,"Named stensor " << stensor << " not present.");
        }
      }
    }

  at=0;
  while (at!=std::string::npos)
    {
    std::string tensor;
    at=ParseValue(metaData,at,"tensor:",tensor);
    if (at!=std::string::npos)
      {
      if (SymetricTensorRepresented(path,tensor.c_str()))
        {
        this->AddTensor(tensor.c_str());
        ++nArrays;
        }
      else
        {
        sqErrorMacro(std::cerr,"Named tensor " << tensor << " not present.");
        }
      }
    }

  // We had to find at least one brick, otherwise we have problems.
  // As long as there is at least one brick, generate the series ids.
  if (nArrays)
    {
    const char *arrayName=this->GetArrayName(0);
    std::string prefix(arrayName);
    if (this->IsArrayVector(arrayName))
      {
      prefix+="x_";
      }
    else
    if (this->IsArrayTensor(arrayName)
      || this->IsArraySymetricTensor(arrayName))
      {
      prefix+="-xx_";
      }
    else
      {
      prefix+="_";
      }
    GetSeriesIds(path,prefix.c_str(),this->TimeSteps);
    if (!this->TimeSteps.size())
      {
      sqErrorMacro(std::cerr,
        << " Error: Time series was not found in " << path << "."
        << " Expected files named according to the following convention \"array_time.ext\".");
      return 0;
      }
    }
  else
    {
    sqErrorMacro(std::cerr,
      << " Error: No bricks found in " << path << "."
      << " Expected bricks in the same directory as the metdata file.");
    return 0;
    }

  this->Mode='r';
  this->IsOpen=1;
  this->FileName=fileName;

  return 1;
}

//-----------------------------------------------------------------------------
int GDAMetaData::CloseDataset()
{
  BOVMetaData::CloseDataset();

  this->HasDipoleCenter=false;
  this->DipoleCenter[0]=
  this->DipoleCenter[1]=
  this->DipoleCenter[2]=0.0;

  return 1;
}

//-----------------------------------------------------------------------------
int GDAMetaData::Write()
{
  if (!this->IsOpen)
    {
    sqErrorMacro(std::cerr, "Can't write because file is not open.");
    return 0;
    }

  if ((this->Mode!='w')&&(this->Mode!='a'))
    {
    sqErrorMacro(std::cerr, "Writing to a read only file.");
    return 0;
    }

  bool writeHeader=true;
  std::ofstream os;
  if (this->Mode=='a')
    {
    writeHeader=!FileExists(this->FileName.c_str());
    os.open(this->FileName.c_str(),std::ios_base::app);
    }
  else
    {
    os.open(this->FileName.c_str(),std::ios_base::trunc);
    }

  if (!os.good())
    {
    sqErrorMacro(std::cerr,
      << "Failed to open " << this->FileName << " for writing.");
    return 0;
    }

  if (writeHeader)
    {
    os
      << "#################################" << std::endl
      << "# SciberQuestToolKit" << std::endl
      << "# Metadata version 1.1" << std::endl
      << "#################################" << std::endl;

    if (this->DataSetTypeIsImage())
      {
      int nCells[3];
      this->Domain.Size(nCells);

      double x0[3];
      this->GetOrigin(x0);

      double dx[3];
      this->GetSpacing(dx);

      os
        << "nx=" << nCells[0] << ", ny=" << nCells[1] << ", nz=" << nCells[2] << std::endl
        << "x0=" << x0[0] << ", y0=" << x0[1] << ", z0=" << x0[2] << std::endl
        << "dx=" << dx[0] << ", dy=" << dx[1] << ", dz=" << dx[2] << std::endl
        << "dt=" << this->GetDt() << std::endl
        << "ext=" << this->GetBrickFileExtension() << std::endl;
      }
    else
      {
      sqErrorMacro(std::cerr,
        << "Unsuported dataset type " << this->GetDataSetType());
      return 0;
      }

    os << std::endl;
    }

  size_t nArrays=this->GetNumberOfArrays();
  for (size_t i=0; i<nArrays; ++i)
    {
    const char *arrayName=this->GetArrayName(i);

    if (!this->IsArrayActive(arrayName))
      {
      continue;
      }

    if (this->IsArrayScalar(arrayName))
      {
      os << "scalar:" << arrayName << std::endl;
      }
    else
    if (this->IsArrayVector(arrayName))
      {
      os
        << "vector:" << arrayName << std::endl
        << "scalar:" << arrayName << "x" << std::endl
        << "scalar:" << arrayName << "y" << std::endl
        << "scalar:" << arrayName << "z" << std::endl;

      }
    else
    if (this->IsArraySymetricTensor(arrayName))
      {
      os
        << "stensor:" << arrayName << std::endl
        << "scalar:" << arrayName << "-xx" << std::endl
        << "scalar:" << arrayName << "-xy" << std::endl
        << "scalar:" << arrayName << "-xz" << std::endl
        << "scalar:" << arrayName << "-yy" << std::endl
        << "scalar:" << arrayName << "-yz" << std::endl
        << "scalar:" << arrayName << "-zz" << std::endl;
      }
    else
    if (this->IsArrayTensor(arrayName))
      {
      os
        << "tensor:" << arrayName << std::endl
        << "scalar:" << arrayName << "-xx" << std::endl
        << "scalar:" << arrayName << "-xy" << std::endl
        << "scalar:" << arrayName << "-xz" << std::endl
        << "scalar:" << arrayName << "-yx" << std::endl
        << "scalar:" << arrayName << "-yy" << std::endl
        << "scalar:" << arrayName << "-yz" << std::endl
        << "scalar:" << arrayName << "-zx" << std::endl
        << "scalar:" << arrayName << "-zy" << std::endl
        << "scalar:" << arrayName << "-zz" << std::endl;
      }
    else
      {
      sqErrorMacro(std::cerr,"Unsupported array type for " << arrayName);
      return 0;
      }

    }

  os << std::endl;
  os.close();

  return 1;
}

//-----------------------------------------------------------------------------
void GDAMetaData::PushPipelineInformation(
      vtkInformation *req,
      vtkInformation *pinfo)
{
  if (this->HasDipoleCenter)
    {
    pinfo->Set(GDAMetaDataKeys::DIPOLE_CENTER(),this->DipoleCenter,3);
    req->Append(vtkExecutive::KEYS_TO_COPY(),GDAMetaDataKeys::DIPOLE_CENTER());
    }
  // if (this->HasCellSizeRE)
  //   {
  //   pinfo->Set(GDAMetaDataKeys::CELL_SIZE_RE(),this->CellSizeRe);
  //   req->Append(vtkExecutive::KEYS_TO_COPY(),GDAMetaDataKeys::DIPOLE_CENTER());
  //   }
}

//-----------------------------------------------------------------------------
void GDAMetaData::Print(std::ostream &os) const
{
  os << "GDAMetaData:  " << this << std::endl;
  os << "\tDipole:     " << Tuple<double>(this->DipoleCenter,3) << std::endl;
  //os << "\tCellSizeRe: " << this->CellSizeRe << std::endl;

  this->BOVMetaData::Print(os);

  os << std::endl;
}
