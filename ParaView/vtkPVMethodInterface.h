/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVMethodInterface.h
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

#ifndef __vtkPVMethodInterface_h
#define __vtkPVMethodInterface_h

#include "vtkObject.h"

#define VTK_STRING 13

class VTK_EXPORT vtkPVMethodInterface : public vtkObject
{
public:
  static vtkPVMethodInterface* New();
  vtkTypeMacro(vtkPVMethodInterface, vtkObject);
  
  vtkSetStringMacro(VariableName);
  vtkGetStringMacro(VariableName);
  
  vtkSetMacro(ArgumentType, int);
  vtkGetMacro(ArgumentType, int);
  
  vtkSetMacro(NumberOfArguments, int);
  vtkGetMacro(NumberOfArguments, int);
  
protected:
  vtkPVMethodInterface();
  ~vtkPVMethodInterface();
  vtkPVMethodInterface(const vtkPVMethodInterface&) {};
  void operator=(const vtkPVMethodInterface&) {};

  char *VariableName;
  int ArgumentType;
  int NumberOfArguments;
};

#endif
