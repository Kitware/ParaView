/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQFTLE.h"

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

#include "SQEigenWarningSupression.h"
#include <Eigen/Eigenvalues>
using namespace Eigen;

// ****************************************************************************
static
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
static
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

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQFTLE);

//-----------------------------------------------------------------------------
vtkSQFTLE::vtkSQFTLE() : PassInput(0)
{
  #ifdef vtkSQFTLEDEBUG
  pCerr() << "=====vtkSQFTLE::vtkSQFTLE" << endl;
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
int vtkSQFTLE::Initialize(vtkPVXMLElement *root)
{
  #ifdef vtkSQFTLEDEBUG
  pCerr() << "=====vtkSQFTLE::Initialize" << endl;
  #endif

  (void)root;

  return 0;
}

//-----------------------------------------------------------------------------
int vtkSQFTLE::RequestData(
      vtkInformation *vtkNotUsed(request),
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQFTLEDEBUG
  pCerr() << "=====vtkSQFTLE::RequestData" << endl;
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

  // Gradient.
  vtkDataArray *V=this->GetInputArrayToProcess(0,inInfos);
  vtkDoubleArray *gradV=vtkDoubleArray::New();
  ComputeVectorGradient(this,0.0,0.4,input,nCells,V,gradV);

  // FTLE
  vtkDoubleArray *ftleV=vtkDoubleArray::New();
  ComputeFTLE(this,0.5,1.0,nCells,gradV,ftleV);
  output->GetCellData()->AddArray(ftleV);
  ftleV->Delete();
  gradV->Delete();

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQFTLE::PrintSelf(ostream& os, vtkIndent indent)
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
