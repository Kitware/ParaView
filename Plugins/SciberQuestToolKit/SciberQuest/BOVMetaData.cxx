/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "BOVMetaData.h"

#include "SharedArray.hxx"
#include "Tuple.hxx"
#include "PrintUtils.h"


//-----------------------------------------------------------------------------
BOVMetaData::BOVMetaData()
{
  this->Mode = 'c';
  this->IsOpen=0;
  this->BrickFileExtension="brick";
  this->Origin[0]=
  this->Origin[1]=
  this->Origin[2]=0.0;
  this->Spacing[0]=
  this->Spacing[1]=
  this->Spacing[2]=1.0;
  this->Coordinates[0]=SharedArray<float>::New();
  this->Coordinates[1]=SharedArray<float>::New();
  this->Coordinates[2]=SharedArray<float>::New();
  this->Dt=1.0;
}

//-----------------------------------------------------------------------------
BOVMetaData::~BOVMetaData()
{
  this->Coordinates[0]->Delete();
  this->Coordinates[1]->Delete();
  this->Coordinates[2]->Delete();
}

//-----------------------------------------------------------------------------
BOVMetaData &BOVMetaData::operator=(const BOVMetaData &other)
{
  if (this==&other)
    {
    return *this;
    }

  this->Mode=other.Mode;
  this->IsOpen=other.IsOpen;
  this->FileName=other.FileName;
  this->PathToBricks=other.PathToBricks;
  this->BrickFileExtension=other.BrickFileExtension;
  this->Arrays=other.Arrays;
  this->TimeSteps=other.TimeSteps;
  this->Domain=other.Domain;
  this->Subset=other.Subset;
  this->Decomp=other.Decomp;
  this->DataSetType=other.DataSetType;
  this->SetOrigin(other.GetOrigin());
  this->SetSpacing(other.GetSpacing());
  this->SetDt(other.GetDt());
  this->Coordinates[0]->Assign(other.Coordinates[0]);
  this->Coordinates[1]->Assign(other.Coordinates[1]);
  this->Coordinates[2]->Assign(other.Coordinates[2]);

  return *this;
}

//-----------------------------------------------------------------------------
int BOVMetaData::CloseDataset()
{
  this->Mode='c';
  this->IsOpen=0;
  this->FileName="";
  this->PathToBricks="";
  this->BrickFileExtension="";
  this->Domain.Clear();
  this->Subset.Clear();
  this->Decomp.Clear();
  this->Arrays.clear();
  this->TimeSteps.clear();
  this->DataSetType="";
  this->SetOrigin(0.0,0.0,0.0);
  this->SetSpacing(1.0,1.0,1.0);
  this->SetDt(1.0);
  this->Coordinates[0]->Clear();
  this->Coordinates[1]->Clear();
  this->Coordinates[2]->Clear();

  return 1;
}

//-----------------------------------------------------------------------------
void BOVMetaData::SetDomain(const CartesianExtent &domain)
{
  this->Domain=domain;

  if (this->Subset.Empty())
    {
    this->SetSubset(domain);
    }
}

//-----------------------------------------------------------------------------
void BOVMetaData::SetSubset(const CartesianExtent &subset)
{
  this->Subset=subset;

  if (this->Decomp.Empty())
    {
    this->Decomp=subset;
    }
}

//-----------------------------------------------------------------------------
void BOVMetaData::SetDecomp(const CartesianExtent &decomp)
{
  this->Decomp=decomp;
}

//-----------------------------------------------------------------------------
void BOVMetaData::SetOrigin(const double *origin)
{
  this->Origin[0]=origin[0];
  this->Origin[1]=origin[1];
  this->Origin[2]=origin[2];
}

//-----------------------------------------------------------------------------
void BOVMetaData::SetOrigin(double x0, double y0, double z0)
{
  double X0[3]={x0,y0,z0};
  this->SetOrigin(X0);
}

//-----------------------------------------------------------------------------
void BOVMetaData::GetOrigin(double *origin) const
{
  origin[0]=this->Origin[0];
  origin[1]=this->Origin[1];
  origin[2]=this->Origin[2];
}

//-----------------------------------------------------------------------------
void BOVMetaData::SetSpacing(const double *spacing)
{
  this->Spacing[0]=spacing[0];
  this->Spacing[1]=spacing[1];
  this->Spacing[2]=spacing[2];
}

//-----------------------------------------------------------------------------
void BOVMetaData::SetSpacing(double dx, double dy, double dz)
{
  double dX[3]={dx,dy,dz};
  this->SetSpacing(dX);
}

//-----------------------------------------------------------------------------
void BOVMetaData::GetSpacing(double *spacing) const
{
  spacing[0]=this->Spacing[0];
  spacing[1]=this->Spacing[1];
  spacing[2]=this->Spacing[2];
}

//-----------------------------------------------------------------------------
float *BOVMetaData::SubsetCoordinate(int q, CartesianExtent &ext) const
{
  int N[3];
  ext.Size(N);

  const float *coord=this->GetCoordinate(q)->GetPointer();
  float *scoord=(float *)malloc(N[q]*sizeof(float));

  for (int i=0,s=ext[2*q],e=ext[2*q+1]; s<=e; ++i,++s)
    {
    scoord[i]=coord[s];
    }

  return scoord;
}

//-----------------------------------------------------------------------------
size_t BOVMetaData::GetNumberOfArrayFiles() const
{
  size_t nFiles=0;
  std::map<std::string,int>::const_iterator it=this->Arrays.begin();
  std::map<std::string,int>::const_iterator end=this->Arrays.end();
  for (;it!=end; ++it)
    {
    if (it->second&SCALAR_BIT)
      {
      nFiles+=1;
      }
    else
    if (it->second&VECTOR_BIT)
      {
      nFiles+=3;
      }
    else
    if (it->second&TENSOR_BIT)
      {
      nFiles+=6;
      }
    else
    if (it->second&SYM_TENSOR_BIT)
      {
      nFiles+=9;
      }
    }
  return nFiles;
}

//-----------------------------------------------------------------------------
const char *BOVMetaData::GetArrayName(size_t i) const
{
  std::map<std::string,int>::const_iterator it=this->Arrays.begin();
  while(i--) it++;
  return it->first.c_str();
}

//-----------------------------------------------------------------------------
void BOVMetaData::DeactivateAllArrays()
{
  size_t nArrays=this->GetNumberOfArrays();
  for (size_t i=0; i<nArrays; ++i)
    {
    const char *arrayName=this->GetArrayName(i);
    this->DeactivateArray(arrayName);
    }
}

//-----------------------------------------------------------------------------
void BOVMetaData::ActivateAllArrays()
{
  size_t nArrays=this->GetNumberOfArrays();
  for (size_t i=0; i<nArrays; ++i)
    {
    const char *arrayName=this->GetArrayName(i);
    this->ActivateArray(arrayName);
    }
}

//-----------------------------------------------------------------------------
void BOVMetaData::Pack(BinaryStream &os)
{
  os.Pack(this->Mode);
  os.Pack(this->IsOpen);
  os.Pack(this->FileName);
  os.Pack(this->PathToBricks);
  os.Pack(this->BrickFileExtension);
  os.Pack(this->Domain.GetData(),6);
  os.Pack(this->Decomp.GetData(),6);
  os.Pack(this->Subset.GetData(),6);
  os.Pack(this->Arrays);
  os.Pack(this->TimeSteps);
  os.Pack(this->DataSetType);
  os.Pack(this->Origin,3);
  os.Pack(this->Spacing,3);
  os.Pack(this->Dt);
  os.Pack(*this->Coordinates[0]);
  os.Pack(*this->Coordinates[1]);
  os.Pack(*this->Coordinates[2]);
}

//-----------------------------------------------------------------------------
void BOVMetaData::UnPack(BinaryStream &is)
{
  is.UnPack(this->Mode);
  is.UnPack(this->IsOpen);
  is.UnPack(this->FileName);
  is.UnPack(this->PathToBricks);
  is.UnPack(this->BrickFileExtension);
  is.UnPack(this->Domain.GetData(),6);
  is.UnPack(this->Decomp.GetData(),6);
  is.UnPack(this->Subset.GetData(),6);
  is.UnPack(this->Arrays);
  is.UnPack(this->TimeSteps);
  is.UnPack(this->DataSetType);
  is.UnPack(this->Origin,3);
  is.UnPack(this->Spacing,3);
  is.UnPack(this->Dt);
  is.UnPack(*this->Coordinates[0]);
  is.UnPack(*this->Coordinates[1]);
  is.UnPack(*this->Coordinates[2]);
}

//-----------------------------------------------------------------------------
void BOVMetaData::Print(std::ostream &os) const
{
  os
    << "BOVMetaData: " << this << std::endl
    << "\tMode: " << this->Mode << std::endl
    << "\tIsOpen: " << this->IsOpen << std::endl
    << "\tFileName: " << this->FileName << std::endl
    << "\tPathToBricks: " << this->PathToBricks << std::endl
    << "\tBrickFileExtension: " << this->BrickFileExtension << std::endl
    << "\tDomain: " << this->Domain << std::endl
    << "\tSubset: " << this->Subset << std::endl
    << "\tDecomp: " << this->Decomp << std::endl
    << "\tArrays: " << this->Arrays << std::endl
    << "\tTimeSteps: " << this->TimeSteps << std::endl
    << "\tDataSetType: " << this->DataSetType << std::endl
    << "\tOrigin: " << Tuple<double>(this->Origin,3) << std::endl
    << "\tSpacing: " << Tuple<double>(this->Spacing,3) << std::endl
    << "\tDt: " << this->Dt << std::endl
    << "\tCoordinates: " << std::endl
    << "\t\t" << *this->Coordinates[0] << std::endl
    << "\t\t" << *this->Coordinates[1] << std::endl
    << "\t\t" << *this->Coordinates[2] << std::endl
    << std::endl;
}

//-----------------------------------------------------------------------------
std::ostream &operator<<(std::ostream &os, const BOVMetaData &md)
{
  md.Print(os);
  return os;
}
