/*=========================================================================

   Program: ParaView
   Module:    pqUndoStack.h

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

========================================================================*/
#ifndef pqUndoStack_h
#define pqUndoStack_h

#include "pqCoreModule.h"
#include <QObject>

class pqServer;
class vtkSMUndoElement;
class vtkSMUndoStack;
class vtkSMUndoStackBuilder;
class vtkUndoElement;
class vtkUndoSet;      // vistrails
class vtkPVXMLElement; // vistrails

/**
* pqUndoStack represents a vtkSMUndoStack along with a
* a vtkSMUndoStackBuilder. It provides Qt slots to call
* methods on undo stack or builder. Also it converts
* vtk events from the stack/builder to Qt signals.
* The only purpose of this class is to provide Qt
* friendly API to the ServerManager classes.
* All logic must be in the server manager classes (or their
* subclasses).
*/
class PQCORE_EXPORT pqUndoStack : public QObject
{
  Q_OBJECT
public:
  /**
  * If no \c builder is provided a default vtkSMUndoStackBuilder object
  * will be created.
  */
  pqUndoStack(vtkSMUndoStackBuilder* builder = 0, QObject* parent = nullptr);
  ~pqUndoStack() override;

  /**
  * returns if it's possible to undo.
  */
  bool canUndo();

  /**
  * returns if it's possible to redo.
  */
  bool canRedo();

  /**
  * returns the undo label.
  */
  QString undoLabel();

  /**
  * returns the redo label.
  */
  QString redoLabel();

  /**
  * Get the status of the IgnoreAllChanges flag on the
  * stack builder.
  */
  bool ignoreAllChanges() const;

  /**
  * Register Application specific undo elements.
  */
  void registerElementForLoader(vtkSMUndoElement*);

  /**
  * Get if the stack is currently being undone/redone.
  */
  bool getInUndo() const;
  bool getInRedo() const;

  /**
  * vistrails - push an undo set directly onto the undo stack (don't apply the changes - we want to
  * be able to undo them)
  */
  void Push(const char* label, vtkUndoSet* set);
  vtkUndoSet* getLastUndoSet();                         // vistrails
  vtkUndoSet* getUndoSetFromXML(vtkPVXMLElement* root); // vistrails

  /**
  * Get the UndoStackBuilder that is used with that UndoStack
  */
  vtkSMUndoStackBuilder* GetUndoStackBuilder();

  /**
  * Make sure all proxy of all SessionProxyManager get updated
  */
  void updateAllModifiedProxies();

public Q_SLOTS:
  void beginUndoSet(QString label);
  void endUndoSet();

  /**
  * triggers Undo.
  */
  void undo();

  /**
  * triggers Redo.
  */
  void redo();

  /**
  * Clears undo stack.
  */
  void clear();

  /**
  * when the GUI is performing some changes
  * that should not go on the UndoStack at all, it should
  * call beginNonUndoableChanges(). Once it's finished doing
  * these changes, it must call endNonUndoableChanges() to restore
  * the IgnoreAllChanges flag state to the one before the push.
  */
  void beginNonUndoableChanges();
  void endNonUndoableChanges();

  /**
  * One can add arbitrary elements to the
  * undo set currently being built.
  */
  void addToActiveUndoSet(vtkUndoElement* element);

Q_SIGNALS:
  /**
  * Fired to notify interested parites that the stack has changed.
  * Has information to know the status of the top of the stack.
  */
  void stackChanged(bool canUndo, QString undoLabel, bool canRedo, QString redoLabel);

  void canUndoChanged(bool);
  void canRedoChanged(bool);
  void undoLabelChanged(const QString&);
  void redoLabelChanged(const QString&);

  // Fired after undo.
  void undone();
  // Fired after redo.
  void redone();

private Q_SLOTS:
  void onStackChanged();

private:
  class pqImplementation;
  pqImplementation* Implementation;
};

#include "pqApplicationCore.h"

inline void BEGIN_UNDO_SET(const QString& name)
{
  pqUndoStack* usStack = pqApplicationCore::instance()->getUndoStack();
  if (usStack)
  {
    usStack->beginUndoSet(name);
  }
}

inline void END_UNDO_SET()
{
  pqUndoStack* usStack = pqApplicationCore::instance()->getUndoStack();
  if (usStack)
  {
    usStack->endUndoSet();
  }
}

inline void CLEAR_UNDO_STACK()
{
  pqUndoStack* usStack = pqApplicationCore::instance()->getUndoStack();
  if (usStack)
  {
    usStack->clear();
  }
}

inline void ADD_UNDO_ELEM(vtkUndoElement* elem)
{
  pqUndoStack* usStack = pqApplicationCore::instance()->getUndoStack();
  if (usStack)
  {
    usStack->addToActiveUndoSet(elem);
  }
}

inline void BEGIN_UNDO_EXCLUDE()
{
  pqUndoStack* usStack = pqApplicationCore::instance()->getUndoStack();
  if (usStack)
  {
    usStack->beginNonUndoableChanges();
  }
}

inline void END_UNDO_EXCLUDE()
{
  pqUndoStack* usStack = pqApplicationCore::instance()->getUndoStack();
  if (usStack)
  {
    usStack->endNonUndoableChanges();
  }
}

class PQCORE_EXPORT pqScopedUndoExclude
{
public:
  pqScopedUndoExclude() { BEGIN_UNDO_EXCLUDE(); }
  ~pqScopedUndoExclude() { END_UNDO_EXCLUDE(); }
private:
  Q_DISABLE_COPY(pqScopedUndoExclude);
};

class PQCORE_EXPORT pqScopedUndoSet
{
public:
  pqScopedUndoSet(const QString& label) { BEGIN_UNDO_SET(label); }
  ~pqScopedUndoSet() { END_UNDO_SET(); }
private:
  Q_DISABLE_COPY(pqScopedUndoSet);
};

#define SCOPED_UNDO_EXCLUDE() SCOPED_UNDO_EXCLUDE__0(__LINE__)
#define SCOPED_UNDO_EXCLUDE__0(line) pqScopedUndoExclude val##line

#define SCOPED_UNDO_SET(txt) SCOPED_UNDO_SET__0(__LINE__, txt)
#define SCOPED_UNDO_SET__0(line, txt) pqScopedUndoSet val##line(txt)

#endif
