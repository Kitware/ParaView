/*=========================================================================

  Module:    vtkKWEventMap.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWEventMap.h"
#include "vtkObjectFactory.h"

#include "kwsys/stl/string"

vtkCxxRevisionMacro(vtkKWEventMap, "1.2");
vtkStandardNewMacro(vtkKWEventMap);

//----------------------------------------------------------------------------
vtkKWEventMap::vtkKWEventMap()
{
  this->NumberOfMouseEvents  = 0;
  this->NumberOfKeyEvents    = 0;
  this->NumberOfKeySymEvents = 0;
  
  this->MouseEvents  = NULL;
  this->KeyEvents    = NULL;
  this->KeySymEvents = NULL;
}

//----------------------------------------------------------------------------
vtkKWEventMap::~vtkKWEventMap()
{
  this->RemoveAllMouseEvents();
  this->RemoveAllKeyEvents();
  this->RemoveAllKeySymEvents();
}

//----------------------------------------------------------------------------
void vtkKWEventMap::RemoveAllMouseEvents()
{
  if (this->MouseEvents)
    {
    for (int i = 0; i < this->NumberOfMouseEvents; i++)
      {
      delete [] this->MouseEvents[i].Action;
      }
    delete [] this->MouseEvents;
    this->MouseEvents = NULL;
    }
  this->NumberOfMouseEvents = 0;
}

//----------------------------------------------------------------------------
void vtkKWEventMap::RemoveAllKeyEvents()
{
  if (this->KeyEvents)
    {
    for (int i = 0; i < this->NumberOfKeyEvents; i++)
      {
      delete [] this->KeyEvents[i].Action;
      }
    delete [] this->KeyEvents;
    this->KeyEvents = NULL;
    }
  this->NumberOfKeyEvents = 0;
}

//----------------------------------------------------------------------------
void vtkKWEventMap::RemoveAllKeySymEvents()
{
  if (this->KeySymEvents)
    {
    for (int i = 0; i < this->NumberOfKeySymEvents; i++)
      {
      delete [] this->KeySymEvents[i].KeySym;
      delete [] this->KeySymEvents[i].Action;
      }
    delete [] this->KeySymEvents;
    this->KeySymEvents = NULL;
    }
  this->NumberOfKeySymEvents = 0;
}

//----------------------------------------------------------------------------
void vtkKWEventMap::AddMouseEvent(struct MouseEvent *me)
{
  if (me)
    {
    this->AddMouseEvent(me->Button, me->Modifier, me->Action);
    }
}

//----------------------------------------------------------------------------
void vtkKWEventMap::AddMouseEvent(int button, int modifier, const char *action)
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

//----------------------------------------------------------------------------
void vtkKWEventMap::AddKeyEvent(struct KeyEvent *me)
{
  if (me)
    {
    this->AddKeyEvent(me->Key, me->Modifier, me->Action);
    }
}

//----------------------------------------------------------------------------
void vtkKWEventMap::AddKeyEvent(char key, int modifier, const char *action)
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

//----------------------------------------------------------------------------
void vtkKWEventMap::AddKeySymEvent(struct KeySymEvent *me)
{
  if (me)
    {
    this->AddKeySymEvent(me->KeySym, me->Modifier, me->Action);
    }
}

//----------------------------------------------------------------------------
void vtkKWEventMap::AddKeySymEvent(const char *keySym, int modifier, const char *action)
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
  if (this->FindKeySymAction(keySym, modifier))
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
    events[i].Modifier = this->KeySymEvents[i].Modifier;
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
    this->KeySymEvents[i].Modifier = events[i].Modifier;
    this->KeySymEvents[i].Action = new char[strlen(events[i].Action) + 1];
    strcpy(this->KeySymEvents[i].Action, events[i].Action);
    delete [] events[i].Action;
    }
  delete [] events;

  this->KeySymEvents[i].KeySym = new char[strlen(keySym) + 1];
  strcpy(this->KeySymEvents[i].KeySym, keySym);
  this->KeySymEvents[i].Modifier = modifier;
  this->KeySymEvents[i].Action = new char[strlen(action) + 1];
  strcpy(this->KeySymEvents[i].Action, action);
  
  this->NumberOfKeySymEvents++;
}

//----------------------------------------------------------------------------
void vtkKWEventMap::SetMouseEvent(struct MouseEvent *me)
{
  if (me)
    {
    this->SetMouseEvent(me->Button, me->Modifier, me->Action);
    }
}

//----------------------------------------------------------------------------
void vtkKWEventMap::SetMouseEvent(int button, int modifier, const char *action)
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

//----------------------------------------------------------------------------
void vtkKWEventMap::SetKeyEvent(char key, int modifier, const char *action)
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

//----------------------------------------------------------------------------
void vtkKWEventMap::SetKeySymEvent(const char *keySym, int modifier, const char *action)
{
  int i;

  if ( ! action)
    {
    vtkErrorMacro("Can't set NULL action");
    return;
    }
  
  for (i = 0; i < this->NumberOfKeySymEvents; i++)
    {
    if (this->KeySymEvents[i].Modifier == modifier &&
        ! strcmp(this->KeySymEvents[i].KeySym, keySym))
      {
      delete [] this->KeySymEvents[i].Action;
      this->KeySymEvents[i].Action = new char[strlen(action)+1];
      strcpy(this->KeySymEvents[i].Action, action);
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWEventMap::RemoveMouseEvent(struct MouseEvent *me)
{
  if (me)
    {
    this->RemoveMouseEvent(me->Button, me->Modifier, me->Action);
    }
}

//----------------------------------------------------------------------------
void vtkKWEventMap::RemoveMouseEvent(int button, int modifier, const char *action)
{
  if ( ! action)
    {
    // No action specified, then remove all actions
    do
      {
      action = this->FindMouseAction(button, modifier);
      if (action)
        {
        this->RemoveMouseEvent(button, modifier, action);
        }
      } while (action);
    return;
    }

  if (strcmp(this->FindMouseAction(button, modifier), action))
    {
    // If this event doesn't exist, we can't remove it.
    return;
    }
  
  int i;
  MouseEvent *events = NULL;

  if (this->NumberOfMouseEvents > 1)
    {
    events = new MouseEvent[this->NumberOfMouseEvents];
    for (i = 0; i < this->NumberOfMouseEvents; i++)
      {
      events[i].Button = this->MouseEvents[i].Button;
      events[i].Modifier = this->MouseEvents[i].Modifier;
      events[i].Action = new char[strlen(this->MouseEvents[i].Action) + 1];
      strcpy(events[i].Action, this->MouseEvents[i].Action);
      }
    }

  // Need to keep it around in case sombody is passing a pointer
  // to an action that we are going to delete below
  kwsys_stl::string old_action(action);

  if (this->MouseEvents)
    {
    for (i = 0; i < this->NumberOfMouseEvents; i++)
      {
      delete [] this->MouseEvents[i].Action;
      }
    delete [] this->MouseEvents;
    this->MouseEvents = NULL;
    }
  
  if (this->NumberOfMouseEvents > 1)
    {
    int count = 0;
    this->MouseEvents = new MouseEvent[this->NumberOfMouseEvents-1];
  
    for (i = 0; i < this->NumberOfMouseEvents; i++)
      {
      if (events[i].Button != button ||
          events[i].Modifier != modifier ||
          strcmp(events[i].Action, old_action.c_str()))
        {
        this->MouseEvents[count].Button = events[i].Button;
        this->MouseEvents[count].Modifier = events[i].Modifier;
        this->MouseEvents[count].Action = new char[strlen(events[i].Action)+1];
        strcpy(this->MouseEvents[count].Action, events[i].Action);
        delete [] events[i].Action;
        count++;
        }
      }
    delete [] events;
    }

  this->NumberOfMouseEvents--;
}

//----------------------------------------------------------------------------
void vtkKWEventMap::RemoveKeyEvent(char key, int modifier, const char *action)
{
  if ( ! action)
    {
    // No action specified, then remove all actions
    do
      {
      action = this->FindKeyAction(key, modifier);
      if (action)
        {
        this->RemoveKeyEvent(key, modifier, action);
        }
      } while (action);
    return;
    }

  if (strcmp(this->FindKeyAction(key, modifier), action))
    {
    // If this event doesn't exist, we can't remove it.
    return;
    }
  
  int i;
  KeyEvent *events = NULL;
  
  if (this->NumberOfKeyEvents > 1)
    {
    events = new KeyEvent[this->NumberOfKeyEvents];
    for (i = 0; i < this->NumberOfKeyEvents; i++)
      {
      events[i].Key = this->KeyEvents[i].Key;
      events[i].Modifier = this->KeyEvents[i].Modifier;
      events[i].Action = new char[strlen(this->KeyEvents[i].Action) + 1];
      strcpy(events[i].Action, this->KeyEvents[i].Action);
      }
    }

  // Need to keep it around in case sombody is passing a pointer
  // to an action that we are going to delete below
  kwsys_stl::string old_action(action);

  if (this->KeyEvents)
    {
    for (i = 0; i < this->NumberOfKeyEvents; i++)
      {
      delete [] this->KeyEvents[i].Action;
      }
    delete [] this->KeyEvents;
    this->KeyEvents = NULL;
    }
  
  if (this->NumberOfKeyEvents > 1)
    {
    int count = 0;
    this->KeyEvents = new KeyEvent[this->NumberOfKeyEvents-1];
  
    for (i = 0; i < this->NumberOfKeyEvents; i++)
      {
      if (events[i].Key != key ||
          events[i].Modifier != modifier ||
          strcmp(events[i].Action, old_action.c_str()))
        {
        this->KeyEvents[count].Key = events[i].Key;
        this->KeyEvents[count].Modifier = events[i].Modifier;
        this->KeyEvents[count].Action = new char[strlen(events[i].Action)+1];
        strcpy(this->KeyEvents[count].Action, events[i].Action);
        delete [] events[i].Action;
        count++;
        }
      }
    delete [] events;
    }

  this->NumberOfKeyEvents--;
}

//----------------------------------------------------------------------------
void vtkKWEventMap::RemoveKeySymEvent(const char *keySym, int modifier, const char *action)
{
  if ( ! action)
    {
    // No action specified, then remove all actions
    do
      {
      action = this->FindKeySymAction(keySym, modifier);
      if (action)
        {
        this->RemoveKeySymEvent(keySym, modifier, action);
        }
      } while (action);
    return;
    }

  if ( ! keySym)
    {
    // It isn't possible to add a NULL keySym, so there's no reason to
    // try to remove it.
    return;
    }
  if (strcmp(this->FindKeySymAction(keySym, modifier), action))
    {
    // If this event doesn't exist, we can't remove it.
    return;
    }
  
  int i;
  KeySymEvent *events = NULL;
  
  if (this->NumberOfKeySymEvents > 1)
    {
    events = new KeySymEvent[this->NumberOfKeySymEvents];
    for (i = 0; i < this->NumberOfKeySymEvents; i++)
      {
      events[i].KeySym = new char[strlen(this->KeySymEvents[i].KeySym) + 1];
      strcpy(events[i].KeySym, this->KeySymEvents[i].KeySym);
      events[i].Modifier = this->KeySymEvents[i].Modifier;
      events[i].Action = new char[strlen(this->KeySymEvents[i].Action) + 1];
      strcpy(events[i].Action, this->KeySymEvents[i].Action);
      }
    }

  // Need to keep it around in case sombody is passing a pointer
  // to an action that we are going to delete below
  kwsys_stl::string old_action(action);
  kwsys_stl::string old_keysym(keySym);

  if (this->KeySymEvents)
    {
    for (i = 0; i < this->NumberOfKeySymEvents; i++)
      {
      delete [] this->KeySymEvents[i].KeySym;
      delete [] this->KeySymEvents[i].Action;
      }
    delete [] this->KeySymEvents;
    this->KeySymEvents = NULL;
    }
  
  if (this->NumberOfKeySymEvents > 1)
    {
    int count = 0;
    this->KeySymEvents = new KeySymEvent[this->NumberOfKeySymEvents-1];
  
    for (i = 0; i < this->NumberOfKeySymEvents; i++)
      {
      if (strcmp(events[i].KeySym, old_keysym.c_str()) ||
          events[i].Modifier != modifier ||
          strcmp(events[i].Action, old_action.c_str()))
        {
        this->KeySymEvents[count].KeySym =
          new char[strlen(events[i].KeySym) + 1];
        strcpy(this->KeySymEvents[count].KeySym, events[i].KeySym);
        delete [] events[i].KeySym;
        this->KeySymEvents[count].Modifier = events[i].Modifier;
        this->KeySymEvents[count].Action = 
          new char[strlen(events[i].Action)+1];
        strcpy(this->KeySymEvents[count].Action, events[i].Action);
        delete [] events[i].Action;
        count++;
        }
      }
    delete [] events;
    }

  this->NumberOfKeySymEvents--;
}

//----------------------------------------------------------------------------
const char* vtkKWEventMap::FindMouseAction(int button, int modifier)
{
  if (this->MouseEvents)
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
    }
  
  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkKWEventMap::FindKeyAction(char key, int modifier)
{
  if (this->KeyEvents)
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
    }

  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkKWEventMap::FindKeySymAction(const char *keySym, int modifier)
{
  if (keySym && this->KeySymEvents)
    {
    int i;
    for (i = 0; i < this->NumberOfKeySymEvents; i++)
      {
      if (this->KeySymEvents[i].Modifier == modifier &&
          ! strcmp(this->KeySymEvents[i].KeySym, keySym))
        {
        return this->KeySymEvents[i].Action;
        }
      }
    }
  
  return NULL;
}

//----------------------------------------------------------------------------
struct vtkKWEventMap::MouseEvent* vtkKWEventMap::GetMouseEvent(int index)
{
  if (index < 0 || index >= this->NumberOfMouseEvents)
    {
    return NULL;
    }

  return &this->MouseEvents[index];
}

//----------------------------------------------------------------------------
struct vtkKWEventMap::KeyEvent* vtkKWEventMap::GetKeyEvent(int index)
{
  if (index < 0 || index >= this->NumberOfKeyEvents)
    {
    return NULL;
    }

  return &this->KeyEvents[index];
}

//----------------------------------------------------------------------------
struct vtkKWEventMap::KeySymEvent* vtkKWEventMap::GetKeySymEvent(int index)
{
  if (index < 0 || index >= this->NumberOfKeySymEvents)
    {
    return NULL;
    }

  return &this->KeySymEvents[index];
}

//----------------------------------------------------------------------------
void vtkKWEventMap::ShallowCopy(vtkKWEventMap *map)
{
  if (!map || map == this)
    {
    return;
    }

  int i;

  this->RemoveAllMouseEvents();
  for (i = 0; i < map->GetNumberOfMouseEvents(); i++)
    {
    this->AddMouseEvent(map->GetMouseEvent(i));
    }

  this->RemoveAllKeyEvents();
  for (i = 0; i < map->GetNumberOfKeyEvents(); i++)
    {
    this->AddKeyEvent(map->GetKeyEvent(i));
    }

  this->RemoveAllKeySymEvents();
  for (i = 0; i < map->GetNumberOfKeySymEvents(); i++)
    {
    this->AddKeySymEvent(map->GetKeySymEvent(i));
    }
}

//----------------------------------------------------------------------------
void vtkKWEventMap::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfMouseEvents: " 
     << this->NumberOfMouseEvents << endl;;

  if (this->MouseEvents)
    {
    for (int i = 0; i < this->NumberOfMouseEvents; i++)
      {
      os << indent << " * ";
      if (this->MouseEvents[i].Modifier & vtkKWEventMap::ShiftModifier)
        {
        os << "Shift + ";
        }
      if (this->MouseEvents[i].Modifier & vtkKWEventMap::ControlModifier)
        {
        os << "Control + ";
        }
      switch (this->MouseEvents[i].Button)
        {
        case 0:
          os << "Left";
          break;
        case 1:
          os << "Middle";
          break;
        case 2:
          os << "Right";
          break;
        }
      if (this->MouseEvents[i].Action)
        {
        os << " : " << this->MouseEvents[i].Action;
        }
      os << endl;
      }
    }

  os << indent << "NumberOfKeyEvents: " 
     << this->NumberOfKeyEvents << endl;;
  if (this->KeyEvents)
    {
    for (int i = 0; i < this->NumberOfKeyEvents; i++)
      {
      os << indent << " * ";
      if (this->KeyEvents[i].Modifier & vtkKWEventMap::ShiftModifier)
        {
        os << "Shift + ";
        }
      if (this->KeyEvents[i].Modifier & vtkKWEventMap::ControlModifier)
        {
        os << "Control + ";
        }
      os << "'" << this->KeyEvents[i].Key << "'";
      if (this->KeyEvents[i].Action)
        {
        os << " : " << this->KeyEvents[i].Action;
        }
      os << endl;
      }
    }

  os << indent << "NumberOfKeySymEvents: " 
     << this->NumberOfKeySymEvents << endl;;
  if (this->KeySymEvents)
    {
    for (int i = 0; i < this->NumberOfKeySymEvents; i++)
      {
      os << indent << " * ";
      if (this->KeySymEvents[i].Modifier & vtkKWEventMap::ShiftModifier)
        {
        os << "Shift + ";
        }
      if (this->KeySymEvents[i].Modifier & vtkKWEventMap::ControlModifier)
        {
        os << "Control + ";
        }
      if (this->KeySymEvents[i].KeySym)
        {
        os << "'" << this->KeySymEvents[i].KeySym << "'";
        }
      if (this->KeySymEvents[i].Action)
        {
        os << " : " << this->KeySymEvents[i].Action;
        }
      os << endl;
      }
    }
}

