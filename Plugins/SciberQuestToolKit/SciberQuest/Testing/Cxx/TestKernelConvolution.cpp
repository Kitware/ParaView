/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkMultiProcessController.h"
#include "vtkSQLog.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkAppendPolyData.h"
#include "vtkXMLImageDataReader.h"
#include "vtkSQBOVReader.h"
#include "vtkSQImageGhosts.h"
#include "vtkSQKernelConvolution.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkProcessIdScalars.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"
#include "vtkTesting.h"
#include "TestUtils.h"

#include <iostream>
#include <string>

int TestKernelConvolution(int argc, char *argv[])
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
  logFileName=NativePath(tempDir+"/SciberQuestToolKit-TestKernelConvolution.log");
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
  c1->SetComputeResidual(1);
  c1->SetInputConnection(0,ig->GetOutputPort(0));
  ig->Delete();

  // process id
  vtkProcessIdScalars *p1=vtkProcessIdScalars::New();
  p1->SetInputConnection(0,c1->GetOutputPort(0));
  c1->Delete();

  // image to polydata
  vtkDataSetSurfaceFilter *s1=vtkDataSetSurfaceFilter::New();
  s1->SetInputConnection(0,p1->GetOutputPort(0));
  p1->Delete();

  // execute the pipline for each kernel type
  int kernelType[3]={
      vtkSQKernelConvolution::KERNEL_TYPE_GAUSSIAN,
      vtkSQKernelConvolution::KERNEL_TYPE_CONSTANT,
      vtkSQKernelConvolution::KERNEL_TYPE_LOG};

  GetParallelExec(worldRank, worldSize, s1, 0.0);

  int aTestFailed=0;

  for (int i=0; i<3; ++i)
    {
    c1->SetKernelType(kernelType[i]);
    s1->Update();

    int testStatus = SerialRender(
          controller,
          s1->GetOutput(),
          false,
          tempDir,
          baseline,
          "SciberQuestToolKit-TestKernelConvolution",
          700,300,
          0,1,0,
          0,0,0,
          0,0,1,
          2.25);
    if (testStatus==vtkTesting::FAILED)
      {
      aTestFailed=1;
      }
    }

  s1->Delete();

  return Finalize(controller,aTestFailed);
}
