/*=========================================================================

  Module:    vtkKWSerializer.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWSerializer - a helper class for serialization
// .SECTION Description
// vtkKWSerializer is a helper class that is used by objects to 
// serialize themselves. Put another way, it helps instances write
// or read themselves from disk.

#ifndef __vtkKWSerializer_h
#define __vtkKWSerializer_h

#include "vtkObject.h"

#define VTK_KWSERIALIZER_MAX_TOKEN_LENGTH 8000

class VTK_EXPORT vtkKWSerializer : public vtkObject
{
public:
  static vtkKWSerializer* New();
  vtkTypeRevisionMacro(vtkKWSerializer,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The primary helper functions instances can invoke.
  static void FindClosingBrace(istream *is, vtkObject *obj);
  static void ReadNextToken(istream *is,const char *tok, vtkObject *obj);
  static int GetNextToken(istream *is, char *result);
  static void WriteSafeString(ostream& os, const char *val);
  
  static void EatWhiteSpace(istream *is);

protected:
  vtkKWSerializer() {};
  ~vtkKWSerializer() {};

private:
  vtkKWSerializer(const vtkKWSerializer&); // Not implemented
  void operator=(const vtkKWSerializer&); // Not implemented
};


#endif





