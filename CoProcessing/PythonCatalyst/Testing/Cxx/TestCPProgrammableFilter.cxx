#include <iostream>
#include <mpi.h>

#include <vtkCPProcessor.h>
#include <vtkCPPythonScriptPipeline.h>
#include <vtkNew.h>

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);
  vtkCPProcessor* processor = vtkCPProcessor::New();
  processor->Initialize();

  for(int i=1;i<argc;i++)
    {
    vtkNew<vtkCPPythonScriptPipeline> pipeline;
    pipeline->Initialize(argv[i]);
    processor->AddPipeline(pipeline.GetPointer());
    }

  processor->Finalize();
  processor->Delete();

  MPI_Finalize();
  return 0;
}
