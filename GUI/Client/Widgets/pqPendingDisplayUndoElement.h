/*=========================================================================

   Program: ParaView
   Module:    pqPendingDisplayUndoElement.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#ifndef __pqPendingDisplayUndoElement_h
#define __pqPendingDisplayUndoElement_h

#include "vtkSMUndoElement.h"
#include "pqWidgetsExport.h"

class pqPipelineSource;
class vtkPVXMLElement;

// pqPendingDisplayUndoElement is an GUI undo element used to ensure that
// "pending displays" in pqMainWindowCore work well with undo/redo.
// When any source is created using pqMainWindowCore, the operation has two
// parts (a) embryo-state: the source is in the embryo-state until the first 
// "Accept" following the creation. In this state, as far as the user is concerned
// the source is not yet available for use. The user can merely set the 
// initialization values for parameters. (b) birth:- This happens on the first
// "Accept" following the creation of the source. Here, the pqMainWindowCore
// creates a display for the source in the active view and the source becomes
// usable. Undo/Redo of the source creation has now been split up into two parts
// as well. After the birth of a source, the first Undo will take the source into
// embryo-state, which another undo will actually remove the source i.e. undo the
// creation of the source. Similarly for redo.
//
// A pqPendingDisplayUndoElement object is pushed on the undo stack to mark both
// states i.e. on element for creation of source and another element on the
// first accept following the source creation. The two undo elements
// are distinguished by the value \c state while calling PendingDisplay().
class PQWIDGETS_EXPORT pqPendingDisplayUndoElement : public vtkSMUndoElement
{
public:
  static pqPendingDisplayUndoElement* New();
  vtkTypeRevisionMacro(pqPendingDisplayUndoElement, vtkSMUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Undo the operation encapsulated by this element.
  // If state==ADD, undo means the source creation is undone, hence the source
  // is removed from the pending displays list.
  // If state==REMOVE, undo means the pending display created for the source
  // was undone, hence the source gets added to the pending displays list.
  virtual int Undo();

  // Description:
  // Redo the operation encaspsulated by this element.
  // If state==ADD, redo means the source creation is redone, hence the source
  // is added to the pending displays list.
  // If state==REMOVE, redo means the pending display created for the source
  // was redone, hence the source gets removed from the pending displays list.
  virtual int Redo();

  // Description:
  // Use this method to setup this element.
  // pqMainWindowCore puts two pqPendingDisplayUndoElements
  // per source. 1 in the undoset containing the creation of the 
  // source (with state==ADD) and 1 in the undoset containig the
  // creation of the pending display for the source (with state==REMOVE).
  void PendingDisplay(pqPipelineSource* source, int state);

  enum {
    ADD = 0,
    REMOVE = 1
  };

protected:
  pqPendingDisplayUndoElement();
  ~pqPendingDisplayUndoElement();
  // Description:
  // Overridden by subclasses to save state specific to the class.
  // \arg \c parent element. 
  virtual void SaveStateInternal(vtkPVXMLElement* root);

  // Description:
  // Overridden by subclasses to load state specific to the class.
  // \arg \c parent element. 
  virtual void LoadStateInternal(vtkPVXMLElement* element);

  vtkPVXMLElement* XMLElement;
  void SetXMLElement(vtkPVXMLElement*);

  int InternalUndoRedo(bool undo);
private:
  pqPendingDisplayUndoElement(const pqPendingDisplayUndoElement&); // Not implemented.
  void operator=(const pqPendingDisplayUndoElement&); // Not implemented.
  
};




#endif

