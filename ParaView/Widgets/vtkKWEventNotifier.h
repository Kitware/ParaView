/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWEventNotifier.h
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
// .NAME vtkKWEventNotifier - collect callbacks and invoke them when certain events occur
// .SECTION Description

#ifndef __vtkKWEventNotifier_h
#define __vtkKWEventNotifier_h

#include "vtkKWObject.h"
#include "vtkKWCallbackSpecification.h"


class VTK_EXPORT vtkKWEventNotifier : public vtkKWObject
{
public:
  static vtkKWEventNotifier* New();
  vtkTypeMacro(vtkKWEventNotifier,vtkKWObject);

  // Description:
  // Add a callback for a specified event occurring in a specified
  // window. The command of the object will be called.
  void AddCallback( const char *event,   vtkKWWindow *window,
		    vtkKWObject *object, const char *command );

//BTX  
  // Description:
  // Add a callback for a specified event occurring in a specified
  // window. The command is a C++ method to be called (no tcl interpreter used)
  // It must take a string as an argument, and know how to parse it.
  void AddCallback( const char *event,   vtkKWWindow *window,
		    void (*callMethod)(const char *, void *), 
		    void *arg,
		    void (*delMethod)(void *) );

  // Description:
  // Add a callback for a specified event occurring in a specified
  // window. The command is a C++ method to be called (no tcl interpreter used)
  // It must take a string as an argument, and know how to parse it.
  void AddCallback( const char *event, void (*callMethod)(const char *, void *), 
		    void *arg, void (*delMethod)(void *) )
    {this->AddCallback( event, (vtkKWWindow *)NULL, 
			callMethod, arg, delMethod );};

//ETX
  
  // Description:
  // Remove a specific callback
  void RemoveCallback( const char *event,   vtkKWWindow *window,
		       vtkKWObject *object, const char *command );

//BTX  
  // Description:
  // Remove a specific callback - C++ method version
  void RemoveCallback( const char *event,   vtkKWWindow *window,
		       vtkKWObject *object, void (*f)(const char *, void *) );
//ETX
  
  // Description:
  // Remove all callbacks associated with this object
  void RemoveCallbacks( vtkKWObject *object );

  // Description:
  // This version invokes all callbacks of the specified type for
  // the specified window - even its own callback if it has one
  void InvokeCallbacks( const char *event, vtkKWWindow *window,
			const char *args );

  // Description:
  // This version won't invoke callbacks on the specified object 
  // Usually the calling object uses this to avoid calling itself
  void InvokeCallbacks( vtkKWObject *object, const char *event, 
                        vtkKWWindow *window, const char *args );

protected:
  vtkKWEventNotifier();
  ~vtkKWEventNotifier();
  vtkKWEventNotifier(const vtkKWEventNotifier&) {};
  void operator=(const vtkKWEventNotifier&) {};

  vtkKWCallbackSpecification **Callbacks;

  int ComputeIndex( const char *event );
};


#endif


