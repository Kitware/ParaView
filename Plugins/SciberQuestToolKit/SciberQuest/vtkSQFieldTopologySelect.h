/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
//=========================================================================
// .NAME vtkSQFieldTopologySelect - select a subset of topological classes
// .SECTION Description
//
//  Select subset of the topological classes produced by the field
//  topology mapper.This is specialized for the magneospheric mapping
//  case.
//
//  Topological classes
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
//
// .SECTION Caveats

#ifndef __vtkSQFieldTopologySelect_h
#define __vtkSQFieldTopologySelect_h


#include "vtkSciberQuestModule.h" // for export macro
#include "vtkDataSetAlgorithm.h"

class VTKSCIBERQUEST_EXPORT vtkSQFieldTopologySelect : public vtkDataSetAlgorithm
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkSQFieldTopologySelect,vtkDataSetAlgorithm);
  static vtkSQFieldTopologySelect *New();

  // Description:
  // Enable/Disable topological class inclusion. Selected classes
  // are appended into the output.
  void SetSelectDD(int v);
  void SetSelectND(int v);
  void SetSelectSD(int v);
  void SetSelectOD(int v);
  void SetSelectID(int v);
  void SetSelectNN(int v);
  void SetSelectSN(int v);
  void SetSelectON(int v);
  void SetSelectIN(int v);
  void SetSelectSS(int v);
  void SetSelectOS(int v);
  void SetSelectIS(int v);
  void SetSelectOO(int v);
  void SetSelectIO(int v);
  void SetSelectII(int v);

  int GetSelectDD();
  int GetSelectND();
  int GetSelectSD();
  int GetSelectOD();
  int GetSelectID();
  int GetSelectNN();
  int GetSelectSN();
  int GetSelectON();
  int GetSelectIN();
  int GetSelectSS();
  int GetSelectOS();
  int GetSelectIS();
  int GetSelectOO();
  int GetSelectIO();
  int GetSelectII();

protected:
  vtkSQFieldTopologySelect();
  virtual ~vtkSQFieldTopologySelect();

  int FillInputPortInformation(int /*port*/,vtkInformation *info);
  int FillOutputPortInformation(int /*port*/,vtkInformation *info);
  int RequestInformation(vtkInformation *,vtkInformationVector **,vtkInformationVector *outInfos);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  // topological classes
  int ClassSelection[15];
  //BTX
  enum {
    DD=0,
    ND=1,
    SD=2,
    OD=3,
    ID=4,
    NN=5,
    SN=6,
    ON=7,
    IN=8,
    SS=9,
    OS=10,
    IS=11,
    OO=12,
    IO=13,
    II=14
    };
  //ETX

private:
  vtkSQFieldTopologySelect(const vtkSQFieldTopologySelect&);  // Not implemented.
  void operator=(const vtkSQFieldTopologySelect&);  // Not implemented.
};

#endif
