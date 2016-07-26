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

#ifndef vtkSQFieldTopologySelect_h
#define vtkSQFieldTopologySelect_h


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

private:
  vtkSQFieldTopologySelect(const vtkSQFieldTopologySelect&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQFieldTopologySelect&) VTK_DELETE_FUNCTION;
};

#endif
