/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQLog.h"
#include "vtkMultiProcessController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkXMLImageDataReader.h"
#include "vtkSQBOVReader.h"
#include "vtkSQImageGhosts.h"
#include "vtkSQKernelConvolution.h"
#include "vtkSQVortexFilter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkProcessIdScalars.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"
#include "vtkTesting.h"
#include "TestUtils.h"

#include <iostream>
#include <string>

int TestVortexFilter(int argc, char *argv[])
{
  vtkMultiProcessController *controller=Initialize(&argc,&argv);
  int worldRank=controller->GetLocalProcessId();
  int worldSize=controller->GetNumberOfProcesses();

  // configure
  std::string dataRoot;
  std::string tempDir;
  std::string baseline;
  BroadcastConfiguration(controller,argc,argv,dataRoot,tempDir,baseline);

  std::string inputFileName;
  if (worldSize==1)
    {
    inputFileName=NativePath(dataRoot+"/SciberQuestToolKit/Asym2D/Asym2D.vti");
    }
  else
    {
    inputFileName=NativePath(dataRoot+"/SciberQuestToolKit/Asym2D/Asym2D.bov");
    }

  std::string logFileName;
  logFileName=NativePath(tempDir+"/SciberQuestToolKit-TestVortexFilter.log");
  vtkSQLog::GetGlobalInstance()->SetFileName(logFileName.c_str());
  vtkSQLog::GetGlobalInstance()->SetGlobalLevel(1);

  // build pipeline
  const int kernelWidth=19;

  // reader
  vtkAlgorithm *r;
  if (worldSize==1)
    {
    vtkXMLImageDataReader *r1=vtkXMLImageDataReader::New();
    r1->SetFileName(inputFileName.c_str());
    r1->SetPointArrayStatus("ue",1);
    r=r1;
    }
  else
    {
    vtkSQBOVReader *r1=vtkSQBOVReader::New();
    r1->SetFileName(inputFileName.c_str());
    r1->SetPointArrayStatus("ue",1);
    r=r1;
    }

  // ghost cell
  vtkSQImageGhosts *ig=vtkSQImageGhosts::New();
  ig->SetInputConnection(0,r->GetOutputPort(0));
  r->Delete();

  // convolution
  vtkSQKernelConvolution *c1=vtkSQKernelConvolution::New();
  c1->SetKernelWidth(kernelWidth);
  c1->AddInputArray("ue");
  c1->AddArrayToCopy("ue");
  c1->SetKernelType(vtkSQKernelConvolution::KERNEL_TYPE_GAUSSIAN);
  c1->SetInputConnection(0,ig->GetOutputPort(0));
  ig->Delete();

  // ghost cell
  ig=vtkSQImageGhosts::New();
  ig->SetInputConnection(0,c1->GetOutputPort(0));
  c1->Delete();

  // vortex
  vtkSQVortexFilter *v1=vtkSQVortexFilter::New();
  v1->AddInputArray("ue-gauss-19");
  v1->AddArrayToCopy("ue");
  v1->SetSplitComponents(1);
  v1->SetResultMagnitude(1);
  v1->SetComputeRotation(1);
  v1->SetComputeHelicity(1);
  v1->SetComputeNormalizedHelicity(1);
  v1->SetComputeQ(1);
  v1->SetComputeLambda2(1);
  v1->SetComputeGradient(1);
  v1->SetComputeDivergence(1);
  v1->SetComputeEigenvalueDiagnostic(1);
  v1->SetInputConnection(0,ig->GetOutputPort(0));
  ig->Delete();

  // process id
  vtkProcessIdScalars *p1=vtkProcessIdScalars::New();
  p1->SetInputConnection(0,v1->GetOutputPort(0));
  v1->Delete();

  // image to polydata
  vtkDataSetSurfaceFilter *s1=vtkDataSetSurfaceFilter::New();
  s1->SetInputConnection(0,p1->GetOutputPort(0));
  p1->Delete();

  // execute
  GetParallelExec(worldRank, worldSize, s1, 0.0);
  s1->Update();

  int testStatus = SerialRender(
        controller,
        s1->GetOutput(),
        false,
        tempDir,
        baseline,
        "SciberQuestToolKit-TestVortexFilter",
        700,300,
        0,1,0,
        0,0,0,
        0,0,1,
        2.25);

  s1->Delete();

  return Finalize(controller,testStatus==vtkTesting::PASSED?0:1);
}
