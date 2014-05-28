/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "TestUtils.h"
#include "vtksys/SystemInformation.hxx"
#include "vtkInitializationHelper.h"
#include "vtkSQLog.h"
#include "vtkMultiProcessController.h"
#if defined(PARAVIEW_USE_MPI) && !defined(WIN32)
#include "vtkMPIController.h"
#else
#include "vtkDummyController.h"
#endif
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkAlgorithm.h"
#include "vtkDataSetAttributes.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyData.h"
#include "vtkAppendPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkTesting.h"
#include "vtkWindowToImageFilter.h"
#include "vtkOutlineFilter.h"
#include "vtkPNGWriter.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkColorTransferFunction.h"
#include <cfloat>
#include <iostream>
#include <string>
#include <vector>

// when not set process id arrays are rendered
// and written to the temp dir.
#define SKIP_PROCESS_ID 1

/**
Use a single render window for each run. This was suggested
after strange x11 errors on the Blight dashboard system.
*/
class vtkRenderWindowSingleton
{
public:
  /// get or create the instance.
  static vtkRenderWindow *GetGlobalInstance()
  {
    if (vtkRenderWindowSingleton::RenderWindow==NULL)
      {
      vtkRenderWindowSingleton::RenderWindow=vtkRenderWindow::New();
      }
  return vtkRenderWindowSingleton::RenderWindow;
  }

  /// delete the instance
  static void DeleteGlobalInstance()
  {
    if (vtkRenderWindowSingleton::RenderWindow)
      {
      vtkRenderWindowSingleton::RenderWindow->Delete();
      vtkRenderWindowSingleton::RenderWindow=NULL;
      }
  }

private:
  static vtkRenderWindow *RenderWindow;
};

vtkRenderWindow *vtkRenderWindowSingleton::RenderWindow=NULL;

//*****************************************************************************
vtkMultiProcessController *Initialize(int *argc, char ***argv)
{
  vtkInitializationHelper::StandaloneInitialize();

  vtksys::SystemInformation::SetStackTraceOnError(1);

  vtkMultiProcessController *controller;

  #if defined(PARAVIEW_USE_MPI) && !defined(WIN32)
  controller=vtkMPIController::New();
  controller->Initialize(argc,argv,0);
  #else
  controller=vtkDummyController::New();
  #endif

  vtkMultiProcessController::SetGlobalController(controller);

  vtkCompositeDataPipeline* cexec=vtkCompositeDataPipeline::New();
  vtkAlgorithm::SetDefaultExecutivePrototype(cexec);
  cexec->Delete();

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent(0,"TotalRunTime");

  return controller;
}

//*****************************************************************************
int Finalize(vtkMultiProcessController* controller, int code)
{
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->EndEvent(0,"TotalRunTime");

  if (code==0)
    {
    log->Update();
    log->Write();
    }

  vtkSQLog::DeleteGlobalInstance();

  vtkRenderWindowSingleton::DeleteGlobalInstance();

  vtkMultiProcessController::SetGlobalController(0);
  vtkAlgorithm::SetDefaultExecutivePrototype(0);
  controller->Finalize();
  controller->Delete();

  vtkInitializationHelper::StandaloneFinalize();

  return code;
}

// ****************************************************************************
void Broadcast(vtkMultiProcessController *controller, std::string &s, int root)
{
  int worldRank=controller->GetLocalProcessId();

  std::vector<char> buffer;

  if (worldRank==root)
    {
    buffer.assign(s.begin(),s.end());

    vtkIdType bufferSize=buffer.size();
    controller->Broadcast(&bufferSize,1,0);
    controller->Broadcast(&buffer[0],bufferSize,0);
    }
  else
    {
    vtkIdType bufferSize=0;
    controller->Broadcast(&bufferSize,1,0);

    buffer.resize(bufferSize,'\0');
    controller->Broadcast(&buffer[0],bufferSize,0);

    s.assign(buffer.begin(),buffer.end());
    }
}

// ****************************************************************************
void BroadcastConfiguration(
      vtkMultiProcessController *controller,
      int argc,
      char **argv,
      std::string &dataRoot,
      std::string &tempDir,
      std::string &baseline)
{
  int worldRank=controller->GetLocalProcessId();
  if (worldRank==0)
    {
    vtkTesting *testHelper = vtkTesting::New();
    testHelper->AddArguments(argc,const_cast<const char **>(argv));

    dataRoot=testHelper->GetDataRoot();
    baseline=testHelper->GetValidImageFileName();
    tempDir=testHelper->GetTempDirectory();

    testHelper->Delete();
    }
  Broadcast(controller,dataRoot);
  Broadcast(controller,tempDir);
  Broadcast(controller,baseline);
}

// ****************************************************************************
vtkPolyData *Gather(
    vtkMultiProcessController *controller,
    int rootRank,
    vtkPolyData *data,
    int tag)
{
  int worldRank=controller->GetLocalProcessId();
  int worldSize=controller->GetNumberOfProcesses();

  if (worldRank!=rootRank)
    {
    // send
    controller->Send(data,rootRank,tag);
    }
  else
    {
    // gather
    vtkAppendPolyData *apd=vtkAppendPolyData::New();
    apd->AddInputData(data);

    for (int i=0; i<worldSize; ++i)
      {
      if (i==rootRank)
        {
        continue;
        }
      vtkPolyData* pd = vtkPolyData::New();
      controller->Receive(pd,vtkMultiProcessController::ANY_SOURCE,tag);
      apd->AddInputData(pd);
      pd->Delete();
      }

    apd->Update();
    data=apd->GetOutput();
    data->Register(0);
    apd->Delete();

    return data;
    }

  return NULL;
}

// ****************************************************************************
vtkStreamingDemandDrivenPipeline *GetParallelExec(
        int worldRank,
        int worldSize,
        vtkAlgorithm *a,
        double t)
{
  vtkStreamingDemandDrivenPipeline* exec
     = dynamic_cast<vtkStreamingDemandDrivenPipeline*>(a->GetExecutive());

  vtkInformation *info  = exec->GetOutputInformation(0);

  exec->SetUpdateNumberOfPieces(info,worldSize);
  exec->SetUpdatePiece(info,worldRank);
  exec->UpdateInformation();
  exec->SetUpdateExtent(info,worldRank,worldSize,0);
  exec->SetUpdateTimeStep(0,t);
  exec->PropagateUpdateExtent(0);

  return exec;
}

// ****************************************************************************
vtkDoubleArray *ComputeMagnitude(vtkDataArray *inDa)
{
  vtkDoubleArray *outDa=vtkDoubleArray::New();

  if (inDa==NULL)
    {
    std::cerr << "Null pointer to input array." << std::endl;
    return outDa;
    }

  vtkIdType nTups=inDa->GetNumberOfTuples();
  outDa->SetNumberOfTuples(nTups);

  std::string name=inDa->GetName();
  name+="-mag";
  outDa->SetName(name.c_str());

  int nComps=inDa->GetNumberOfComponents();

  switch(inDa->GetDataType())
    {
    vtkTemplateMacro(

      VTK_TT *val = (VTK_TT*)inDa->GetVoidPointer(0);
      double *mag = (double*)outDa->GetVoidPointer(0);

      double m;
      double v;

      for (vtkIdType i=0; i<nTups; ++i)
        {
        vtkIdType idx=nComps*i;

        m=0.0;
        for (int j=0; j<nComps; ++j)
          {
          v = (double)val[idx+j];
          m = m + v*v;
          }
        m = sqrt(m);
        mag[i] = m;
        }
      );
    }

  return outDa;
}

// ****************************************************************************
void ComputeRange(vtkDataArray *da, double *range)
{
  if (da==NULL)
    {
    std::cerr << "Null pointer to input array." << std::endl;
    return;
    }

  int nComps=da->GetNumberOfComponents();
  if (nComps==1)
    {
    da->GetRange(range);
    }
  else
    {
    range[0]=DBL_MAX;
    range[1]=-DBL_MAX;

    switch(da->GetDataType())
      {
      vtkTemplateMacro(
        VTK_TT *t=(VTK_TT*)da->GetVoidPointer(0);
        vtkIdType nTups=da->GetNumberOfTuples();
        for (vtkIdType i=0; i<nTups; ++i)
          {
          vtkIdType idx=nComps*i;
          VTK_TT m=VTK_TT(0);
          for (int j=0; j<nComps; ++j)
            {
            m+=t[idx+j]*t[idx+j];
            }
          m=sqrt((double)m);
          if (m<range[0])
            {
            range[0]=m;
            }
          if (m>range[1])
            {
            range[1]=m;
            }
          }
        );
      }
    }
  std::cerr << da->GetName() << "=[" << range[0] << ", " << range[1] << "]" << std::endl;
}

// ****************************************************************************
void PrintArrayNames(vtkDataSetAttributes *dsa)
{
  int n=dsa->GetNumberOfArrays();
  std::cerr << n << " " << dsa->GetClassName() << " arrays found." << std::endl;
  for (int i=0; i<n; ++i)
    {
    std::cerr << dsa->GetArray(i)->GetName() << std::endl;
    }
}

// ****************************************************************************
void SetLUTToCoolToWarmDiverging(vtkPolyDataMapper *pdm, vtkDataArray *da)
{
  double range[2] = {0.0, 0.0};

  da->GetRange(range);

  vtkColorTransferFunction *lut=vtkColorTransferFunction::New();
  lut->SetColorSpaceToRGB();
  lut->AddRGBPoint(range[0],0.0,0.0,1.0);
  lut->AddRGBPoint(range[1],1.0,0.0,0.0);
  lut->SetColorSpaceToDiverging();
  lut->Build();

  pdm->SetLookupTable(lut);

  lut->Delete();
}

// ****************************************************************************
vtkActor *MapArrayToActor(
        vtkRenderer *ren,
        vtkDataSet *data,
        int arrayType,
        const char *arrayName)
{
  vtkPolyData *pd=dynamic_cast<vtkPolyData*>(data);
  if (pd==NULL)
    {
    std::cerr << "unsuported dataset type " << data->GetClassName() << std::endl;
    return 0;
    }

  vtkPolyDataMapper *pdm=vtkPolyDataMapper::New();
  pdm->SetInputData(pd);

  vtkDataArray *da=NULL;
  vtkDataSetAttributes *atts=NULL;

  if (arrayName)
    {
    if (arrayType==POINT_ARRAY)
      {
      atts=pd->GetPointData();
      pdm->SetScalarModeToUsePointData();
      }
    else
    if (arrayType==CELL_ARRAY)
      {
      atts=pd->GetCellData();
      pdm->SetScalarModeToUseCellData();
      }

    da=atts->GetArray(arrayName);
    if (da)
      {
      if (da->GetNumberOfComponents()>1)
        {
        vtkDataArray *mag=ComputeMagnitude(da);
        pd->GetPointData()->AddArray(mag);
        mag->Delete();
        da=mag;
        }
      atts->SetActiveScalars(da->GetName());

      SetLUTToCoolToWarmDiverging(pdm,da);
      }
    }
  else
   {
   pdm->ScalarVisibilityOff();
   }

  pdm->Update();

  vtkActor *act=vtkActor::New();
  act->SetMapper(pdm);
  pdm->Delete();

  ren->AddActor(act);
  act->Delete();

  if (arrayName && da==NULL)
    {
    std::cerr << "Error you requested a non-existant array " << arrayName << std::endl;
    PrintArrayNames(pd->GetPointData());
    PrintArrayNames(pd->GetCellData());
    }

  return act;
}

// ****************************************************************************
std::string NativePath(std::string path)
{
  std::string nativePath;
  #ifdef WIN32
  size_t n=path.size();
  for (size_t i=0; i<n; ++i)
    {
    if (path[i]=='/')
      {
      nativePath+='\\';
      continue;
      }
    nativePath+=path[i];
    }
  return nativePath;
  #else
  return path;
  #endif

  return "";
}

// ****************************************************************************
int SerialRender(
    vtkMultiProcessController *controller,
    vtkPolyData *data,
    bool showBounds,
    std::string &tempDir,
    std::string &baseline,
    std::string testName,
    int iwx,
    int iwy,
    double px,
    double py,
    double pz,
    double fx,
    double fy,
    double fz,
    double vux,
    double vuy,
    double vuz,
    double cz,
    double threshold)
{
  int renderRank=0;
  int aTestFailed=0;

  // gather
  data=Gather(controller,renderRank,data,100);

  int worldRank=controller->GetLocalProcessId();
  int worldSize=controller->GetNumberOfProcesses();

  // only the render rank needs to continue.
  if (worldRank==renderRank)
    {
    vtkPolyData *outline=NULL;
    if (showBounds)
      {
      vtkOutlineFilter *of=vtkOutlineFilter::New();
      of->SetInputData(data);
      of->Update();
      outline=of->GetOutput();
      outline->Register(0);
      of->Delete();
      }

    // render point then cell data
    vtkDataSetAttributes *dsa[2]={
        data->GetPointData(),
        data->GetCellData()
        };

    for (int j=0; j<2; ++j)
      {
      const int arrayType=dsa[j]->IsA("vtkPointData");

      int nArrays=dsa[j]->GetNumberOfArrays();
      for (int i=0; i<nArrays; ++i)
        {
        std::string arrayName=dsa[j]->GetArray(i)->GetName();

        if (arrayName == "ProcessId")
          {
          if ((worldSize<2) || SKIP_PROCESS_ID)
            {
            continue;
            }

          // render the decomp and save it
          vtkRenderer *ren=vtkRenderer::New();

          MapArrayToActor(ren,data,arrayType,"ProcessId");

          if (showBounds)
            {
            MapArrayToActor(ren,outline,arrayType,0);
            }

          vtkRenderWindow *rwin=vtkRenderWindowSingleton::GetGlobalInstance();
          rwin->AddRenderer(ren);
          rwin->SetSize(iwx,iwy);
          //ren->Delete(); hold the ref to work around a bug in vtk

          vtkCamera *cam=ren->GetActiveCamera();
          cam->SetPosition(px,py,pz);
          cam->SetFocalPoint(fx,fy,fz);
          cam->ComputeViewPlaneNormal();
          cam->SetViewUp(vux,vuy,vuz);
          cam->OrthogonalizeViewUp();
          ren->ResetCamera();
          cam->Zoom(cz);

          rwin->MakeCurrent();
          rwin->Render();
          vtkWindowToImageFilter *decompImage = vtkWindowToImageFilter::New();
          decompImage->SetInput(rwin);

          std::string tempDecomp;
          tempDecomp+=tempDir;
          tempDecomp+="/";
          tempDecomp+=testName;
          tempDecomp+="-Decomp.png";
          tempDecomp=NativePath(tempDecomp);

          vtkPNGWriter *decompWriter = vtkPNGWriter::New();
          decompWriter->SetFileName(tempDecomp.c_str());
          decompWriter->SetInputConnection(decompImage->GetOutputPort());
          decompWriter->Write();
          decompWriter->Delete();
          decompImage->Delete();

          rwin->RemoveRenderer(ren);
          ren->Delete(); // ok to release

          continue;
          }

        vtkRenderer *ren=vtkRenderer::New();

        MapArrayToActor(ren,data,arrayType,arrayName.c_str());

        if (showBounds)
          {
          MapArrayToActor(ren,outline,arrayType,0);
          }

        vtkRenderWindow *rwin=vtkRenderWindowSingleton::GetGlobalInstance();
        rwin->AddRenderer(ren);
        rwin->SetSize(iwx,iwy);
        //ren->Delete(); hold the ref to work around a bug in vtk

        vtkCamera *cam=ren->GetActiveCamera();
        cam->SetPosition(px,py,pz);
        cam->SetFocalPoint(fx,fy,fz);
        cam->ComputeViewPlaneNormal();
        cam->SetViewUp(vux,vuy,vuz);
        cam->OrthogonalizeViewUp();
        ren->ResetCamera();
        cam->Zoom(cz);

        std::string base;
        base+=baseline;
        base+="/";
        base+=testName;
        base+="-";
        base+=arrayName;
        base+=".png";

        vtkTesting *testHelper = vtkTesting::New();
        testHelper->AddArgument("-T");
        testHelper->AddArgument(tempDir.c_str());
        testHelper->AddArgument("-V");
        testHelper->AddArgument(base.c_str());
        testHelper->SetRenderWindow(rwin);
        int result=testHelper->RegressionTest(threshold);
        if (result!=vtkTesting::PASSED)
          {
          aTestFailed=1;
          std::cerr << "Test for array " << arrayName << " failed." << std::endl;
          }
        testHelper->Delete();
        rwin->RemoveRenderer(ren);
        ren->Delete(); // ok to release
        }
      }
    data->Delete();

    if (showBounds)
      {
      outline->Delete();
      }
    }

  // sync up here, if not when this function
  // is called in a loop, the gather's may cross
  controller->Barrier();

  return aTestFailed?vtkTesting::FAILED:vtkTesting::PASSED;
}
