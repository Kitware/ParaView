/*=========================================================================

  Program:   ParaView
  Module:    CTHAdaptor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "CTHAdaptor.h"

#include "vtkCPDataDescription.h"
#include "vtkCPProcessor.h"
#include "vtkCPPythonScriptPipeline.h"
#include "vtkCTHSource.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"

#include <cstring>
#include <fstream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

namespace
{
vtkCTHSource gSource;
vtkCPProcessor* coProcessor = nullptr;
vtkCPDataDescription* coProcessorData = nullptr;
}

#if !defined(_WIN32)
#include <execinfo.h>
//------------------------------------------------------------------------------
void handler(int sig)
{
  void* array[100];
  size_t size;
  size = backtrace(array, 100);
  fprintf(stderr, "Error: signal %d\n", sig);
  backtrace_symbols_fd(array, size, 2);
  exit(1);
}
#endif

//------------------------------------------------------------------------------
void pvspy_qa(char* /*qadate*/, char* /*qatime*/, char* /*qajobn*/)
{
  // vtkSMPVSpy::GetSingletonInstance ()->qa (qadate, qatime, qajobn);
}

//------------------------------------------------------------------------------
void pvspy_fil(char* filename, int len, char* /*runid*/, int* /*error*/)
{
  // signal (SIGSEGV, handler);

  // Build all the coprocessing classes
  coProcessor = vtkCPProcessor::New();
  coProcessor->Initialize();

  coProcessorData = vtkCPDataDescription::New();
  coProcessorData->AddInput("input");

  // this must be initialized here because it initializes global controller
  vtkCPPythonScriptPipeline* pipeline = vtkCPPythonScriptPipeline::New();

  vtkMultiProcessController* ctrl;
  // GlobalController is set in vtkCPPythonScriptPipeline::New ()
  ctrl = vtkMultiProcessController::GetGlobalController();
  int localId = ctrl->GetLocalProcessId();

  // determine the script for pipeline on root and then broadcast it
  char line[256];
  // give script some memory, if not root, for later broadcast
  char* script = line;
  if (localId == 0)
  {
    len = 0;
    std::ifstream is;
    is.open(filename, std::ifstream::in);
    while (is.good())
    {
      is.getline(line, 256);
      char* tok = strtok(line, " ");
      if (tok && strstr(tok, "*paraview"))
      {
        script = strtok(nullptr, " ");
        len = static_cast<int>(strlen(script) + 1);
        break;
      }
    }
    is.close();
  }
  ctrl->Broadcast(&len, 1, 0);
  if (len)
  {
    ctrl->Broadcast(script, len, 0);
    // finally set the scripted pipeline for every class
    pipeline->Initialize(script);
    coProcessor->AddPipeline(pipeline);
  }

  pipeline->Delete();
}

//------------------------------------------------------------------------------
int pvspy_vizcheck(int cycle, double ptime)
{
  coProcessorData->SetTimeData(ptime, cycle);
  return coProcessor->RequestDataDescription(coProcessorData);
}

//------------------------------------------------------------------------------
void pvspy_viz(int cycle, double ptime, double /*pdt*/, int, int)
{
  coProcessorData->SetTimeData(ptime, cycle);
  if (coProcessor->RequestDataDescription(coProcessorData))
  {
    gSource.FillInputData(coProcessorData->GetInputDescriptionByName("input"));
    coProcessor->CoProcess(coProcessorData);
  }
}

//------------------------------------------------------------------------------
void pvspy_fin()
{
  coProcessor->Delete();
  coProcessor = nullptr;

  coProcessorData->Delete();
  coProcessorData = nullptr;
}

//------------------------------------------------------------------------------
void pvspy_stm(int igm, int n_blocks, int nmat, int max_mat, int NCFieldNames, int NMFieldNames,
  double* x0, double* x1, int max_level)
{
  gSource.Initialize(igm, n_blocks, nmat, max_mat, NCFieldNames, NMFieldNames, x0, x1, max_level);
}

//------------------------------------------------------------------------------
void pvspy_scf(int field_id, char* field_name, char* comment, int matid)
{
  gSource.SetCellFieldName(field_id, field_name, comment, matid);
}

//------------------------------------------------------------------------------
void pvspy_smf(int field_id, char* field_name, char* comment)
{
  gSource.SetMaterialFieldName(field_id, field_name, comment);
}

//------------------------------------------------------------------------------
void pvspy_scx(int block_id, int field_id, int k, int j, double* istrip)
{
  gSource.SetCellFieldPointer(block_id, field_id, k, j, istrip);
}

//------------------------------------------------------------------------------
void pvspy_smx(int block_id, int field_id, int mat, int k, int j, double* istrip)
{
  gSource.SetMaterialFieldPointer(block_id, field_id, mat, k, j, istrip);
}

//------------------------------------------------------------------------------
void pvspy_stb(int block_id, int Nx, int Ny, int Nz, double* x, double* y, double* z, int allocated,
  int active, int level)
{
  gSource.InitializeBlock(block_id, Nx, Ny, Nz, x, y, z, allocated, active, level);
}

//------------------------------------------------------------------------------
void pvspy_sta(int block_id, int allocated, int active, int level, int max_level, int bxbot,
  int bxtop, int bybot, int bytop, int bzbot, int bztop, int npxma11, int npxma21, int npxma12,
  int npxma22, int npyma11, int npyma21, int npyma12, int npyma22, int npzma11, int npzma21,
  int npzma12, int npzma22, int npxpa11, int npxpa21, int npxpa12, int npxpa22, int npypa11,
  int npypa21, int npypa12, int npypa22, int npzpa11, int npzpa21, int npzpa12, int npzpa22,

  int nbxma11, int nbxma21, int nbxma12, int nbxma22, int nbyma11, int nbyma21, int nbyma12,
  int nbyma22, int nbzma11, int nbzma21, int nbzma12, int nbzma22, int nbxpa11, int nbxpa21,
  int nbxpa12, int nbxpa22, int nbypa11, int nbypa21, int nbypa12, int nbypa22, int nbzpa11,
  int nbzpa21, int nbzpa12, int nbzpa22)
{
  int np[24] = { npxma11, npxma21, npxma12, npxma22, npyma11, npyma21, npyma12, npyma22, npzma11,
    npzma21, npzma12, npzma22, npxpa11, npxpa21, npxpa12, npxpa22, npypa11, npypa21, npypa12,
    npypa22, npzpa11, npzpa21, npzpa12, npzpa22 };

  int nb[24] = { nbxma11, nbxma21, nbxma12, nbxma22, nbyma11, nbyma21, nbyma12, nbyma22, nbzma11,
    nbzma21, nbzma12, nbzma22, nbxpa11, nbxpa21, nbxpa12, nbxpa22, nbypa11, nbypa21, nbypa12,
    nbypa22, nbzpa11, nbzpa21, nbzpa12, nbzpa22 };

  /*
  int np[24] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  int nb[24] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  */

  gSource.UpdateBlock(block_id, allocated, active, level, max_level, bxbot, bxtop, bybot, bytop,
    bzbot, bztop, np, nb);
}

//------------------------------------------------------------------------------
void pvspy_trc(int /*num*/, double* /*xt*/, double* /*yt*/, double* /*zt*/, int* /*id*/,
  int* /*lt*/, int* /*it*/, int* /*jt*/, int* /*kt*/)
{
  // gSource->InitializeTracers (
  // num, xt, yt, zt, id, lt, it, jk, kt);
}
