/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWCallbackSpecification.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

  // Don't use a set macro since we don't want to reference count this
  void SetCalledObject( vtkKWObject *object ) {this->CalledObject = object;};
  vtkGetObjectMacro( CalledObject, vtkKWObject );

  // Don't use a set macro since we don't want to reference count this
  void SetWindow( vtkKWWindow *window ) {this->Window = window;};
  vtkGetObjectMacro( Window, vtkKWWindow );

  // We do want to reference count this one
  vtkSetObjectMacro( NextCallback, vtkKWCallbackSpecification );
  vtkGetObjectMacro( NextCallback, vtkKWCallbackSpecification );
  
  // This is the C++ method to be called instead of going through tcl interpreter.
//BTX
  void SetCommandMethod(void (*f)(const char *,void *)) {this->CommandMethod = f;};
  void (*CommandMethod)(const char *, void *);

  void SetArgDeleteMethod(void (*f)(void *)) {this->ArgDeleteMethod = f;};
  void (*ArgDeleteMethod)(void *);
  
  void SetArg( void *arg ) { this->Arg = arg;};
  void *GetArg() {return this->Arg;};  
//ETX

protected:
  vtkKWCallbackSpecification();
  ~vtkKWCallbackSpecification();
  vtkKWCallbackSpecification(const vtkKWCallbackSpecification&) {};
  void operator=(const vtkKWCallbackSpecification&) {};

  char                       *EventString;
  char                       *CommandString;
  vtkKWObject                *CalledObject;
  vtkKWWindow                *Window;
  void                       *Arg;    
  vtkKWCallbackSpecification *NextCallback;
};

#endif


