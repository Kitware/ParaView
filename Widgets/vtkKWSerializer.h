/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWSerializer.h
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
// .NAME vtkKWSerializer
// .SECTION Description
// vtkKWSerializer is a helper class that is used by objects to 
// serialize themselves. Put another way, it helps instances write
// or read themselves from disk.

#ifndef __vtkKWSerializer_h
#define __vtkKWSerializer_h

#include "vtkKWApplication.h"

// var args
#ifndef _WIN32
#include <unistd.h>
#endif
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "tcl.h"

class VTK_EXPORT vtkKWSerializer : public vtkObject
{
public:
  static vtkKWSerializer* New();
  const char *GetClassName() {return "vtkKWSerializer";};

  // Description:
  // The primary helper functions instances can invoke.
  static void FindClosingBrace(istream *is, vtkObject *obj);
  static void ReadNextToken(istream *is,const char *tok, vtkObject *obj);
  static int GetNextToken(istream *is, char result[1024]);
  static void WriteSafeString(ostream& os, const char *val);
  
protected:
  static void EatWhiteSpace(istream *is);
};


#endif


