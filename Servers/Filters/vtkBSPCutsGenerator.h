/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBSPCutsGenerator
// .SECTION Description
//

#ifndef __vtkBSPCutsGenerator_h
#define __vtkBSPCutsGenerator_h

#include "vtkDataObjectAlgorithm.h"
class vtkPKdTree;

class VTK_EXPORT vtkBSPCutsGenerator : public vtkDataObjectAlgorithm
{
public:
  static vtkBSPCutsGenerator* New();
  vtkTypeMacro(vtkBSPCutsGenerator, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Enable/Disable generation of the cuts.
  vtkSetMacro(Enabled, bool);
  vtkGetMacro(Enabled, bool);

  // This is only valid after Update().
  vtkGetObjectMacro(PKdTree, vtkPKdTree);

//BTX
protected:
  vtkBSPCutsGenerator();
  ~vtkBSPCutsGenerator();
  
  int FillInputPortInformation(int port, vtkInformation *info);
  int RequestDataObject(vtkInformation *,
    vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *,
    vtkInformationVector **, vtkInformationVector *);

  void SetPKdTree(vtkPKdTree*);
  vtkPKdTree* PKdTree;
  bool Enabled;
private:
  vtkBSPCutsGenerator(const vtkBSPCutsGenerator&); // Not implemented
  void operator=(const vtkBSPCutsGenerator&); // Not implemented
//ETX
};

#endif

