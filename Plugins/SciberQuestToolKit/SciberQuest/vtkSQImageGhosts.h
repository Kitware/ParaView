/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.

*/
#ifndef __vtkSQImageGhosts_h
#define __vtkSQImageGhosts_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkDataSetAlgorithm.h"
#include "CartesianExtent.h" // for CartesianExtent
#include "GhostTransaction.h" // for GhostTransaction

#include <vector> // for vector
#include <set> // for set
#include <string> // for string

#ifdef SQTK_WITHOUT_MPI
typedef void * MPI_Comm;
#else
#include "SQMPICHWarningSupression.h" // for suppressing MPI warnings
#include <mpi.h> // for MPI_Comm
#endif

class vtkInformation;
class vtkInformationVector;
class vtkDataSetAttributes;
class vtkPVXMLElement;

class VTKSCIBERQUEST_EXPORT vtkSQImageGhosts : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkSQImageGhosts,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkSQImageGhosts *New();

  // Description:
  // Initialize from an xml document.
  int Initialize(vtkPVXMLElement *root);

  // Description:
  // If explicitly unset then the list of arrays constructed by
  // AddArrayToCopy methods is used. The default is to copy all
  // arrays. See AddArrayToCopy
  vtkSetMacro(CopyAllArrays,int);
  vtkGetMacro(CopyAllArrays,int);

  // Description:
  // Deep copy a list of arrays from input to the output. In order
  // for the filter to use the constructed list the default of copying
  // all arrays must be explicitly disabled by calling SetAllArrays(0).
  void AddArrayToCopy(const char *name);
  void ClearArraysToCopy();

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

  // Description:
  // Set the log level.
  // 0 -- no logging
  // 1 -- basic logging
  // .
  // n -- advanced logging
  vtkSetMacro(LogLevel,int);
  vtkGetMacro(LogLevel,int);

protected:
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestUpdateExtent(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  vtkSQImageGhosts();
  virtual ~vtkSQImageGhosts();

private:
  // CartesianExtent Grow(const CartesianExtent &inputExt);

  //BTX
  void ExecuteTransactions(
      vtkDataSetAttributes *inDsa,
      vtkDataSetAttributes *outDsa,
      CartesianExtent inputExt,
      CartesianExtent outputExt,
      std::vector<GhostTransaction> &transactions,
      bool pointData);
  //ETX

private:
  int WorldSize;
  int WorldRank;
  int NGhosts;
  int Mode;
  CartesianExtent ProblemDomain;
  MPI_Comm Comm;
  int CopyAllArrays;
  std::set<std::string> ArraysToCopy;
  int LogLevel;

private:
  vtkSQImageGhosts(const vtkSQImageGhosts &); // Not implemented
  void operator=(const vtkSQImageGhosts &); // Not implemented
};

#endif
