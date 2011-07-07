/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#ifndef __vtkSQImageGhosts_h
#define __vtkSQImageGhosts_h

#include "vtkDataSetAlgorithm.h"
#include "CartesianExtent.h"
#include "GhostTransaction.h"

#include <vector>
using std::vector;

#include <mpi.h>

class vtkInformation;
class vtkInformationVector;
class vtkDataSetAttributes;

class vtkSQImageGhosts : public vtkDataSetAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkSQImageGhosts,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQImageGhosts *New();

  // Description:
  // Set the mode to 2D or 3D.
  vtkSetMacro(Mode,int);
  vtkGetMacro(Mode,int);

  // Description:
  // Set the number of ghost cells.
  vtkSetMacro(NGhosts,int);
  vtkGetMacro(NGhosts,int);

  // Description:
  // Set the communicator.
  void SetCommunicator(MPI_Comm comm);
  MPI_Comm GetCommunicator(){ return this->Comm; }

protected:
  //int FillInputPortInformation(int port, vtkInformation *info);
  //int FillOutputPortInformation(int port, vtkInformation *info);
  //int RequestDataObject(vtkInformation*,vtkInformationVector** inInfoVec,vtkInformationVector* outInfoVec);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestUpdateExtent(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  vtkSQImageGhosts();
  virtual ~vtkSQImageGhosts();

private:
  // CartesianExtent Grow(const CartesianExtent &inputExt);

  void ExecuteTransactions(
      vtkDataSetAttributes *inDsa,
      vtkDataSetAttributes *outDsa,
      CartesianExtent inputExt,
      CartesianExtent outputExt,
      vector<GhostTransaction> &transactions,
      bool pointData);

private:
  int WorldSize;
  int WorldRank;
  int NGhosts;
  int Mode;
  CartesianExtent ProblemDomain;
  MPI_Comm Comm;

private:
  vtkSQImageGhosts(const vtkSQImageGhosts &); // Not implemented
  void operator=(const vtkSQImageGhosts &); // Not implemented
};

#endif

