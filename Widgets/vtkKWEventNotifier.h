/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWEventNotifier.h
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


