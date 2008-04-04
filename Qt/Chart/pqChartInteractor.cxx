/*=========================================================================

   Program: ParaView
   Module:    pqChartInteractor.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

/// \file pqChartInteractor.cxx
/// \date 5/2/2007

#include "pqChartInteractor.h"

#include "pqChartContentsSpace.h"
#include "pqChartMouseBox.h"
#include "pqChartMouseFunction.h"

#include <QCursor>
#include <QKeyEvent>
#include <QList>
#include <QMouseEvent>
#include <QRect>
#include <QVector>
#include <QWheelEvent>


class pqChartInteractorModeItem
{
public:
  pqChartInteractorModeItem(pqChartMouseFunction *function,
      Qt::KeyboardModifiers modifiers);
  pqChartInteractorModeItem(const pqChartInteractorModeItem &other);
  ~pqChartInteractorModeItem() {}

  pqChartMouseFunction *Function;
  Qt::KeyboardModifiers Modifiers;
};


class pqChartInteractorMode
{
public:
  pqChartInteractorMode();
  pqChartInteractorMode(const pqChartInteractorMode &other);
  ~pqChartInteractorMode() {}

  pqChartMouseFunction *getFunction(Qt::KeyboardModifiers modifiers);

  QList<pqChartInteractorModeItem> Functions;
};


class pqChartInteractorModeList
{
public:
  pqChartInteractorModeList();
  pqChartInteractorModeList(const pqChartInteractorModeList &other);
  ~pqChartInteractorModeList() {}

  pqChartInteractorMode *getCurrentMode();

  QList<pqChartInteractorMode> Modes;
  int CurrentMode;
};


class pqChartInteractorInternal
{
public:
  pqChartInteractorInternal();
  ~pqChartInteractorInternal() {}

  pqChartInteractorModeList *getModeList(Qt::MouseButton button);

  pqChartMouseFunction *Owner;
  pqChartInteractorModeList *OwnerList;
  QVector<pqChartInteractorModeList> Buttons;
};


//----------------------------------------------------------------------------
pqChartInteractorModeItem::pqChartInteractorModeItem(
    pqChartMouseFunction *function, Qt::KeyboardModifiers modifiers)
{
  this->Function = function;
  this->Modifiers = modifiers;
}

pqChartInteractorModeItem::pqChartInteractorModeItem(
    const pqChartInteractorModeItem &other)
{
  this->Function = other.Function;
  this->Modifiers = other.Modifiers;
}


//----------------------------------------------------------------------------
pqChartInteractorMode::pqChartInteractorMode()
  : Functions()
{
}

pqChartInteractorMode::pqChartInteractorMode(
    const pqChartInteractorMode &other)
  : Functions()
{
  // Copy the list of functions.
  QList<pqChartInteractorModeItem>::ConstIterator iter;
  for(iter = other.Functions.begin(); iter != other.Functions.end(); ++iter)
    {
    this->Functions.append(*iter);
    }
}

pqChartMouseFunction *pqChartInteractorMode::getFunction(
    Qt::KeyboardModifiers modifiers)
{
  // If there is only one function, ignore the event modifiers.
  if(this->Functions.size() == 1)
    {
    return this->Functions[0].Function;
    }

  QList<pqChartInteractorModeItem>::Iterator iter = this->Functions.begin();
  for( ; iter != this->Functions.end(); ++iter)
    {
    if(modifiers == iter->Modifiers)
      {
      return iter->Function;
      }
    }

  return 0;
}


//----------------------------------------------------------------------------
pqChartInteractorModeList::pqChartInteractorModeList()
  : Modes()
{
  this->CurrentMode = 0;
}

pqChartInteractorModeList::pqChartInteractorModeList(
    const pqChartInteractorModeList &other)
  : Modes()
{
  this->CurrentMode = other.CurrentMode;

  // Copy the mode list.
  QList<pqChartInteractorMode>::ConstIterator iter = other.Modes.begin();
  for( ; iter != other.Modes.end(); ++iter)
    {
    this->Modes.append(*iter);
    }
}

pqChartInteractorMode *pqChartInteractorModeList::getCurrentMode()
{
  if(this->CurrentMode < this->Modes.size())
    {
    return &this->Modes[this->CurrentMode];
    }

  return 0;
}


//----------------------------------------------------------------------------
pqChartInteractorInternal::pqChartInteractorInternal()
  : Buttons(3)
{
  this->Owner = 0;
  this->OwnerList = 0;
}

pqChartInteractorModeList *pqChartInteractorInternal::getModeList(
    Qt::MouseButton button)
{
  if(button == Qt::LeftButton)
    {
    return &this->Buttons[0];
    }
  else if(button == Qt::MidButton)
    {
    return &this->Buttons[1];
    }
  else if(button == Qt::RightButton)
    {
    return &this->Buttons[2];
    }

  return 0;
}


//----------------------------------------------------------------------------
pqChartInteractor::pqChartInteractor(QObject *parentObject)
  : QObject(parentObject)
{
  this->Internal = new pqChartInteractorInternal();
  this->Contents = 0;
  this->MouseBox = 0;
  this->XModifier = Qt::ControlModifier;
  this->YModifier = Qt::AltModifier;
}

pqChartInteractor::~pqChartInteractor()
{
  delete this->Internal;
}

void pqChartInteractor::setContentsSpace(pqChartContentsSpace *space)
{
  this->Contents = space;
}

void pqChartInteractor::setMouseBox(pqChartMouseBox *box)
{
  this->MouseBox = box;

  // Update the mouse box in all the functions.
  QVector<pqChartInteractorModeList>::Iterator iter =
      this->Internal->Buttons.begin();
  for( ; iter != this->Internal->Buttons.end(); ++iter)
    {
    QList<pqChartInteractorMode>::Iterator jter = iter->Modes.begin();
    for(int index = 0; jter != iter->Modes.end(); ++jter, ++index)
      {
      QList<pqChartInteractorModeItem>::Iterator kter =
          jter->Functions.begin();
      for( ; kter != jter->Functions.end(); ++kter)
        {
        kter->Function->setMouseBox(this->MouseBox);
        }
      }
    }
}

void pqChartInteractor::setFunction(pqChartMouseFunction *function,
    Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
{
  this->removeFunctions(button);
  this->addFunction(function, button, modifiers);
}

void pqChartInteractor::addFunction(pqChartMouseFunction *function,
    Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
{
  if(!function)
    {
    return;
    }

  pqChartInteractorModeList *list = this->Internal->getModeList(button);
  if(list)
    {
    pqChartInteractorMode *mode = 0;
    if(function->isCombinable())
      {
      // If the function is combinable, search for a compatible mode.
      QList<pqChartInteractorMode>::Iterator iter = list->Modes.begin();
      for( ; iter != list->Modes.end(); ++iter)
        {
        bool canCombine = true;
        QList<pqChartInteractorModeItem>::Iterator jter =
            iter->Functions.begin();
        for( ; jter != iter->Functions.end(); ++jter)
          {
          if(!jter->Function->isCombinable())
            {
            canCombine = false;
            break;
            }

          if(modifiers == jter->Modifiers)
            {
            canCombine = false;
            break;
            }
          }

        if(canCombine)
          {
          mode = &(*iter);
          break;
          }
        }
      }

    if(!mode)
      {
      // Add a new mode if the function can't be added to any of the
      // current modes.
      list->Modes.append(pqChartInteractorMode());
      mode = &list->Modes.last();
      }

    // Finally, add the function to the mode.
    mode->Functions.append(pqChartInteractorModeItem(function, modifiers));
    function->setMouseBox(this->MouseBox);
    this->connect(function, SIGNAL(repaintNeeded()),
        this, SIGNAL(repaintNeeded()));
    this->connect(function, SIGNAL(repaintNeeded(const QRect &)),
        this, SIGNAL(repaintNeeded(const QRect &)));
    this->connect(function, SIGNAL(cursorChangeRequested(const QCursor &)),
        this, SIGNAL(cursorChangeRequested(const QCursor &)));
    this->connect(function, SIGNAL(interactionStarted(pqChartMouseFunction *)),
        this, SLOT(beginState(pqChartMouseFunction *)));
    this->connect(
        function, SIGNAL(interactionFinished(pqChartMouseFunction *)),
        this, SLOT(endState(pqChartMouseFunction *)));
    }
}

void pqChartInteractor::removeFunction(pqChartMouseFunction *function)
{
  if(!function)
    {
    return;
    }

  // If the function being removed is currently active, cancel the
  // mouse state.
  if(function == this->Internal->Owner)
    {
    this->Internal->Owner->setMouseOwner(false);
    this->Internal->Owner = 0;
    this->Internal->OwnerList = 0;
    }

  // Find the function and remove it from the list.
  QVector<pqChartInteractorModeList>::Iterator iter =
      this->Internal->Buttons.begin();
  for( ; iter != this->Internal->Buttons.end(); ++iter)
    {
    QList<pqChartInteractorMode>::Iterator jter = iter->Modes.begin();
    for(int index = 0; jter != iter->Modes.end(); ++jter, ++index)
      {
      QList<pqChartInteractorModeItem>::Iterator kter =
          jter->Functions.begin();
      for( ; kter != jter->Functions.end(); ++kter)
        {
        if(function == kter->Function)
          {
          jter->Functions.erase(kter);
          if(jter->Functions.size() == 0)
            {
            // Remove the mode if it is empty.
            iter->Modes.erase(jter);
            if(index == iter->CurrentMode)
              {
              iter->CurrentMode = 0;
              }
            }

          break;
          }
        }
      }
    }

  // Disconnect from the function signals.
  this->disconnect(function, 0, this, 0);
  function->setMouseBox(0);
}

void pqChartInteractor::removeFunctions(Qt::MouseButton button)
{
  pqChartInteractorModeList *list = this->Internal->getModeList(button);
  if(list)
    {
    // If the button contains an active function, cancel the mouse
    // state before removing the button's functions.
    if(this->Internal->Owner && list == this->Internal->OwnerList)
      {
      this->Internal->Owner->setMouseOwner(false);
      this->Internal->Owner = 0;
      this->Internal->OwnerList = 0;
      }

    // Disconnect from the function signals.
    QList<pqChartInteractorMode>::Iterator iter = list->Modes.begin();
    for( ; iter != list->Modes.end(); ++iter)
      {
      QList<pqChartInteractorModeItem>::Iterator jter =
          iter->Functions.begin();
      for( ; jter != iter->Functions.end(); ++jter)
        {
        this->disconnect(jter->Function, 0, this, 0);
        jter->Function->setMouseBox(0);
        }
      }

    // Clear all the button functions.
    list->CurrentMode = 0;
    list->Modes.clear();
    }
}

void pqChartInteractor::removeAllFunctions()
{
  this->removeFunctions(Qt::LeftButton);
  this->removeFunctions(Qt::MidButton);
  this->removeFunctions(Qt::RightButton);
}

int pqChartInteractor::getNumberOfModes(Qt::MouseButton button) const
{
  pqChartInteractorModeList *list = this->Internal->getModeList(button);
  if(list)
    {
    return list->Modes.size();
    }

  return 0;
}

int pqChartInteractor::getMode(Qt::MouseButton button) const
{
  pqChartInteractorModeList *list = this->Internal->getModeList(button);
  if(list)
    {
    return list->CurrentMode;
    }

  return 0;
}

void pqChartInteractor::setMode(Qt::MouseButton button, int index)
{
  pqChartInteractorModeList *list = this->Internal->getModeList(button);
  if(list && index >= 0 && index < list->Modes.size())
    {
    list->CurrentMode = index;
    }
}

bool pqChartInteractor::keyPressEvent(QKeyEvent *e)
{
  if(!this->Contents)
    {
    return false;
    }

  bool handled = true;
  if(e->key() == Qt::Key_Plus || e->key() == Qt::Key_Minus ||
      e->key() == Qt::Key_Equal)
    {
    // If only the ctrl key is down, zoom only in the x. If only
    // the alt key is down, zoom only in the y. Otherwise, zoom
    // both axes by the same amount. Mask off the shift key since
    // it is needed to press the plus key.
    pqChartContentsSpace::InteractFlags flags = pqChartContentsSpace::ZoomBoth;
    Qt::KeyboardModifiers state = e->modifiers() & (Qt::ControlModifier |
        Qt::AltModifier | Qt::MetaModifier);
    if(state & this->XModifier)
      {
      flags = pqChartContentsSpace::ZoomXOnly;
      }
    else if(state & this->YModifier)
      {
      flags = pqChartContentsSpace::ZoomYOnly;
      }

    // Zoom in for the plus/equal key and out for the minus key.
    if(e->key() == Qt::Key_Minus)
      {
      this->Contents->zoomOut(flags);
      }
    else
      {
      this->Contents->zoomIn(flags);
      }
    }
  else if(e->key() == Qt::Key_Up)
    {
    this->Contents->panUp();
    }
  else if(e->key() == Qt::Key_Down)
    {
    this->Contents->panDown();
    }
  else if(e->key() == Qt::Key_Left)
    {
    if(e->modifiers() & Qt::AltModifier)
      {
      this->Contents->historyPrevious();
      }
    else
      {
      this->Contents->panLeft();
      }
    }
  else if(e->key() == Qt::Key_Right)
    {
    if(e->modifiers() & Qt::AltModifier)
      {
      this->Contents->historyNext();
      }
    else
      {
      this->Contents->panRight();
      }
    }
  else
    {
    handled = false;
    }

  return handled;
}

void pqChartInteractor::mousePressEvent(QMouseEvent *e)
{
  // Find the mode list associated with a button. If a function on
  // another button owns the mouse state, don't pass the event to the
  // button's functions.
  bool handled = false;
  pqChartInteractorModeList *list = this->Internal->getModeList(e->button());
  if(list && (!this->Internal->OwnerList || list == this->Internal->OwnerList))
    {
    // If there is an active function, send it the event. If not,
    // find the function for the current mode and modifiers.
    pqChartMouseFunction *function = this->Internal->Owner;
    if(!function)
      {
      pqChartInteractorMode *mode = list->getCurrentMode();
      if(mode)
        {
        function = mode->getFunction(e->modifiers());
        }
      }

    if(function)
      {
      handled = function->mousePressEvent(e, this->Contents);
      }
    }

  if(handled || this->Internal->Owner)
    {
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

void pqChartInteractor::mouseMoveEvent(QMouseEvent *e)
{
  // See which buttons are pressed.
  pqChartInteractorModeList *left = 0;
  pqChartInteractorModeList *middle = 0;
  pqChartInteractorModeList *right = 0;
  Qt::MouseButtons buttons = e->buttons();
  if(buttons & Qt::LeftButton)
    {
    left = this->Internal->getModeList(Qt::LeftButton);
    }

  if(buttons & Qt::MidButton)
    {
    middle = this->Internal->getModeList(Qt::MidButton);
    }

  if(buttons & Qt::RightButton)
    {
    right = this->Internal->getModeList(Qt::RightButton);
    }

  bool handled = false;
  if(left || middle || right)
    {
    // If more than one button is pressed and no function is active,
    // it is unclear which function to call. An active function can
    // be called even if multiple buttons are pressed.
    pqChartMouseFunction *function = 0;
    bool multiple = (left && middle) || (left && right) || (middle && right);
    if(this->Internal->Owner)
      {
      // Make sure the owner's button is pressed.
      if(this->Internal->OwnerList == left ||
          this->Internal->OwnerList == middle ||
          this->Internal->OwnerList == right)
        {
        function = this->Internal->Owner;
        }
      }
    else if(!multiple)
      {
      pqChartInteractorModeList *list = left;
      if(!list)
        {
        list = middle;
        }

      if(!list)
        {
        list = right;
        }

      pqChartInteractorMode *mode = list->getCurrentMode();
      if(mode)
        {
        function = mode->getFunction(e->modifiers());
        }
      }

    if(function)
      {
      handled = function->mouseMoveEvent(e, this->Contents);
      }
    }

  if(handled)
    {
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

void pqChartInteractor::mouseReleaseEvent(QMouseEvent *e)
{
  // Find the mode list associated with a button. Always send the
  // mouse release event even if a function on another button owns
  // the state. This ensures that mouse press events sent to mouse
  // functions receive mouse release events.
  bool handled = false;
  pqChartInteractorModeList *list = this->Internal->getModeList(e->button());
  if(list)
    {
    // If there is an active function, send it the event. If not,
    // find the function for the current mode and modifiers.
    pqChartMouseFunction *function = 0;
    if(this->Internal->OwnerList == list)
      {
      function = this->Internal->Owner;
      }

    if(!function)
      {
      pqChartInteractorMode *mode = list->getCurrentMode();
      if(mode)
        {
        function = mode->getFunction(e->modifiers());
        }
      }

    if(function)
      {
      handled = function->mouseReleaseEvent(e, this->Contents);
      }
    }

  if(handled || this->Internal->Owner)
    {
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

void pqChartInteractor::mouseDoubleClickEvent(QMouseEvent *e)
{
  // Find the mode list associated with a button. If a function on
  // another button owns the mouse state, don't pass the event to the
  // button's functions.
  bool handled = false;
  pqChartInteractorModeList *list = this->Internal->getModeList(e->button());
  if(list && (!this->Internal->OwnerList || list == this->Internal->OwnerList))
    {
    // If there is an active function, send it the event. If not,
    // find the function for the current mode and modifiers.
    pqChartMouseFunction *function = this->Internal->Owner;
    if(!function)
      {
      pqChartInteractorMode *mode = list->getCurrentMode();
      if(mode)
        {
        function = mode->getFunction(e->modifiers());
        }
      }

    if(function)
      {
      handled = function->mouseDoubleClickEvent(e, this->Contents);
      }
    }

  if(handled || this->Internal->Owner)
    {
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

void pqChartInteractor::wheelEvent(QWheelEvent *e)
{
  if(this->Contents)
    {
    pqChartContentsSpace::InteractFlags flags = pqChartContentsSpace::ZoomBoth;
    if(e->modifiers() & this->XModifier)
      {
      flags = pqChartContentsSpace::ZoomXOnly;
      }
    else if(e->modifiers() & this->YModifier)
      {
      flags = pqChartContentsSpace::ZoomYOnly;
      }

    // Get the current mouse position and convert it to contents coords.
    this->Contents->handleWheelZoom(e->delta(), e->pos(), flags);
    }

  e->accept();
}

void pqChartInteractor::beginState(pqChartMouseFunction *owner)
{
  if(this->Internal->Owner == 0)
    {
    // Find the mouse button this function is attached to.
    QVector<pqChartInteractorModeList>::Iterator iter =
        this->Internal->Buttons.begin();
    for( ; iter != this->Internal->Buttons.end(); ++iter)
      {
      QList<pqChartInteractorMode>::Iterator jter = iter->Modes.begin();
      for( ; jter != iter->Modes.end(); ++jter)
        {
        QList<pqChartInteractorModeItem>::Iterator kter =
            jter->Functions.begin();
        for( ; kter != jter->Functions.end(); ++kter)
          {
          if(owner == kter->Function)
            {
            owner->setMouseOwner(true);
            this->Internal->Owner = owner;
            this->Internal->OwnerList = &(*iter);
            break;
            }
          }
        }
      }
    }
}

void pqChartInteractor::endState(pqChartMouseFunction *owner)
{
  if(owner && this->Internal->Owner == owner)
    {
    owner->setMouseOwner(false);
    this->Internal->Owner = 0;
    this->Internal->OwnerList = 0;
    }
}


