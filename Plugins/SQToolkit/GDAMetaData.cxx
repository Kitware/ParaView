/*   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "GDAMetaData.h"
#include "GDAMetaDataKeys.h"

#include <iostream>
using std::ostream;
using std::endl;

#include <sstream>
using std::istringstream;
using std::ostringstream;

#include "vtkInformation.h"
#include "vtkExecutive.h"

#include "PrintUtils.h"
#include "FsUtils.h"
#include "SQMacros.h"
#include "Tuple.hxx"

//*****************************************************************************
void ToLower(string &in)
{
  size_t n=in.size();
  for (size_t i=0; i<n; ++i)
    {
    in[i]=tolower(in[i]);
    }
}

// Parse a string for a "key", starting at offset "at" then 
// advance past the key and attempt to convert what follows
// in to a value of type "T". If the key isn't found, then 
// npos is returned otherwise the position imediately following
// the key is returned.
//*****************************************************************************
template <typename T>
size_t ParseValue(string &in,size_t at, string key, T &value)
{
  size_t p=in.find(key,at);
  if (p!=string::npos)
    {
    p+=key.size();
    istringstream valss(in.substr(p,10));
    valss >> value;
    }
  return p;
}

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
int GDAMetaData::OpenDataset(const char *fileName)
{
  this->IsOpen=0;

  // Open
  ifstream metaFile(fileName);
  if (!metaFile.is_open())
    {
    sqErrorMacro(cerr,"Could not open " << fileName << ".");
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
  string metaData(metaDataBuffer);
  free(metaDataBuffer); // TODO use string's memory directly

  // Parse
  ToLower(metaData);

  // We are expecting are nx,ny, and nz in the header for all
  // mesh types.
  int nx,ny,nz;
  if ( ParseValue(metaData,0,"nx=",nx)==string::npos
    || ParseValue(metaData,0,"ny=",ny)==string::npos
    || ParseValue(metaData,0,"nz=",nz)==string::npos )
    {
    sqErrorMacro(cerr,
         << "Parsing " << fileName
         << " dimensions not found. Expected nx=N, ny=M, nz=P.");
    return 0;
    }
  CartesianExtent domain(0,nx-1,0,ny-1,0,nz-1);
  this->SetDomain(domain);

  if (Present(path,"x.gda")
    && Present(path,"y.gda")
    && Present(path,"z.gda"))
    {
    // mark as stretched mesh
    this->SetDataSetType("vtkRectilinearGrid");

    // read the coordinate arrays here. Even for large grids
    // this represents a small ammount of data thus it's
    // probably better to read on proc 0 and distribute over
    // the network.
    size_t n[3]={nx+1,ny+1,nz+1};
    char coordId[]="xyz";
    ostringstream coordFn;

    for (int q=0; q<3; ++q)
      {
      // read coordinate array
      coordFn.str("");
      coordFn << path << PATH_SEP << coordId[q] << ".gda";

      SharedArray<float> *coord=this->GetCoordinate(q);
      coord->Resize(n[q]);

      float *pCoord=coord->GetPointer();

      if (LoadBin(coordFn.str().c_str(),n[q],pCoord)==0)
        {
        sqErrorMacro(cerr,
          << "Error: Failed to read coordinate "
          << q << " from \"" << coordFn.str() << "\".");
        return 0;
        }

      // shift on to the dual grid
      size_t nc=n[q]-1;

      for (size_t i=0; i<nc; ++i)
        {
        pCoord[i]=(pCoord[i]+pCoord[i+1])/2.0;
        }

      coord->Resize(nc);
      }
    }
  else
    {
    // mark as uniform grid
    this->SetDataSetType("vtkImageData");

    double x0,y0,z0;
    if ( ParseValue(metaData,0,"x0=",x0)==string::npos
      || ParseValue(metaData,0,"y0=",y0)==string::npos
      || ParseValue(metaData,0,"z0=",z0)==string::npos )
      {
      // if no origin provided assume 0,0,0
      x0=y0=z0=0.0;
      }
    double X0[3]={x0,y0,z0};
    this->SetOrigin(X0);

    double dx,dy,dz;
    if ( ParseValue(metaData,0,"dx=",dx)==string::npos
      || ParseValue(metaData,0,"dy=",dy)==string::npos
      || ParseValue(metaData,0,"dz=",dz)==string::npos )
      {
      // if no spacing is provided assume 1,1,1
      dx=dy=dz=1.0;
      }
    double dX[3]={dx,dy,dz};
    this->SetSpacing(dX);
    }

  // TODO
  // the following meta data enhancments are disabled until
  // I add the virtual pack/unpack methods, so that the process
  // that touches disk can actually stream them to the other
  // processes

  // // Look for the dipole center
  // double di_i,di_j,di_k;
  // if ( ParseValue(metaData,0,"i_dipole=",di_i)==string::npos
  //   || ParseValue(metaData,0,"j_dipole=",di_j)==string::npos
  //   || ParseValue(metaData,0,"k_dipole=",di_k)==string::npos)
  //   {
  //   // cerr << __LINE__ << " Warning: Parsing " << fileName
  //   //       << " dipole center not found." << endl;
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
  // if ( ParseValue(metaData,0,"R_MP=",r_mp)==string::npos
  //   || ParseValue(metaData,0,"R_obstacle_to_MP=",r_obs_to_mp)==string::npos)
  //   {
  //   cerr << __LINE__ << " Warning: Parsing " << fileName 
  //         << " magnetopause dimension not found." << endl;
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

  // H3D scalars ...
  int nArrays=0;
  if (Represented(path,"den_"))
    {
    this->AddScalar("den");
    ++nArrays;
    }
  if (Represented(path,"eta_"))
    {
    this->AddScalar("eta");
    ++nArrays;
    }
  if (Represented(path,"tpar_"))
    {
    this->AddScalar("tpar");
    ++nArrays;
    }
  if (Represented(path,"tperp_"))
    {
    this->AddScalar("tperp");
    ++nArrays;
    }
  if (Represented(path,"t_"))
    {
    this->AddScalar("t");
    ++nArrays;
    }
  if (Represented(path,"p_"))
    {
    this->AddScalar("p");
    ++nArrays;
    }

  // VPIC scalars
  if (Represented(path,"misc_"))
    {
    this->AddScalar("misc");
    ++nArrays;
    }
  if (Represented(path,"pe_"))
    {
    this->AddScalar("pe");
    ++nArrays;
    }
  if (Represented(path,"pi_"))
    {
    this->AddScalar("pi");
    ++nArrays;
    }
  if (Represented(path,"ne_"))
    {
    this->AddScalar("ne");
    ++nArrays;
    }
  if (Represented(path,"ni_"))
    {
    this->AddScalar("ni");
    ++nArrays;
    }
  if (Represented(path,"eeb01_"))
    {
    this->AddScalar("eeb01");
    ++nArrays;
    }
  if (Represented(path,"eeb02_"))
    {
    this->AddScalar("eeb02");
    ++nArrays;
    }
  if (Represented(path,"eeb03_"))
    {
    this->AddScalar("eeb03");
    ++nArrays;
    }
  if (Represented(path,"eeb04_"))
    {
    this->AddScalar("eeb04");
    ++nArrays;
    }
  if (Represented(path,"eeb05_"))
    {
    this->AddScalar("eeb05");
    ++nArrays;
    }
  if (Represented(path,"eeb06_"))
    {
    this->AddScalar("eeb06");
    ++nArrays;
    }
  if (Represented(path,"eeb07_"))
    {
    this->AddScalar("eeb07");
    ++nArrays;
    }
  if (Represented(path,"eeb08_"))
    {
    this->AddScalar("eeb08");
    ++nArrays;
    }
  if (Represented(path,"eeb09_"))
    {
    this->AddScalar("eeb09");
    ++nArrays;
    }
  if (Represented(path,"eeb10_"))
    {
    this->AddScalar("eeb10");
    ++nArrays;
    }
  if (Represented(path,"ieb01_"))
    {
    this->AddScalar("ieb01");
    ++nArrays;
    }
  if (Represented(path,"ieb02_"))
    {
    this->AddScalar("ieb02");
    ++nArrays;
    }
  if (Represented(path,"ieb03_"))
    {
    this->AddScalar("ieb03");
    ++nArrays;
    }
  if (Represented(path,"ieb04_"))
    {
    this->AddScalar("ieb04");
    ++nArrays;
    }
  if (Represented(path,"ieb05_"))
    {
    this->AddScalar("ieb05");
    ++nArrays;
    }
  if (Represented(path,"ieb06_"))
    {
    this->AddScalar("ieb06");
    ++nArrays;
    }
  if (Represented(path,"ieb07_"))
    {
    this->AddScalar("ieb07");
    ++nArrays;
    }
  if (Represented(path,"ieb08_"))
    {
    this->AddScalar("ieb08");
    ++nArrays;
    }
  if (Represented(path,"ieb09_"))
    {
    this->AddScalar("ieb09");
    ++nArrays;
    }
  if (Represented(path,"ieb10_"))
    {
    this->AddScalar("ieb10");
    ++nArrays;
    }

  // H3D vectors
  if (Represented(path,"bx_")
    && Represented(path,"by_")
    && Represented(path,"bz_"))
    {
    this->AddVector("b");
    ++nArrays;
    }
  if (Represented(path,"ex_")
    && Represented(path,"ey_")
    && Represented(path,"ez_"))
    {
    this->AddVector("e");
    ++nArrays;
    }
  if (Represented(path,"vix_")
    && Represented(path,"viy_")
    && Represented(path,"viz_"))
    {
    this->AddVector("vi");
    ++nArrays;
    }
  // 2d vector projections
  if (Represented(path,"bpx_")
    && Represented(path,"bpy_")
    && Represented(path,"bpz_"))
    {
    this->AddVector("bp");
    ++nArrays;
    }
  if (Represented(path,"epx_")
    && Represented(path,"epy_")
    && Represented(path,"epz_"))
    {
    this->AddVector("ep");
    ++nArrays;
    }
  if (Represented(path,"vipx_")
    && Represented(path,"vipy_")
    && Represented(path,"vipz_"))
    {
    this->AddVector("vip");
    ++nArrays;
    }
 // VPIC vectors
 if (Represented(path,"vex_")
    && Represented(path,"vey_")
    && Represented(path,"vez_"))
    {
    this->AddVector("ve");
    ++nArrays;
    }
 if (Represented(path,"uix_")
    && Represented(path,"uiy_")
    && Represented(path,"uiz_"))
    {
    this->AddVector("ui");
    ++nArrays;
    }
 if (Represented(path,"uex_")
    && Represented(path,"uey_")
    && Represented(path,"uez_"))
    {
    this->AddVector("ue");
    ++nArrays;
    }
 if (Represented(path,"ax_")
    && Represented(path,"ay_")
    && Represented(path,"az_"))
    {
    this->AddVector("a");
    ++nArrays;
    }
 if (Represented(path,"jx_")
    && Represented(path,"jy_")
    && Represented(path,"jz_"))
    {
    this->AddVector("j");
    ++nArrays;
    }
  // projections
 if (Represented(path,"vepx_")
    && Represented(path,"vepy_")
    && Represented(path,"vepz_"))
    {
    this->AddVector("vep");
    ++nArrays;
    }
 if (Represented(path,"uipx_")
    && Represented(path,"uipy_")
    && Represented(path,"uipz_"))
    {
    this->AddVector("uip");
    ++nArrays;
    }
 if (Represented(path,"uepx_")
    && Represented(path,"uepy_")
    && Represented(path,"uepz_"))
    {
    this->AddVector("uep");
    ++nArrays;
    }
 if (Represented(path,"apx_")
    && Represented(path,"apy_")
    && Represented(path,"apz_"))
    {
    this->AddVector("ap");
    ++nArrays;
    }
 if (Represented(path,"jpx_")
    && Represented(path,"jpy_")
    && Represented(path,"jpz_"))
    {
    this->AddVector("jp");
    ++nArrays;
    }
 if (Represented(path,"vmiscx_")
    && Represented(path,"vmiscy_")
    && Represented(path,"vmiscz_"))
    {
    this->AddVector("vmisc");
    ++nArrays;
    }
  // VPIC tensors
  if (Represented(path,"pe-xx_")
    && Represented(path,"pe-xy_")
    && Represented(path,"pe-xz_")
    && Represented(path,"pe-yy_")
    && Represented(path,"pe-yz_")
    && Represented(path,"pe-zz_"))
    {
    this->AddSymetricTensor("pe");
    ++nArrays;
    }
  if (Represented(path,"pi-xx_")
    && Represented(path,"pi-xy_")
    && Represented(path,"pi-xz_")
    && Represented(path,"pi-yy_")
    && Represented(path,"pi-yz_")
    && Represented(path,"pi-zz_"))
    {
    this->AddSymetricTensor("pi");
    ++nArrays;
    }

  // We had to find at least one brick, otherwise we have problems.
  // As long as there is at least one brick, generate the series ids.
  if (nArrays)
    {
    const char *arrayName=this->GetArrayName(0);
    string prefix(arrayName);
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
      sqErrorMacro(cerr,
        << " Error: Time series was not found in " << path << "."
        << " Expected files named according to the following convention \"array_time.ext\".");
      return 0;
      }
    }
  else
    {
    sqErrorMacro(cerr,
      << " Error: No bricks found in " << path << "."
      << " Expected bricks in the same directory as the metdata file.");
    return 0;
    }

  this->IsOpen=1;

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
void GDAMetaData::Print(ostream &os) const
{
  os << "GDAMetaData:  " << this << endl;
  os << "\tDipole:     " << Tuple<double>(this->DipoleCenter,3) << endl;
  //os << "\tCellSizeRe: " << this->CellSizeRe << endl;

  this->BOVMetaData::Print(os);

  os << endl;
}
