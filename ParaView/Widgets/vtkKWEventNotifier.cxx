/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWEventNotifier.cxx
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
#include "vtkKWEventNotifier.h"
#include "vtkObjectFactory.h"
#include <ctype.h>


vtkStandardNewMacro( vtkKWEventNotifier );


int vtkKWEventNotifierCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

vtkKWEventNotifier::vtkKWEventNotifier()
{
  this->Callbacks = new vtkKWCallbackSpecification *[26];
  for ( int i=0; i < 26; i++ )
    {
    this->Callbacks[i] = NULL;
    }
  this->CommandFunction = vtkKWEventNotifierCommand;
}

vtkKWEventNotifier::~vtkKWEventNotifier()
{
  if ( this->Callbacks )
    {
    for ( int i = 0; i < 26; i++ )
      {
      if ( this->Callbacks[i] )
	{
	this->Callbacks[i]->UnRegister(this);
	this->Callbacks[i] = NULL;
	}
      }
    delete [] this->Callbacks;
    }
}

void vtkKWEventNotifier::AddCallback( const char *event, 
				      vtkKWWindow *window,
				      vtkKWObject *object, 
				      const char *command )
{
  int                         index;
  vtkKWCallbackSpecification  *tmp1, *tmp2;

  // Find out where to put it
  index  = this->ComputeIndex(event);

  // Create a callback specification object to store it in
  tmp1   = (vtkKWCallbackSpecification *)vtkKWCallbackSpecification::New();
  tmp1->SetEventString  ( event   );
  tmp1->SetCalledObject ( object  );
  tmp1->SetWindow( window );
  tmp1->SetCommandString( command );

  // If we already have another callback in this bucket
  if ( this->Callbacks[index] )
    {
    // Find the end of the list and put this callback there
    tmp2 = this->Callbacks[index];
    while (tmp2->GetNextCallback())
      {
      tmp2 = tmp2->GetNextCallback();
      }    
    tmp2->SetNextCallback(tmp1);
    }
  // Otherwise, this is the first callback. 
  else
    {
    this->Callbacks[index] = tmp1;
    // We must register it because we are going to delete it below
    this->Callbacks[index]->Register( this );
    }

  tmp1->Delete();
}


void vtkKWEventNotifier::AddCallback( const char *event, 
				      vtkKWWindow *window,
				      void (*command)(const char *, void *),
				      void *arg,
				      void (*delCommand)(void *) )
{
  int                         index;
  vtkKWCallbackSpecification  *tmp1, *tmp2;

  // Find out where to put it
  index  = this->ComputeIndex(event);

  // Create a callback specification object to store it in
  tmp1   = (vtkKWCallbackSpecification *)vtkKWCallbackSpecification::New();
  tmp1->SetEventString  ( event   );
  tmp1->SetWindow( window );
  tmp1->SetCommandMethod( command );
  tmp1->SetArg( arg );
  tmp1->SetArgDeleteMethod( delCommand );

  // If we already have another callback in this bucket
  if ( this->Callbacks[index] )
    {
    // Find the end of the list and put this callback there
    tmp2 = this->Callbacks[index];
    while (tmp2->GetNextCallback())
      {
      tmp2 = tmp2->GetNextCallback();
      }    
    tmp2->SetNextCallback(tmp1);
    }
  // Otherwise, this is the first callback. 
  else
    {
    this->Callbacks[index] = tmp1;
    // We must register it because we are going to delete it below
    this->Callbacks[index]->Register( this );
    }

  tmp1->Delete();
}

void vtkKWEventNotifier::RemoveCallbacks( vtkKWObject *object ) 
{
  int                         index, done = 0;
  vtkKWCallbackSpecification  *tmp1, *tmp2, *tmp3;


  // Keep going until we don't find any more
  while ( !done )
    {
    done = 1;
    // check in all lists
    for ( index = 0; index < 26; index++ )
      {
      // Are there any callbacks in the right list? If not, this must not
      // be a callback that we have - just ignore it. If so, then find it in
      // that list, and remove it.
      if ( this->Callbacks[index] )
	{
	// Is it the first callback? Treat this case specially
	if ( this->Callbacks[index]->GetCalledObject() == object )
	  {
	  done = 0;
	  // Get the next callback
	  tmp1 = this->Callbacks[index]->GetNextCallback();

	  // Is the next one non-null? If so, we need to register it so
	  // that it is not deleted when unregister is called on 
	  // this->Callbacks[index].
	  if ( tmp1 )
	    {
	    tmp1->Register( this );
	    this->Callbacks[index]->UnRegister(this);
	    this->Callbacks[index] = tmp1;
	    }
	  // If the next one is null, then this whole list becomes null after
	  // we delete its only entry
	  else
	    {
	    this->Callbacks[index]->UnRegister(this);
	    this->Callbacks[index] = NULL;
	    }
	  }
	// Otherwise, it is not the first callback and we have to keep
	// looking for it.
	else
	  {
	  tmp1 = this->Callbacks[index];
	  tmp2 = this->Callbacks[index]->GetNextCallback();
	  while ( tmp2 && tmp2->GetCalledObject() != object )
	    {
	    tmp1 = tmp2;
	    tmp2 = tmp2->GetNextCallback();
	    }

	  // If tmp2 is null, then we can't find the callback to remove,
	  // so just silently ignore it. If tmp2 is non null, then this
	  // is the callback we have to remove, and tmp1 is the one right
	  // before it in the list.
	  if ( tmp2 )
	    {
	    done = 0;
	    // If there is another callback after the one we are deleting,
	    // reconnect the list
	    if ( tmp2->GetNextCallback() )
	      {
	      // Hang on to it so it isn't deleted when the event before it
	      // is unregistered. Then when we set it as the next event it
	      // is registered so we can unregister it again.
	      tmp3 = tmp2->GetNextCallback();
	      tmp3->Register(this);
	      tmp1->SetNextCallback( tmp2->GetNextCallback() );
	      tmp3->UnRegister(this);
	      }
	    // Otherwise we are removing the last callback in the list.
	    else
	      {
	      tmp1->SetNextCallback(NULL);
	      }
	    }
	  }
	}
      }
    }
}

void vtkKWEventNotifier::RemoveCallback( const char *event, 
					 vtkKWWindow *window,
					 vtkKWObject *object, 
					 const char *command )
{
  int                         index;
  vtkKWCallbackSpecification  *tmp1, *tmp2, *tmp3;

  index  = this->ComputeIndex(event);

  // Are there any callbacks in the right list? If not, this must not
  // be a callback that we have - just ignore it. If so, then find it in
  // that list, and remove it.
  if ( this->Callbacks[index] )
    {
      // Is it the first callback? Treat this case specially
      if ( !strcmp( this->Callbacks[index]->GetEventString(), event ) &&
	   !strcmp( this->Callbacks[index]->GetCommandString(), command ) &&
	   this->Callbacks[index]->GetWindow() == window &&
	   this->Callbacks[index]->GetCalledObject() == object )
	{
	// Get the next callback
	tmp1 = this->Callbacks[index]->GetNextCallback();

	// Is the next one non-null? If so, we need to register it so
	// that it is not deleted when unregister is called on 
	// this->Callbacks[index].
	if ( tmp1 )
	  {
	  tmp1->Register( this );
	  this->Callbacks[index]->UnRegister(this);
	  this->Callbacks[index] = tmp1;
	  }
	// If the next one is null, then this whole list becomes null after
	// we delete its only entry
	else
	  {
	  this->Callbacks[index]->UnRegister(this);
	  this->Callbacks[index] = NULL;
	  }
	}
      // Otherwise, it is not the first callback and we have to keep
      // looking for it.
      else
	{
	tmp1 = this->Callbacks[index];
	tmp2 = this->Callbacks[index]->GetNextCallback();
	while ( tmp2 &&
		( strcmp( tmp2->GetEventString(), event ) ||
		  strcmp( tmp2->GetCommandString(), command ) ||
		  tmp2->GetWindow() != window ||
		  tmp2->GetCalledObject() != object ) )
	  {
	  tmp1 = tmp2;
	  tmp2 = tmp2->GetNextCallback();
	  }

	// If tmp2 is null, then we can't find the callback to remove,
	// so just silently ignore it. If tmp2 is non null, then this
	// is the callback we have to remove, and tmp1 is the one right
	// before it in the list.
	if ( tmp2 )
	  {
	  // If there is another callback after the one we are deleting,
	  // reconnect the list
	  if ( tmp2->GetNextCallback() )
	    {
	    // Hang on to it so it isn't deleted when the event before it
	    // is unregistered. Then when we set it as the next event it
	    // is registered so we can unregister it again.
	    tmp3 = tmp2->GetNextCallback();
	    tmp3->Register(this);
	    tmp1->SetNextCallback( tmp2->GetNextCallback() );
	    tmp3->UnRegister(this);
	    }
	  // Otherwise we are removing the last callback in the list.
	  else
	    {
	    tmp1->SetNextCallback(NULL);
	    }
	  }
	}
    }
}


void vtkKWEventNotifier::RemoveCallback( const char *event, 
					 vtkKWWindow *window,
					 vtkKWObject *object, 
					 void (*command)(const char *, void *) )
{
  int                         index;
  vtkKWCallbackSpecification  *tmp1, *tmp2, *tmp3;

  index  = this->ComputeIndex(event);

  // Are there any callbacks in the right list? If not, this must not
  // be a callback that we have - just ignore it. If so, then find it in
  // that list, and remove it.
  if ( this->Callbacks[index] )
    {
      // Is it the first callback? Treat this case specially
      if ( !strcmp( this->Callbacks[index]->GetEventString(), event ) &&
	   this->Callbacks[index]->CommandMethod ==  command &&
	   this->Callbacks[index]->GetWindow() == window &&
	   this->Callbacks[index]->GetCalledObject() == object )
	{
	// Get the next callback
	tmp1 = this->Callbacks[index]->GetNextCallback();

	// Is the next one non-null? If so, we need to register it so
	// that it is not deleted when unregister is called on 
	// this->Callbacks[index].
	if ( tmp1 )
	  {
	  tmp1->Register( this );
	  this->Callbacks[index]->UnRegister(this);
	  this->Callbacks[index] = tmp1;
	  }
	// If the next one is null, then this whole list becomes null after
	// we delete its only entry
	else
	  {
	  this->Callbacks[index]->UnRegister(this);
	  this->Callbacks[index] = NULL;
	  }
	}
      // Otherwise, it is not the first callback and we have to keep
      // looking for it.
      else
	{
	tmp1 = this->Callbacks[index];
	tmp2 = this->Callbacks[index]->GetNextCallback();
	while ( tmp2 &&
		( strcmp( tmp2->GetEventString(), event ) ||
		  tmp2->CommandMethod != command ||
		  tmp2->GetWindow() != window ||
		  tmp2->GetCalledObject() != object ) )
	  {
	  tmp1 = tmp2;
	  tmp2 = tmp2->GetNextCallback();
	  }

	// If tmp2 is null, then we can't find the callback to remove,
	// so just silently ignore it. If tmp2 is non null, then this
	// is the callback we have to remove, and tmp1 is the one right
	// before it in the list.
	if ( tmp2 )
	  {
	  // If there is another callback after the one we are deleting,
	  // reconnect the list
	  if ( tmp2->GetNextCallback() )
	    {
	    // Hang on to it so it isn't deleted when the event before it
	    // is unregistered. Then when we set it as the next event it
	    // is registered so we can unregister it again.
	    tmp3 = tmp2->GetNextCallback();
	    tmp3->Register(this);
	    tmp1->SetNextCallback( tmp2->GetNextCallback() );
	    tmp3->UnRegister(this);
	    }
	  // Otherwise we are removing the last callback in the list.
	  else
	    {
	    tmp1->SetNextCallback(NULL);
	    }
	  }
	}
    }
}


void vtkKWEventNotifier::InvokeCallbacks( const char *event,
					  vtkKWWindow *window,
					  const char *args )
{
  this->InvokeCallbacks( NULL, event, window, args );
}

void vtkKWEventNotifier::InvokeCallbacks( vtkKWObject *object,
                                          const char *event,
					  vtkKWWindow *window,
					  const char *args )
{
  int                         index;
  vtkKWCallbackSpecification  *tmp;

  // Find out where to look for callbacks matching the event
  index  = this->ComputeIndex(event);

  // If there are some callbacks in this bucket, invoke any that match
  if ( this->Callbacks[index] )
    {
    tmp = this->Callbacks[index];
    while (tmp)
      {
      // If we have a match, invoke it. Either a window is NULL, or the
      // window arguments must match as well as the event string. The object
      // argument must not match - this is used to exclude the calling
      // object from invoking its own callbacks.
      if ( !strcmp( tmp->GetEventString(), event ) &&
	   ( window == NULL || tmp->GetWindow() == NULL ||
	     tmp->GetWindow() == window ) &&
           ( object == NULL || tmp->GetCalledObject() != object ) )
	{
        if ( tmp->GetCommandString() )
          {
          this->Script("eval %s %s %s", tmp->GetCalledObject()->GetTclName(),
                       tmp->GetCommandString(), args );
          }
        else if ( tmp->CommandMethod )
          {
          tmp->CommandMethod(args, tmp->GetArg() );
          }
        
	}
      tmp = tmp->GetNextCallback();
      }    
    }
}

int vtkKWEventNotifier::ComputeIndex( const char *event )
{
  if ( !isalpha(event[0]) )
    {
    return 25;
    }
  else if ( isupper(event[0]) )
    {
    return (int)(event[0] - 'A');
    }
  else
    {
    return (int)(event[0] - 'a');
    }
}

