/*=====================

  Program:   Visualization Toolkit
  Module:    vtkSQVortexDetect.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=====================*/
#include "vtkSQVortexDetect.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTensor.h"


#include <cmath>

#include <string>
using std::string;

#include <algorithm>
using std::max;

#include<Eigen/Eigenvalues>
using namespace Eigen;

namespace {

// ****************************************************************************
void ComputeVectorGradient(
      vtkAlgorithm *alg,
      double prog0,
      double prog1,
      vtkDataSet *input,
      vtkIdType nCells,
      vtkDataArray *V,
      vtkDoubleArray *gradV)
{
  const vtkIdType nProgSteps=10;
  const vtkIdType progInt=max(nCells/nProgSteps,vtkIdType(1));
  const double progInc=(prog1-prog0)/nProgSteps;
  double prog=prog0;

  gradV->SetNumberOfComponents(9);
  gradV->SetNumberOfTuples(nCells);
  string name="grad-";
  name+=V->GetName();
  gradV->SetName(name.c_str());
  double *pGradV=gradV->GetPointer(0);

  vtkDoubleArray *cellV=vtkDoubleArray::New();
  cellV->SetNumberOfComponents(3);
  cellV->Allocate(3*VTK_CELL_SIZE);

  vtkGenericCell *cell=vtkGenericCell::New();

  // for each cell
  for (vtkIdType cellId=0; cellId<nCells; ++cellId)
    {
    if (!(cellId%progInt))
      {
      alg->UpdateProgress(prog);
      prog+=progInc;
      }

    input->GetCell(cellId,cell);

    double coords[3];
    cell->GetParametricCenter(coords);

    V->GetTuples(cell->PointIds,cellV);

    double *pCellV=cellV->GetPointer(0);

    double grad[9];
    cell->Derivatives(0,coords,pCellV,3,grad);

    for (int i=0; i<9; ++i)
      {
      pGradV[i]=grad[i];
      }
    pGradV+=9;
    }

  cell->Delete();
  cellV->Delete();
}

// ****************************************************************************
void ComputeVorticity(
      vtkAlgorithm *alg,
      double prog0,
      double prog1,
      vtkIdType nCells,
      vtkDoubleArray *gradV,
      vtkDoubleArray *vortV)
{
  const vtkIdType nProgSteps=10;
  const vtkIdType progInt=max(nCells/nProgSteps,vtkIdType(1));
  const double progInc=(prog1-prog0)/nProgSteps;
  double prog=prog0;

  double *pGradV=gradV->GetPointer(0);

  vortV=vtkDoubleArray::New();
  vortV->SetNumberOfComponents(3);
  vortV->SetNumberOfTuples(nCells);
  string name="vorticity-";
  name+=string(gradV->GetName()).substr(5,string::npos);
  vortV->SetName(name.c_str());
  double *pVortV=vortV->GetPointer(0);

  // for each cell
  for (vtkIdType cellId=0; cellId<nCells; ++cellId)
    {
    if (!(cellId%progInt))
      {
      alg->UpdateProgress(prog);
      prog+=progInc;
      }

    pVortV[0]=pGradV[7]-pGradV[5];
    pVortV[1]=pGradV[2]-pGradV[6];
    pVortV[2]=pGradV[3]-pGradV[1];

    pVortV+=3;
    pGradV+=9;
    }
}

// ****************************************************************************
void ComputeHelicity(
      vtkAlgorithm *alg,
      double prog0,
      double prog1,
      vtkIdType nCells,
      vtkDoubleArray *V,
      vtkDoubleArray *vortV,
      vtkDoubleArray *helV)
{
  const vtkIdType nProgSteps=10;
  const vtkIdType progInt=max(nCells/nProgSteps,vtkIdType(1));
  const double progInc=(prog1-prog0)/nProgSteps;
  double prog=prog0;

  double *pHelV=vortV->GetPointer(0);

  helV->SetNumberOfComponents(3);
  helV->SetNumberOfTuples(nCells);
  string name="helicity-";
  name+=V->GetName();
  helV->SetName(name.c_str());
  double *pVortV=helV->GetPointer(0);

  // for each cell
  for (vtkIdType cellId=0; cellId<nCells; ++cellId)
    {
    if (!(cellId%progInt))
      {
      alg->UpdateProgress(prog);
      prog+=progInc;
      }

    pVortV[0]=pHelV[7]-pHelV[5];
    pVortV[1]=pHelV[2]-pHelV[6];
    pVortV[2]=pHelV[3]-pHelV[1];

    pVortV+=3;
    pHelV+=9;
    }
}

/*
// *****************************************************************************
void ComputeLambda2(
      vtkAlgorithm *alg,
      double prog0,
      double prog1,
      vtkIdType nCells,
      vtkDoubleArray *V,
      vtkDoubleArray *vortV,
      vtkDoubleArray *helV)
{
  // input array bounds.
  const int ni=input[1]-input[0]+1;
  const int nj=input[3]-input[2]+1;
  const int nk=input[5]-input[4]+1;
  FlatIndex idx(ni,nj,nk,mode);

  const int iok=(ni<3?0:1);
  const int jok=(nj<3?0:1);
  const int kok=(nk<3?0:1);

  // output array bounds
  const int _ni=output[1]-output[0]+1;
  const int _nj=output[3]-output[2]+1;
  const int _nk=output[5]-output[4]+1;
  FlatIndex _idx(_ni,_nj,_nk,mode);

  // stencil deltas
  const double dx[3]={dX[0]*2.0,dX[1]*2.0,dX[2]*2.0};

  // loop over output in patch coordinates (both patches are in the same space)
  for (int r=output[4]; r<=output[5]; ++r)
    {
    const int _k=r-output[4];
    const int  k=r-input[4];
    for (int q=output[2]; q<=output[3]; ++q)
      {
      const int _j=q-output[2];
      const int  j=q-input[2];
      for (int p=output[0]; p<=output[1]; ++p)
        {
        const int _i=p-output[0];
        const int  i=p-input[0];

        // J: gradient velocity tensor, (jacobian)
        double j11=0.0, j12=0.0, j13=0.0;
        if (iok)
          {
          int vilo_x=3*idx.Index(i-1,j,k);
          int vilo_y=vilo_x+1;
          int vilo_z=vilo_y+1;

          int vihi_x=3*idx.Index(i+1,j,k);
          int vihi_y=vihi_x+1;
          int vihi_z=vihi_y+1;

          j11=(V[vihi_x]-V[vilo_x])/dx[0];
          j12=(V[vihi_y]-V[vilo_y])/dx[0];
          j13=(V[vihi_z]-V[vilo_z])/dx[0];
          }

        double j21=0.0, j22=0.0, j23=0.0;
        if (jok)
          {
          int vjlo_x=3*idx.Index(i,j-1,k);
          int vjlo_y=vjlo_x+1;
          int vjlo_z=vjlo_y+1;

          int vjhi_x=3*idx.Index(i,j+1,k);
          int vjhi_y=vjhi_x+1;
          int vjhi_z=vjhi_y+1;

          j21=(V[vjhi_x]-V[vjlo_x])/dx[1];
          j22=(V[vjhi_y]-V[vjlo_y])/dx[1];
          j23=(V[vjhi_z]-V[vjlo_z])/dx[1];
          }

        double j31=0.0, j32=0.0, j33=0.0;
        if (kok)
          {
          int vklo_x=3*idx.Index(i,j,k-1);
          int vklo_y=vklo_x+1;
          int vklo_z=vklo_y+1;

          int vkhi_x=3*idx.Index(i,j,k+1);
          int vkhi_y=vkhi_x+1;
          int vkhi_z=vkhi_y+1;

          j31=(V[vkhi_x]-V[vklo_x])/dx[2];
          j32=(V[vkhi_y]-V[vklo_y])/dx[2];
          j33=(V[vkhi_z]-V[vklo_z])/dx[2];
          }

        Matrix<double,3,3> J;
        J <<
          j11, j12, j13,
          j21, j22, j23,
          j31, j32, j33;

        // construct pressure corrected hessian
        Matrix<double,3,3> S=0.5*(J+J.transpose());
        Matrix<double,3,3> W=0.5*(J-J.transpose());
        Matrix<double,3,3> HP=S*S+W*W;

        // compute eigen values, lambda
        Matrix<double,3,1> e;
        SelfAdjointEigenSolver<Matrix<double,3,3> >solver(HP,false);
        e=solver.eigenvalues();  // input array bounds.

        const int pi=_idx.Index(_i,_j,_k);
        / *
        const int vi=3*pi;
        const int vj=vi+1;
        const int vk=vi+2;
        * /

        // extract lambda-2
        slowSort(e.data(),0,3);
        L2[pi]=e(1,0);
        }
      }
    }
}
*/


// ****************************************************************************
void ComputeFTLE(
      vtkAlgorithm *alg,
      double prog0,
      double prog1,
      vtkIdType nCells,
      vtkDoubleArray *gradV,
      vtkDoubleArray *ftleV)
{
  const vtkIdType nProgSteps=10;
  const vtkIdType progInt=max(nCells/nProgSteps,vtkIdType(1));
  const double progInc=(prog1-prog0)/nProgSteps;
  double prog=prog0;

  double *pGradV=gradV->GetPointer(0);

  ftleV->SetNumberOfComponents(1);
  ftleV->SetNumberOfTuples(nCells);
  ftleV->SetName("ftle");
  double *pFtleV=ftleV->GetPointer(0);

  // for each cell
  for (vtkIdType cellId=0; cellId<nCells; ++cellId)
    {
    if (!(cellId%progInt))
      {
      alg->UpdateProgress(prog);
      prog+=progInc;
      }

    Matrix<double,3,3> J;
    J <<
      pGradV[0], pGradV[1], pGradV[2],
      pGradV[3], pGradV[4], pGradV[5],
      pGradV[6], pGradV[7], pGradV[8];

    Matrix<double,3,3> JJT;
    JJT=J*J.transpose();

    // compute eigen values
    Matrix<double,3,1> e;
    SelfAdjointEigenSolver<Matrix<double,3,3> >solver(JJT,false);
    e=solver.eigenvalues();

    double lam;
    lam=max(e(0,0),e(1,0));
    lam=max(lam,e(2,0));
    lam=max(lam,1.0);

    pFtleV[0]=log(sqrt(lam));

    pFtleV+=1;
    pGradV+=9;
    }
}

// ****************************************************************************
template <typename T>
void ComputeMagnitude(size_t nTups, size_t nComps,  T *V, T *mV)
{
  for (size_t i=0; i<nTups; ++i)
    {
    double mag=0.0;
    size_t ii=nComps*i;
    for (size_t q=0; q<nComps; ++q)
      {
      double v=V[ii+q];
      mag+=v*v;
      }
    mV[i]=sqrt(mag);
    }
}

};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQVortexDetect);

//-----------------------------------------------------------------------------
vtkSQVortexDetect::vtkSQVortexDetect()
      :
  PassInput(0),
  SplitComponents(0),
  ResultMagnitude(0),
  ComputeGradient(0),
  ComputeVorticity(0),
  ComputeHelicity(0),
  ComputeNormalizedHelicity(0),
  ComputeQ(0),
  ComputeLambda2(0),
  ComputeDivergence(0),
  ComputeFTLE(0),
  ComputeEigenvalueDiagnostic(0)
{
  #ifdef vtkSQVortexDetectDEBUG
  pCerr() << "=====vtkSQVortexDetect::vtkSQVortexDetect" << endl;
  #endif

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  // by default process active point vectors
  this->SetInputArrayToProcess(
        0,
        0,
        0,
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vtkDataSetAttributes::VECTORS);
}

//-----------------------------------------------------------------------------
int vtkSQVortexDetect::Initialize(vtkPVXMLElement *root)
{
  #ifdef vtkSQVortexDetectDEBUG
  pCerr() << "=====vtkSQVortexDetect::Initialize" << endl;
  #endif

  return 0;
}

//-----------------------------------------------------------------------------
int vtkSQVortexDetect::RequestData(
      vtkInformation *vtkNotUsed(request),
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQVortexDetectDEBUG
  pCerr() << "=====vtkSQVortexDetect::RequestData" << endl;
  #endif

  vtkInformation *inInfo = inInfos[0]->GetInformationObject(0);
  vtkInformation *outInfo = outInfos->GetInformationObject(0);

  vtkDataSet *input
     = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
    {
    vtkErrorMacro("Null input.");
    return 1;
    }

  vtkDataSet *output
     = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
    {
    vtkErrorMacro("Null output.");
    return 1;
    }

  output->CopyStructure(input);
  if (this->PassInput)
    {
    output->CopyAttributes(input);
    }

  vtkIdType nCells=input->GetNumberOfCells();
  if (nCells<1)
    {
    vtkErrorMacro("No cells on input.");
    return 1;
    }

  vtkDataArray *V=this->GetInputArrayToProcess(0,inInfos);

  // Gradient.
  vtkDoubleArray *gradV=vtkDoubleArray::New();
  ::ComputeVectorGradient(this,0.0,0.1,input,nCells,V,gradV);
  if (this->ComputeGradient)
    {
    output->GetCellData()->AddArray(gradV);
    }
  this->UpdateProgress(0.1);

  if (this->ComputeVorticity || this->ComputeHelicity || this->ComputeNormalizedHelicity)
    {
    // Vorticity.
    vtkDoubleArray *vortV=vtkDoubleArray::New();
    ::ComputeVorticity(this,0.1,0.2,nCells,gradV,vortV);
    if (this->ComputeVorticity)
      {
      output->GetCellData()->AddArray(vortV);
      }
    this->UpdateProgress(0.2);

    // Helicity.
    if (this->ComputeHelicity)
      {
      }
    this->UpdateProgress(0.3);

    // Normalized Helicity.
    if (this->ComputeNormalizedHelicity)
      {
      }

    vortV->Delete();
    }
  this->UpdateProgress(0.4);

  // Q Criteria
  if (this->ComputeQ)
    {
    }
  this->UpdateProgress(0.5);

  // Lambda-2.
  if (this->ComputeLambda2)
    {
    }
  this->UpdateProgress(0.6);

  // Divergence.
  if (this->ComputeDivergence)
    {
    }
  this->UpdateProgress(0.7);

  // EigenvalueDiagnostic.
  if (this->ComputeEigenvalueDiagnostic)
    {
    }
  this->UpdateProgress(0.8);

  // FTLE
  if (this->ComputeFTLE)
    {
    vtkDoubleArray *ftleV=vtkDoubleArray::New();
    ::ComputeFTLE(this,0.8,0.9,nCells,gradV,ftleV);
    output->GetCellData()->AddArray(ftleV);
    ftleV->Delete();
    }
  this->UpdateProgress(0.9);

  // magnitidue
  if (this->ResultMagnitude)
    {
    int nOutArrays=output->GetCellData()->GetNumberOfArrays();
    for (int i=0; i<nOutArrays; ++i)
      {
      vtkDataArray *da=output->GetCellData()->GetArray(i);
      size_t daNc=da->GetNumberOfComponents();
      if (daNc==1)
        {
        continue;
        }
      vtkDataArray *mda=da->NewInstance();
      size_t daNt=da->GetNumberOfTuples();
      mda->SetNumberOfTuples(daNt);
      string name="mag-";
      name+=da->GetName();
      mda->SetName(name.c_str());
      output->GetPointData()->AddArray(mda);
      mda->Delete();
      switch(V->GetDataType())
        {
        vtkTemplateMacro(
          ::ComputeMagnitude<VTK_TT>(
              daNt,
              daNc,
              (VTK_TT*)da->GetVoidPointer(0),
              (VTK_TT*)mda->GetVoidPointer(0)));
        }
      }
    }
  this->UpdateProgress(1.0);

  gradV->Delete();

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQVortexDetect::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

/*
vtk code for eigen values.

  double ux=pV[0];
  double uy=pV[1];
  double uz=pV[2];
  double vx=pV[3];
  double vy=pV[4];
  double vz=pV[5];
  double wx=pV[6];
  double wy=pV[7];
  double wz=pV[8];

  double JJT0[3], JJT1[3], JJT2[3];
  double *JJT[3]={JJT0, JJT1, JJT2};

  JJT[0][0] = ux*ux+vx*vx+wx*wx;
  JJT[0][1] = ux*uy+vx*vy+wx*wy;
  JJT[0][2] = ux*uz+vx*vz+wx*wz;
  JJT[1][0] = JJT[0][1];
  JJT[1][1] = uy*uy+vy*vy+uz*wz;
  JJT[1][2] = uy*uz+vy*vz+wy*wz;
  JJT[2][0] = JJT[0][2];
  JJT[2][1] = JJT[1][2];
  JJT[2][2] = uz*uz+vz*vz+wz*wz;
*/
