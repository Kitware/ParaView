/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
//=========================================================================
// .NAME vtkSQFieldTopologySplit - split a dataset along topologic boundaries
// .SECTION Description
//
//  Split a dataset produced by the field topology mapper along
//  topologic boundaries. This is specialized for the magneospheric
//  mapping case and there is one ouput per topological class.
//
//  Topological classes
//
//----------------------------------------
//  class       value     definition
//--------------------------------------
//  solar wind  0         d-d
//              3         0-d
//              4         i-d
//--------------------------------------
//  magnetos-   5         n-n
//  phere       6         s-n
//              9         s-s
//--------------------------------------
//  north       1         n-d
//  connected   7         0-n
//              8         i-n
//--------------------------------------
//  south       2         s-d
//  connected   10        0-s
//              11        i-s
//-------------------------------------
//  null/short  12        0-0
//  integration 13        i-0
//              14        i-i
//---------------------------------------
// .SECTION Caveats


#ifndef __vtkSQFieldTopologySplit_h
#define __vtkSQFieldTopologySplit_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkDataSetAlgorithm.h"

class VTKSCIBERQUEST_EXPORT vtkSQFieldTopologySplit : public vtkDataSetAlgorithm
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkSQFieldTopologySplit,vtkDataSetAlgorithm);
  static vtkSQFieldTopologySplit *New();

protected:
  vtkSQFieldTopologySplit();
  virtual ~vtkSQFieldTopologySplit();

  int FillInputPortInformation(int /*port*/,vtkInformation *info);
  int FillOutputPortInformation(int /*port*/,vtkInformation *info);
  int RequestInformation(vtkInformation *,vtkInformationVector **,vtkInformationVector *outInfos);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:


private:
  vtkSQFieldTopologySplit(const vtkSQFieldTopologySplit&);  // Not implemented.
  void operator=(const vtkSQFieldTopologySplit&);  // Not implemented.
};

#endif
