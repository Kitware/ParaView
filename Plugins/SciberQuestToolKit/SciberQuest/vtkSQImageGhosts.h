/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef vtkSQImageGhosts_h
#define vtkSQImageGhosts_h

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

  void ExecuteTransactions(
      vtkDataSetAttributes *inDsa,
      vtkDataSetAttributes *outDsa,
      CartesianExtent inputExt,
      CartesianExtent outputExt,
      std::vector<GhostTransaction> &transactions,
      bool pointData);

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
  vtkSQImageGhosts(const vtkSQImageGhosts &) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQImageGhosts &) VTK_DELETE_FUNCTION;
};

#endif
