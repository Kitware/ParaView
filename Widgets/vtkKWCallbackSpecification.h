/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWCallbackSpecification.h
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
// .NAME vtkKWCallbackSpecification - helper class for vtkKWEventNotifier
// .SECTION Description

#ifndef __vtkKWCallbackSpecification_h
#define __vtkKWCallbackSpecification_h

#include "vtkKWObject.h"
#include "vtkKWWindow.h"

class VTK_EXPORT vtkKWCallbackSpecification : public vtkKWObject
{
 public:
  static vtkKWCallbackSpecification* New();
  vtkTypeMacro(vtkKWCallbackSpecification, vtkKWObject);

  vtkSetStringMacro( EventString );
  vtkGetStringMacro( EventString );

  vtkSetStringMacro( CommandString );
  vtkGetStringMacro( CommandString );

  void SetCalledObject( vtkKWObject *object ) {this->CalledObject = object;};
  vtkGetObjectMacro( CalledObject, vtkKWObject );

  void SetWindow( vtkKWWindow *window ) {this->Window = window;};
  vtkGetObjectMacro( Window, vtkKWWindow );

  vtkSetObjectMacro( NextCallback, vtkKWCallbackSpecification );
  vtkGetObjectMacro( NextCallback, vtkKWCallbackSpecification );
  
 protected:
  vtkKWCallbackSpecification();
  ~vtkKWCallbackSpecification();
  vtkKWCallbackSpecification(const vtkKWCallbackSpecification&) {};
  void operator=(const vtkKWCallbackSpecification&) {};

  char                       *EventString;
  char                       *CommandString;
  vtkKWObject                *CalledObject;
  vtkKWWindow                *Window;
  vtkKWCallbackSpecification *NextCallback;
};

#endif


