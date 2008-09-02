

#include <iostream>
#include <fstream>

#include "vtkCompositeDataPipeline.h"
#include "vtkMultiProcessController.h"
#include "vtkMPIController.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPolyData.h"
#include "vtkPointData.h"
#include "vtkCTHFragmentConnect.h"
#include "vtkSpyPlotReader.h"
#include "vtkXMLMultiBlockDataWriter.h"
#include "vtkXMLMultiBlockDataReader.h"
#include "vtkPVTestUtilities.h"
#include "vtkstd/vector"
using vtkstd::vector;

using namespace std;

ostream &operator<<(ostream &sout, vector<double> &vd)
{
  sout << "(";
  int n=vd.size();
  if (n>0)
  {
    sout << vd[0];
    for (int i=1; i<n; ++i)
    {
      sout << ", " << vd[i];
    }
  }
  sout << ")";
  return sout;
}

bool operator==(vector<double> &left, vector<double> &right)
{
  int nL=left.size();
  int nR=right.size();
  if (nL!=nR)
  {
    return false;
  }
  for (int i=0; i<nL; ++i)
  {
    double d=fabs(left[i]-right[i]);
    if (d>1.0e-6)
    {
      return false;
    }
  }
  return true;
}


/// Sum points and each point data array in the given data set.
void SumPointDataArrays(vtkPolyData *pd, vector<double> &sum)
{
  int nArrays=pd->GetPointData()->GetNumberOfArrays();
  sum.clear();

  // sum each of the point components
  sum.resize(3,0.0);
  vtkIdType nPoints=pd->GetPoints()->GetNumberOfPoints();
  for (vtkIdType i=0; i<nPoints; ++i)
  {
    double pt[3];
    pd->GetPoints()->GetPoint(i,pt);
    for (int q=0; q<3; ++q)
    {
      sum[q]+=pt[q];
    }
  }
  // for each array sum each of its components
  int baseIdx=3;
  for (int a=0; a<nArrays; ++a)
  {
    vtkDataArray *da=pd->GetPointData()->GetArray(a);
    int nComps=da->GetNumberOfComponents();
    int currentSize=sum.size();
    sum.resize(currentSize+nComps,0.0);
    double tup[32];
    for (vtkIdType i=0; i<nPoints; ++i)
    {
      da->GetTuple(i,tup);
      for (int q=0; q<nComps; ++q)
      {
        sum[baseIdx+q]+=tup[q];
      }
    }
    baseIdx+=nComps;
  }
}




/// Test
int main( int argc, char* argv[] )
{
  // Initialize MPI if available.
  vtkMPIController* controller = vtkMPIController::New(); 
  controller->Initialize(&argc, &argv, 0);
  vtkMultiProcessController::SetGlobalController(controller);
  int myProcId = controller->GetLocalProcessId();
  int nProcs = controller->GetNumberOfProcesses();
  // We have to use a composite pipeline.
  vtkCompositeDataPipeline* prototype = vtkCompositeDataPipeline::New();
  vtkAlgorithm::SetDefaultExecutivePrototype(prototype);
  prototype->Delete();

  // Get various paths.
  vtkPVTestUtilities tu(argc,argv);
  char *baselinePath=tu.GetDataFilePath("Baseline/TestCTHFragmentConnect.vtm");
  char *inputDataPath=tu.GetDataFilePath("Data/spcth_fc_0.0");
  char *tempDataPath=tu.GetTempFilePath("TestCTHFragmentConnect.vtm");

  if (myProcId==0)
  {
    cerr << "Base: " << baselinePath << endl;
    cerr << "Input:" << inputDataPath << endl;
    cerr << "Temp: " << tempDataPath << endl;
  }

  // read input
  vtkSpyPlotReader* spy=vtkSpyPlotReader::New();
  spy->SetFileName(inputDataPath);
  spy->MergeXYZComponentsOn();
  spy->UpdateInformation();
  // query time steps & select last
  int *tsr = spy->GetTimeStepRange();
  int stepNo = tsr[1];
  spy->SetTimeStep(stepNo);
  // read these in
  spy->SetCellArrayStatus("Material volume fraction - 1",1);
  spy->SetCellArrayStatus("Material volume fraction - 2",1);
  spy->SetCellArrayStatus("Mass (g) - 1",1);
  spy->SetCellArrayStatus("Mass (g) - 2",1);
  spy->SetCellArrayStatus("X velocity (cm/s)",1);
  spy->SetCellArrayStatus("Y velocity (cm/s)",1);
  spy->SetCellArrayStatus("Z velocity (cm/s)",1);
  spy->SetCellArrayStatus("Pressure (dynes/cm^2^)",1);
  spy->SetCellArrayStatus("Temperature (eV)",1);
  // Set paramter to connect filter
  vtkCTHFragmentConnect* frag = vtkCTHFragmentConnect::New();
  frag->SelectMaterialArray("Material volume fraction - 1");
  frag->SelectMaterialArray("Material volume fraction - 2");
  frag->SelectMassArray("Mass (g) - 1");
  frag->SelectMassArray("Mass (g) - 2");
  //frag->SetVolumeWtdAvgArrayStatus("velocity (cm/s)", 1);
  //frag->SetMassWtdAvgArrayStatus("velocity (cm/s)", 1);
  frag->SetVolumeWtdAvgArrayStatus("Pressure (dynes/cm^2^)",1);
  frag->SetMassWtdAvgArrayStatus("Pressure (dynes/cm^2^)",1);
  frag->SetVolumeWtdAvgArrayStatus("Temperature (eV)",1);
  frag->SetMassWtdAvgArrayStatus("Temperature (eV)",1);
  frag->SetComputeOBB(1);
  //frag->SetInput(dynamic_cast<vtkDataObject *>(spy));
  frag->SetInputConnection(spy->GetOutputPort());
  spy->Delete();
  frag->GetOutput()->SetUpdatePiece(myProcId);
  frag->GetOutput()->SetUpdateNumberOfPieces(nProcs);
  frag->Update();

  vtkMultiBlockDataSet *statsOut=dynamic_cast<vtkMultiBlockDataSet *>(frag->GetOutput(1));
  statsOut->Update();

  int testStatus=0; // assume good

  // Process 0 checks the results.
  if (myProcId==0)
  {
    // Save the current results in the tetsing temp folder
    vtkXMLMultiBlockDataWriter *mbdsw=vtkXMLMultiBlockDataWriter::New();
    mbdsw->SetInput(statsOut);
    mbdsw->SetDataModeToBinary();
    mbdsw->SetFileName(tempDataPath);
    mbdsw->Write();
    mbdsw->Delete();
    // Load the baseline data to compare current against
    vtkXMLMultiBlockDataReader *mbdsr=vtkXMLMultiBlockDataReader::New();
    if (!mbdsr->CanReadFile(baselinePath))
    {
      // Oops can't read the baseline image
      delete baselinePath;
      delete tempDataPath;
      delete inputDataPath;
      mbdsr->Delete();
      frag->Delete();
      controller->Finalize();
      controller->Delete();
      vtkAlgorithm::SetDefaultExecutivePrototype(0);
      return 1;
    }
    mbdsr->SetFileName(baselinePath);
    vtkMultiBlockDataSet *mbdsBaseline
      = dynamic_cast<vtkMultiBlockDataSet *>(mbdsr->GetOutput());
    mbdsBaseline->Update();

    // Loop over blocks comparing each against the baseline
    int nBlocks=statsOut->GetNumberOfBlocks();
    for (int block=0; block<nBlocks; ++block)
    {
      vtkPolyData *pdCurrent
        = dynamic_cast<vtkPolyData *>(statsOut->GetBlock(block));

      vtkPolyData *pdBaseline
        = dynamic_cast<vtkPolyData *>(mbdsBaseline->GetBlock(block));

      if (tu.ComparePointData(pdBaseline, pdCurrent, 1.0E-6)==false)
      {
        cerr << "Error: Block "
             << block
             << " is not equal to baseline."
             << endl;
        testStatus=1;
      }
    }
    mbdsr->Delete();
  }

  delete baselinePath;
  delete tempDataPath;
  delete inputDataPath;

  frag->Delete();
  controller->Finalize();
  controller->Delete();
  vtkAlgorithm::SetDefaultExecutivePrototype(0);

  return testStatus;
}




