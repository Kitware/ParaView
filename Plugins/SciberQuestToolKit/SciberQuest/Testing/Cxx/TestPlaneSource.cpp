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
#include "vtkProcessIdScalars.h"
#include "vtkSQPlaneSource.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkTesting.h"
#include "TestUtils.h"

#include <iostream>
#include <string>

int TestPlaneSource(int argc, char *argv[])
{
  vtkMultiProcessController *controller=Initialize(&argc,&argv);
  int worldRank=controller->GetLocalProcessId();
  int worldSize=controller->GetNumberOfProcesses();

  // configure
  std::string dataRoot;
  std::string tempDir;
  std::string baseline;
  BroadcastConfiguration(controller,argc,argv,dataRoot,tempDir,baseline);

  std::string logFileName;
  logFileName=NativePath(tempDir+"/SciberQuestToolKit-TestPlaneSource.log");
  vtkSQLog::GetGlobalInstance()->SetFileName(logFileName.c_str());
  vtkSQLog::GetGlobalInstance()->SetGlobalLevel(1);

  // plane
  const int nResolutions=4;
  int resolution[nResolutions][2]={
        {1,1,},
        {537,1,},
        {1,537,},
        {127,67},
        };

  const int nDecomps=2;
  int decomp[nDecomps]={
        vtkSQPlaneSource::DECOMP_TYPE_STRIPS,
        vtkSQPlaneSource::DECOMP_TYPE_PATCHES
        };

  const char *decompName[nDecomps]={
        "Strips",
        "Patches"};

  int ext[6]={-1,1,-1,1,0,0};

  int aTestFailed=0;
  for (int i=0; i<nResolutions; ++i)
    {
    for (int j=0; j<nDecomps; ++j)
      {
      int *res=resolution[i];

      vtkSQPlaneSource *p=vtkSQPlaneSource::New();
      p->SetOrigin(ext[0],ext[2],ext[4]);
      p->SetPoint1(ext[1],ext[2],ext[4]);
      p->SetPoint2(ext[0],ext[3],ext[4]);
      p->SetXResolution(res[0]);
      p->SetYResolution(res[1]);
      p->SetDecompType(decomp[j]);

      // process id
      vtkProcessIdScalars *pid=vtkProcessIdScalars::New();
      pid->SetInputConnection(0,p->GetOutputPort(0));
      p->Delete();

      // execute
      GetParallelExec(worldRank, worldSize, pid, 0.0);
      pid->Update();

      vtkPolyData *output=dynamic_cast<vtkPolyData*>(pid->GetOutput());

      // rename resolution dependent arrays
      std::ostringstream oss;
      oss << "Decomp-" << decompName[j] << "-" << res[0] << "x" << res[1];
      vtkDataArray *ids=output->GetPointData()->GetArray("ProcessId");
      if (ids)
        {
        ids->SetName(oss.str().c_str());
        }
      oss.str("");
      oss << "TCoords-"  << decompName[j] << "-" << res[0] << "x" << res[1];
      vtkDataArray *tcoord=output->GetPointData()->GetTCoords();
      if (tcoord)
        {
        tcoord->SetName(oss.str().c_str());
        }

      int testStatus = SerialRender(
            controller,
            output,
            false,
            tempDir,
            baseline,
            "SciberQuestToolKit-TestPlaneSource",
            300,300,
            63,63,128,
            0,0,0,
            0,1,0,
            1.25);
      if (testStatus==vtkTesting::FAILED)
        {
        aTestFailed=1;
        }

      pid->Delete();
      }
    }

  return Finalize(controller,aTestFailed);
}
