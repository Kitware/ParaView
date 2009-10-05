/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMetaInfoDatabase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMetaInfoDatabase - A place to store scalar ranges for pieces of dataset.
// .SECTION Description
// This class maintains hierarchical records of scalar ranges for different parts of a file.
// As information is obtained, it is inserted into the database. Lower resolution records are
// assumed to be less precise than their higher resolution contents. So when new records are
// added, old records are updated to reflect their contents. 
// When ranges are queried, the most precise information for the piece is returned.
//
// LIMITATIONS:
// 1) A binary relationship is used to determine piece parentage, 
// which may not be true if you do not use the default extent translator.
// 2) I intend to keep geometric metainformation here as well, not just attribute 
// metainformation.

#ifndef __vtkMetaInfoDatabase_h
#define __vtkMetaInfoDatabase_h

#include "vtkObject.h"

class vtkMIDBInternals;

class VTK_EXPORT vtkMetaInfoDatabase : public vtkObject
{
public:
  static vtkMetaInfoDatabase *New();
  vtkTypeRevisionMacro(vtkMetaInfoDatabase,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  //insert a scalar range record for piece p/np at resolution res
  //NOTE:ext is currently ignored
  void Insert(int p, int np, int ext[6], double range[2], double r);

  //request the scalar range record for piece p/np
  //NOTE:ext is currently ignored
  int Search(int p, int np, int ext[6], double *range);
  
protected:
  vtkMetaInfoDatabase();
  ~vtkMetaInfoDatabase();

  vtkMIDBInternals *Internals;

private:
  vtkMetaInfoDatabase(const vtkMetaInfoDatabase&);  // Not implemented.
  void operator=(const vtkMetaInfoDatabase&);  // Not implemented.
};
#endif


