/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStringList.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
// .NAME vtkKWAssignment - An object for assigning data to processors.
// .SECTION Description
// A vtkStringList holds a data piece/extent specification for a process.
// It is really just a parallel object wrapper for vtkPVExtentTranslator.

#ifndef __vtkStringList_h
#define __vtkStringList_h

#include "vtkObject.h"


class VTK_EXPORT vtkStringList : public vtkObject
{
public:
  static vtkStringList* New();
  vtkTypeMacro(vtkStringList,vtkObject);
  
//BTX
  // Description:
  // Add a command and format it any way you like/
  void AddString(const char *EventString, ...);
//ETX
  
  // Description:
  // Random access.
  void SetString(int idx, const char *str);

  // Description:
  // Get the length of the list.
  int GetLength() { return this->NumberOfStrings;}
  
  // Description:
  // Get a command from its index.
  char *GetString(int idx);
  
  
protected:
  vtkStringList();
  ~vtkStringList();
  vtkStringList(const vtkStringList&) {};
  void operator=(const vtkStringList&) {};
  
  int NumberOfStrings;
  int StringArrayLength;
  char **Strings;
  void Reallocate(int num);
  void DeleteStrings();

};

#endif
