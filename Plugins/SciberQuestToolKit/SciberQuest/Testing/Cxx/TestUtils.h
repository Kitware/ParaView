/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef TestUtils_h
#define TestUtils_h

class vtkMultiProcessController;
class vtkRenderer;
class vtkDataSet;
class vtkPolyData;
class vtkActor;
class vtkAlgorithm;
class vtkStreamingDemandDrivenPipeline;
#include <string>

/**
Initialize MPI and the pipeline.
*/
vtkMultiProcessController *Initialize(int *argc, char ***argv);

/**
Finalize MPI and the pipleine.
*/
int Finalize(vtkMultiProcessController* controller, int code);

/**
Broadcast the string.
*/
void Broadcast(vtkMultiProcessController *controller, std::string &s, int root=0);

/**
Broadcast the test configuration.
*/
void BroadcastConfiguration(
      vtkMultiProcessController *controller,
      int argc,
      char **argv,
      std::string &dataRoot,
      std::string &tempDir,
      std::string &baseline);

/**
Gather polydata.
*/
vtkPolyData *Gather(
    vtkMultiProcessController *controller,
    int rootRank,
    vtkPolyData *data,
    int tag=100);

/**
Gather to a single rank, render and test.
*/
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
    double threshold=10.0);

/**
Get the pipeline executive for parallel execution.
*/
vtkStreamingDemandDrivenPipeline *GetParallelExec(
        int worldRank,
        int worldSize,
        vtkAlgorithm *a,
        double t);

/**
Set up a mapper/actor for the named array in the
given renderer.
*/
enum {
  POINT_ARRAY=1,
  CELL_ARRAY=0
};
vtkActor *MapArrayToActor(
        vtkRenderer *ren,
        vtkDataSet *data,
        int dataType,
        const char *arrayName);

/**
Convert unix to windows paths on windows.
*/
std::string NativePath(std::string path);

#endif
