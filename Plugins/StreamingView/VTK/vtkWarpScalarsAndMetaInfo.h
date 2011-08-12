/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpScalarsAndMetaInfo.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkWarpScalarsAndMetaInfo - like parent class but works better
// with view prioritization
// .SECTION Description
// Beyond what the parent class does, this class also warps the piece bounds
// in the pipeline so that warping data doesn't necessarily preclude view
// prioritization. If warping by point associated normals, the meta-info
// is reset.

#ifndef __vtkWarpScalarsAndMetaInfo_h
#define __vtkWarpScalarsAndMetaInfo_h

#include "vtkWarpScalar.h"

class VTK_EXPORT vtkWarpScalarsAndMetaInfo : public vtkWarpScalar
{
public:
  static vtkWarpScalarsAndMetaInfo *New();
  vtkTypeMacro(vtkWarpScalarsAndMetaInfo,vtkWarpScalar);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkWarpScalarsAndMetaInfo();
  ~vtkWarpScalarsAndMetaInfo();

  //Description:
  //Overridden to inject meta-information manipulation into pipeline
  int ProcessRequest(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkWarpScalarsAndMetaInfo(const vtkWarpScalarsAndMetaInfo&);  // Not implemented.
  void operator=(const vtkWarpScalarsAndMetaInfo&);  // Not implemented.
};

#endif
