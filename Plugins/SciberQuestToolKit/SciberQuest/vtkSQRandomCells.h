/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
// .NAME vtkSQRandomCells - Select cells at random
// .SECTION Description
// Pass a specified number of randomly selected cells from input
// to ouput.

#ifndef __vtkSQRandomCells_h
#define __vtkSQRandomCells_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkDataSetAlgorithm.h"

class VTKSCIBERQUEST_EXPORT vtkSQRandomCells : public vtkDataSetAlgorithm
{
public:
  static vtkSQRandomCells *New();
  vtkTypeMacro(vtkSQRandomCells,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of cells to select.
  vtkSetMacro(SampleSize,int);
  vtkGetMacro(SampleSize,int);

  // Description:
  // Set the reandom number generator seed. The default is -1
  // which causes the system clock to be used.
  vtkSetMacro(Seed,int);
  vtkGetMacro(Seed,int);

protected:
  /// Pipeline internals.
  //int FillInputPortInformation(int port,vtkInformation *info);
  int RequestData(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);
  int RequestInformation(vtkInformation *req, vtkInformationVector **input, vtkInformationVector *output);

  vtkSQRandomCells();
  ~vtkSQRandomCells();

private:
  int SampleSize;
  int Seed;

private:
  vtkSQRandomCells(const vtkSQRandomCells&);  // Not implemented.
  void operator=(const vtkSQRandomCells&);  // Not implemented.
};

#endif
