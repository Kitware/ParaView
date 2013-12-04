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
#include "vtkPVXMLElement.h"
#include "XMLUtils.h"
#include "vtkSQLog.h"

#include <cmath>
#include <string>
#include <algorithm>

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
  const vtkIdType progInt=std::max(nCells/nProgSteps,vtkIdType(1));
  const double progInc=(prog1-prog0)/nProgSteps;
  double prog=prog0;

  gradV->SetNumberOfComponents(9);
  gradV->SetNumberOfTuples(nCells);
  std::string name="grad-";
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
      double timeInterval,
      vtkDoubleArray *ftleV)
{
  const vtkIdType nProgSteps=10;
  const vtkIdType progInt=std::max(nCells/nProgSteps,vtkIdType(1));
  const double progInc=(prog1-prog0)/nProgSteps;
  double prog=prog0;

  double *pGradV=gradV->GetPointer(0);

  ftleV->SetNumberOfComponents(1);
  ftleV->SetNumberOfTuples(nCells);
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
    lam=std::max(e(0,0),e(1,0));
    lam=std::max(lam,e(2,0));
    lam=std::max(lam,1.0);

    pFtleV[0]=log(sqrt(lam))/timeInterval;

    pFtleV+=1;
    pGradV+=9;
    }
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQFTLE);

//-----------------------------------------------------------------------------
vtkSQFTLE::vtkSQFTLE()
      :
  PassInput(0),
  TimeInterval(1.0),
  LogLevel(0)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQFTLE::vtkSQFTLE" << std::endl;
  #endif

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
int vtkSQFTLE::Initialize(vtkPVXMLElement *root)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQFTLE::Initialize" << std::endl;
  #endif

  vtkPVXMLElement *elem=0;
  elem=GetOptionalElement(root,"vtkSQFTLE");
  if (elem==0)
    {
    return -1;
    }

  // input arrays, optional but must be set somewhwere
  vtkPVXMLElement *nelem;
  nelem=GetOptionalElement(elem,"input_arrays");
  if (nelem)
    {
    ExtractValues(nelem->GetCharacterData(),this->InputArrays);
    }

  int passInput=0;
  GetOptionalAttribute<int,1>(elem,"pass_input",&passInput);
  if (passInput>0)
    {
    this->SetPassInput(passInput);
    }

  double timeInterval=0.0;
  GetOptionalAttribute<double,1>(elem,"time_interval",&timeInterval);
  if (timeInterval>0.0)
    {
    this->SetTimeInterval(timeInterval);
    }

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
    log->GetHeader()
      << "# ::vtkSQFTLE" << "\n"
      << "#   pass_input=" << this->PassInput << "\n"
      << "#   time_interval=" << this->TimeInterval << "\n"
      << "#   input_arrays=";

    std::set<std::string>::iterator it=this->InputArrays.begin();
    std::set<std::string>::iterator end=this->InputArrays.end();
    for (; it!=end; ++it)
      {
      log->GetHeader() << *it << " ";
      }
    log->GetHeader() << "\n";
    }

  return 0;
}

//-----------------------------------------------------------------------------
void vtkSQFTLE::AddInputArray(const char *name)
{
  #ifdef SQTK_DEBUG
  pCerr()
    << "=====vtkSQFTLE::AddInputArray"
    << "name=" << name << std::endl;
  #endif

  if (this->InputArrays.insert(name).second)
    {
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkSQFTLE::ClearInputArrays()
{
  #ifdef SQTK_DEBUG
  pCerr()
    << "=====vtkSQFTLE::ClearInputArrays" << std::endl;
  #endif

  if (this->InputArrays.size())
    {
    this->InputArrays.clear();
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
int vtkSQFTLE::RequestData(
      vtkInformation *vtkNotUsed(request),
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQFTLE::RequestData" << std::endl;
  #endif

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
    log->StartEvent("vtkSQFTLE::RequestData");
    }

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
  if (nCells>0)
    {
    std::set<std::string>::iterator it;
    std::set<std::string>::iterator begin=this->InputArrays.begin();
    std::set<std::string>::iterator end=this->InputArrays.end();
    for (it=begin; it!=end; ++it)
      {
      vtkDataArray *V=input->GetPointData()->GetArray((*it).c_str());
      if (V==0)
        {
        vtkErrorMacro(
          << "Array " << (*it).c_str()
          << " was requested but is not present");
        continue;
        }

      if (V->GetNumberOfComponents()!=3)
        {
        vtkErrorMacro(
          << "Array " << (*it).c_str() << " is not a vector.");
        continue;
        }

      // Gradient.
      vtkDoubleArray *gradV=vtkDoubleArray::New();
      ComputeVectorGradient(this,0.0,0.4,input,nCells,V,gradV);

      // FTLE
      std::string name;
      name+="ftle-";
      name+=V->GetName();

      vtkDoubleArray *ftleV=vtkDoubleArray::New();
      ftleV->SetName(name.c_str());

      ComputeFTLE(this,0.5,1.0,nCells,gradV,this->TimeInterval,ftleV);

      output->GetCellData()->AddArray(ftleV);

      ftleV->Delete();
      gradV->Delete();
      }
    }

  if (this->LogLevel || globalLogLevel)
    {
    log->EndEvent("vtkSQFTLE::RequestData");
    }

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
