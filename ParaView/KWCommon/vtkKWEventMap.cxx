/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWEventMap.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWEventMap.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkKWEventMap, "1.1");
vtkStandardNewMacro(vtkKWEventMap);

vtkKWEventMap::vtkKWEventMap()
{
  this->NumberOfMouseEvents = 0;
  this->NumberOfKeyEvents = 0;
  this->NumberOfKeySymEvents = 0;
  
  this->MouseEvents = NULL;
  this->KeyEvents = NULL;
  this->KeySymEvents = NULL;
}

vtkKWEventMap::~vtkKWEventMap()
{
  int i;
  
  for (i = 0; i < this->NumberOfMouseEvents; i++)
    {
    delete [] this->MouseEvents[i].Action;
    }
  if (this->MouseEvents)
    {
    delete [] this->MouseEvents;
    }

  for (i = 0; i < this->NumberOfKeyEvents; i++)
    {
    delete [] this->KeyEvents[i].Action;
    }
  if (this->KeyEvents)
    {
    delete [] this->KeyEvents;
    }
  
  for (i = 0; i < this->NumberOfKeySymEvents; i++)
    {
    delete [] this->KeySymEvents[i].Action;
    }
  if (this->KeySymEvents)
    {
    delete [] this->MouseEvents;
    }
}

void vtkKWEventMap::AddMouseEvent(int button, int modifier, char *action)
{
  if ( ! action)
    {
    vtkErrorMacro("Can't add NULL action");
    return;
    }
  if (this->FindMouseAction(button, modifier))
    {
    vtkErrorMacro("Action already exists for this button and modifier.\n"
                  << "Try SetMouseEvent to change this binding.");
    return;
    }
  
  int i;
  MouseEvent *events = new MouseEvent[this->NumberOfMouseEvents];
  
  for (i = 0; i < this->NumberOfMouseEvents; i++)
    {
    events[i].Button = this->MouseEvents[i].Button;
    events[i].Modifier = this->MouseEvents[i].Modifier;
    events[i].Action = new char[strlen(this->MouseEvents[i].Action) + 1];
    strcpy(events[i].Action, this->MouseEvents[i].Action);
    delete [] this->MouseEvents[i].Action;
    }
  if (this->MouseEvents)
    {
    delete [] this->MouseEvents;
    this->MouseEvents = NULL;
    }
  
  this->MouseEvents = new MouseEvent[this->NumberOfMouseEvents + 1];
  
  for (i = 0; i < this->NumberOfMouseEvents; i++)
    {
    this->MouseEvents[i].Button = events[i].Button;
    this->MouseEvents[i].Modifier = events[i].Modifier;
    this->MouseEvents[i].Action = new char[strlen(events[i].Action) + 1];
    strcpy(this->MouseEvents[i].Action, events[i].Action);
    delete [] events[i].Action;
    }
  delete [] events;
  
  this->MouseEvents[i].Button = button;
  this->MouseEvents[i].Modifier = modifier;
  this->MouseEvents[i].Action = new char[strlen(action) + 1];
  strcpy(this->MouseEvents[i].Action, action);
  
  this->NumberOfMouseEvents++;
}

void vtkKWEventMap::AddKeyEvent(char key, int modifier, char *action)
{
  if ( ! action)
    {
    vtkErrorMacro("Can't add NULL action");
    return;
    }
  if (this->FindKeyAction(key, modifier))
    {
    vtkErrorMacro("Action already exists for this key and modifier.\n"
                  << "Try SetKeyEvent to change this binding.");
    return;
    }
  
  int i;
  KeyEvent *events = new KeyEvent[this->NumberOfKeyEvents];
  
  for (i = 0; i < this->NumberOfKeyEvents; i++)
    {
    events[i].Key = this->KeyEvents[i].Key;
    events[i].Modifier = this->KeyEvents[i].Modifier;
    events[i].Action = new char[strlen(this->KeyEvents[i].Action) + 1];
    strcpy(events[i].Action, this->KeyEvents[i].Action);
    delete [] this->KeyEvents[i].Action;
    }
  if (this->KeyEvents)
    {
    delete [] this->KeyEvents;
    this->KeyEvents = NULL;
    }
  
  this->KeyEvents = new KeyEvent[this->NumberOfKeyEvents + 1];
  
  for (i = 0; i < this->NumberOfKeyEvents; i++)
    {
    this->KeyEvents[i].Key = events[i].Key;
    this->KeyEvents[i].Modifier = events[i].Modifier;
    this->KeyEvents[i].Action = new char[strlen(events[i].Action) + 1];
    strcpy(this->KeyEvents[i].Action, events[i].Action);
    delete [] events[i].Action;
    }
  delete [] events;
  
  this->KeyEvents[i].Key = key;
  this->KeyEvents[i].Modifier = modifier;
  this->KeyEvents[i].Action = new char[strlen(action) + 1];
  strcpy(this->KeyEvents[i].Action, action);
  
  this->NumberOfKeyEvents++;
}

void vtkKWEventMap::AddKeySymEvent(char *keySym, char *action)
{
  if ( ! keySym)
    {
    vtkErrorMacro("Can't add event for NULL keySym");
    return;
    }
  if ( ! action)
    {
    vtkErrorMacro("Can't add NULL action");
    return;
    }
  if (this->FindKeySymAction(keySym))
    {
    vtkErrorMacro("Action already exists for this keySym.\n"
                  << "Try SetKeySymEvent to change this binding.");
    return;
    }  
  
  int i;
  KeySymEvent *events = new KeySymEvent[this->NumberOfKeySymEvents];
  
  for (i = 0; i < this->NumberOfKeySymEvents; i++)
    {
    events[i].KeySym = new char[strlen(this->KeySymEvents[i].KeySym) + 1];
    strcpy(events[i].KeySym, this->KeySymEvents[i].KeySym);
    delete [] this->KeySymEvents[i].KeySym;
    events[i].Action = new char[strlen(this->KeySymEvents[i].Action) + 1];
    strcpy(events[i].Action, this->KeySymEvents[i].Action);
    delete [] this->KeySymEvents[i].Action;
    }
  if (this->KeySymEvents)
    {
    delete [] this->KeySymEvents;
    this->KeySymEvents = NULL;
    }
  
  this->KeySymEvents = new KeySymEvent[this->NumberOfKeySymEvents + 1];
  
  for (i = 0; i < this->NumberOfKeySymEvents; i++)
    {
    this->KeySymEvents[i].KeySym = new char[strlen(events[i].KeySym) + 1];
    strcpy(this->KeySymEvents[i].KeySym, events[i].KeySym);
    delete [] events[i].KeySym;
    this->KeySymEvents[i].Action = new char[strlen(events[i].Action) + 1];
    strcpy(this->KeySymEvents[i].Action, events[i].Action);
    delete [] events[i].Action;
    }
  delete [] events;

  this->KeySymEvents[i].KeySym = new char[strlen(keySym) + 1];
  strcpy(this->KeySymEvents[i].KeySym, keySym);
  this->KeySymEvents[i].Action = new char[strlen(action) + 1];
  strcpy(this->KeySymEvents[i].Action, action);
  
  this->NumberOfKeySymEvents++;
}

void vtkKWEventMap::SetMouseEvent(int button, int modifier, char *action)
{
  int i;

  if ( ! action)
    {
    vtkErrorMacro("Can't set NULL action");
    return;
    }
  
  for (i = 0; i < this->NumberOfMouseEvents; i++)
    {
    if (this->MouseEvents[i].Button == button &&
        this->MouseEvents[i].Modifier == modifier)
      {
      delete [] this->MouseEvents[i].Action;
      this->MouseEvents[i].Action = new char[strlen(action)+1];
      strcpy(this->MouseEvents[i].Action, action);
      break;
      }
    }
}

void vtkKWEventMap::SetKeyEvent(char key, int modifier, char *action)
{
  int i;

  if ( ! action)
    {
    vtkErrorMacro("Can't set NULL action");
    return;
    }
  
  for (i = 0; i < this->NumberOfKeyEvents; i++)
    {
    if (this->KeyEvents[i].Key == key &&
        this->KeyEvents[i].Modifier == modifier)
      {
      delete [] this->KeyEvents[i].Action;
      this->KeyEvents[i].Action = new char[strlen(action)+1];
      strcpy(this->KeyEvents[i].Action, action);
      break;
      }
    }
}

void vtkKWEventMap::SetKeySymEvent(char *keySym, char *action)
{
  int i;

  if ( ! action)
    {
    vtkErrorMacro("Can't set NULL action");
    return;
    }
  
  for (i = 0; i < this->NumberOfKeySymEvents; i++)
    {
    if ( ! strcmp(this->KeySymEvents[i].KeySym, keySym))
      {
      delete [] this->KeySymEvents[i].Action;
      this->KeySymEvents[i].Action = new char[strlen(action)+1];
      strcpy(this->KeySymEvents[i].Action, action);
      break;
      }
    }
}

void vtkKWEventMap::RemoveMouseEvent(int button, int modifier, char *action)
{
  if ( ! action)
    {
    // It isn't possible to add a NULL action, so there's no reason to
    // try to remove it.
    return;
    }
  if (strcmp(this->FindMouseAction(button, modifier), action))
    {
    // If this event doesn't exist, we can't remove it.
    return;
    }
  
  int i;
  MouseEvent *events = new MouseEvent[this->NumberOfMouseEvents];
  
  for (i = 0; i < this->NumberOfMouseEvents; i++)
    {
    events[i].Button = this->MouseEvents[i].Button;
    events[i].Modifier = this->MouseEvents[i].Modifier;
    events[i].Action = new char[strlen(this->MouseEvents[i].Action) + 1];
    strcpy(events[i].Action, this->MouseEvents[i].Action);
    delete [] this->MouseEvents[i].Action;
    }
  if (this->MouseEvents)
    {
    delete [] this->MouseEvents;
    this->MouseEvents = NULL;
    }
  
  int count = 0;
  this->MouseEvents = new MouseEvent[this->NumberOfMouseEvents-1];
  
  for (i = 0; i < this->NumberOfMouseEvents; i++)
    {
    if (events[i].Button != button ||
        events[i].Modifier != modifier ||
        strcmp(events[i].Action, action))
      {
      this->MouseEvents[count].Button = events[i].Button;
      this->MouseEvents[count].Modifier = events[i].Modifier;
      this->MouseEvents[count].Action = new char[strlen(events[i].Action)+1];
      strcpy(this->MouseEvents[count].Action, events[i].Action);
      delete [] events[i].Action;
      count++;
      }
    }
  this->NumberOfMouseEvents--;
}

void vtkKWEventMap::RemoveKeyEvent(char key, int modifier, char *action)
{
  if ( ! action)
    {
    // It isn't possible to add a NULL action, so there's no reason to
    // try to remove it.
    return;
    }
  if (strcmp(this->FindKeyAction(key, modifier), action))
    {
    // If this event doesn't exist, we can't remove it.
    return;
    }
  
  int i;
  KeyEvent *events = new KeyEvent[this->NumberOfKeyEvents];
  
  for (i = 0; i < this->NumberOfKeyEvents; i++)
    {
    events[i].Key = this->KeyEvents[i].Key;
    events[i].Modifier = this->KeyEvents[i].Modifier;
    events[i].Action = new char[strlen(this->KeyEvents[i].Action) + 1];
    strcpy(events[i].Action, this->KeyEvents[i].Action);
    delete [] this->KeyEvents[i].Action;
    }
  if (this->KeyEvents)
    {
    delete [] this->KeyEvents;
    this->KeyEvents = NULL;
    }
  
  int count = 0;
  this->KeyEvents = new KeyEvent[this->NumberOfKeyEvents-1];
  
  for (i = 0; i < this->NumberOfKeyEvents; i++)
    {
    if (events[i].Key != key ||
        events[i].Modifier != modifier ||
        strcmp(events[i].Action, action))
      {
      this->KeyEvents[count].Key = events[i].Key;
      this->KeyEvents[count].Modifier = events[i].Modifier;
      this->KeyEvents[count].Action = new char[strlen(events[i].Action)+1];
      strcpy(this->KeyEvents[count].Action, events[i].Action);
      delete [] events[i].Action;
      count++;
      }
    }
  this->NumberOfKeyEvents--;
}

void vtkKWEventMap::RemoveKeySymEvent(char *keySym, char *action)
{
  if ( ! action)
    {
    // It isn't possible to add a NULL action, so there's no reason to
    // try to remove it.
    return;
    }
  if ( ! keySym)
    {
    // It isn't possible to add a NULL keySym, so there's no reason to
    // try to remove it.
    return;
    }
  if (strcmp(this->FindKeySymAction(keySym), action))
    {
    // If this event doesn't exist, we can't remove it.
    return;
    }
  
  int i;
  KeySymEvent *events = new KeySymEvent[this->NumberOfKeySymEvents];
  
  for (i = 0; i < this->NumberOfKeySymEvents; i++)
    {
    events[i].KeySym = new char[strlen(this->KeySymEvents[i].KeySym) + 1];
    strcpy(events[i].KeySym, this->KeySymEvents[i].KeySym);
    delete [] this->KeySymEvents[i].KeySym;
    events[i].Action = new char[strlen(this->KeySymEvents[i].Action) + 1];
    strcpy(events[i].Action, this->KeySymEvents[i].Action);
    delete [] this->KeySymEvents[i].Action;
    }
  if (this->KeySymEvents)
    {
    delete [] this->KeySymEvents;
    this->KeySymEvents = NULL;
    }
  
  int count = 0;
  this->KeySymEvents = new KeySymEvent[this->NumberOfKeySymEvents-1];
  
  for (i = 0; i < this->NumberOfKeySymEvents; i++)
    {
    if (strcmp(events[i].KeySym, keySym) ||
        strcmp(events[i].Action, action))
      {
      this->KeySymEvents[count].KeySym =
        new char[strlen(events[i].KeySym) + 1];
      strcpy(this->KeySymEvents[count].KeySym, events[i].KeySym);
      delete [] events[i].KeySym;
      this->KeySymEvents[count].Action = new char[strlen(events[i].Action)+1];
      strcpy(this->KeySymEvents[count].Action, events[i].Action);
      delete [] events[i].Action;
      count++;
      }
    }
  this->NumberOfKeySymEvents--;
}

char* vtkKWEventMap::FindMouseAction(int button, int modifier)
{
  int i;
  for (i = 0; i < this->NumberOfMouseEvents; i++)
    {
    if (this->MouseEvents[i].Button == button &&
        this->MouseEvents[i].Modifier == modifier)
      {
      return this->MouseEvents[i].Action;
      }
    }
  
  return NULL;
}

char* vtkKWEventMap::FindKeyAction(char key, int modifier)
{
  int i;
  for (i = 0; i < this->NumberOfKeyEvents; i++)
    {
    if (this->KeyEvents[i].Key == key &&
        this->KeyEvents[i].Modifier == modifier)
      {
      return this->KeyEvents[i].Action;
      }
    }
  
  return NULL;
}

char* vtkKWEventMap::FindKeySymAction(char *keySym)
{
  if ( ! keySym)
    {
    return NULL;
    }
  
  int i;
  for (i = 0; i < this->NumberOfKeySymEvents; i++)
    {
    if ( ! strcmp(this->KeySymEvents[i].KeySym, keySym))
      {
      return this->KeySymEvents[i].Action;
      }
    }
  
  return NULL;
}

void vtkKWEventMap::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
