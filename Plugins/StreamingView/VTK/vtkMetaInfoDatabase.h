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
// .NAME vtkMetaInfoDatabase - A place to store array ranges for pieces of dataset.
// .SECTION Description
// This class maintains hierarchical records of array ranges for different parts
// of a file. As information is obtained, it is inserted into the database. Lower
// resolution records are assumed to be less precise than their higher resolution
// contents. So when new records are added, old records are updated to reflect
// their contents. When ranges are queried, the most precise information for the
// piece is returned.
//
// LIMITATIONS:
// 1) A binary relationship is used to determine piece parentage,
// which may not be true if you do not use the default extent translator.

#ifndef __vtkMetaInfoDatabase_h
#define __vtkMetaInfoDatabase_h

#include "vtkObject.h"

class VTK_EXPORT vtkMetaInfoDatabase : public vtkObject
{
public:
  static vtkMetaInfoDatabase *New();
  vtkTypeMacro(vtkMetaInfoDatabase,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //insert a range for an array within piece p/np at resolution res
  //will update any ancestor pieces
  //NOTE:ext is currently ignored
  void Insert(int p, int np, int ext[6], double resolution,
              int field_association, const char *ArrayName, int component,
              double range[2]);

  //Description:
  //request the range record for piece p/np
  //NOTE:ext is currently ignored
  int Search(int p, int np, int ext[6],
             int field_association, const char *ArrayName, int component,
             double *range);

protected:
  vtkMetaInfoDatabase();
  ~vtkMetaInfoDatabase();

  class ArrayRecords;
  ArrayRecords *Records;

private:
  vtkMetaInfoDatabase(const vtkMetaInfoDatabase&);  // Not implemented.
  void operator=(const vtkMetaInfoDatabase&);  // Not implemented.
};
#endif
