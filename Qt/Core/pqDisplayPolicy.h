/*=========================================================================

   Program: ParaView
   Module:    pqDisplayPolicy.h

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
#ifndef __pqDisplayPolicy_h
#define __pqDisplayPolicy_h

#include <QObject>
#include "pqCoreExport.h" // Needed for PQCORE_EXPORT macro

class pqPipelineSource;
class pqGenericViewModule;
class pqConsumerDisplay;
class vtkSMProxy;

/// Display policy defines the application specific policy
/// for creating display proxies. Given a pair of a proxy to be displayed 
/// and a view proxy in which to display, this class must tell the type 
/// of display to create, if any. Custom applications can subclass 
/// this to define their own policy. The pqApplicationCore maintains
/// an instance of the policy used by the application. Custom applications
/// should set their own policy instance on the global application core 
/// instance.
class PQCORE_EXPORT pqDisplayPolicy : public QObject
{
  Q_OBJECT
public:
  pqDisplayPolicy(QObject* p);
  virtual ~pqDisplayPolicy();

  /// Creates a new display proxy for the (source, view) pair and returns it.
  /// The caller must release the reference when it's done with the returned proxy.
  /// Both view and source must be non-null. If source cannot be displayed in the
  /// view, then this method will return NULL. This method should not bother
  /// about preferred views for the source (use createPreferredDisplay instead). 
  /// It should simply create the display for the given arguments.
  virtual vtkSMProxy* newDisplayProxy(pqPipelineSource* source,
    pqGenericViewModule* view) const;

  /// Returns a new display for thethe given (source,view) pair, or NULL 
  /// on failure. If the \c view is not a preferred view to display the source
  /// this method will attempt to create a new view of the preferred type
  /// and create a display for that view. In other words, the argument \c view
  /// is merely a suggestion, it's not necessary that the new display will
  /// be added to the same view. 
  /// In the current implementation the rules as as under:
  /// <ul>
  /// <li>If \c dont_create_view is true and \c view is non-null, then a display
  ///     is created for the given view and returned. If \c view is null, then
  ///     NULL is returned, since we can't create a new view. </li>
  /// <li> If \c dont_create_view is false and \c view is non-null:
  ///     <ul>
  ///     <li>If \c view is of preferred type, we added display to \c view itself.
  ///     </li>
  ///     <li>If \c view is not of preferred type, we check if a view of
  ///         the preferred type exists. 
  ///         If not, then alone we create a a new view of the preferred type.
  ///         Otherwise the display is added to the current \c view itself, if possible.
  ///     </li>
  ///     </ul>
  /// </li>
  /// <li> If \c dont_create_view is false and view is null, then a new view of 
  ///      the preferred type is created and the display added to that view.
  /// </li>
  /// </ul>
  /// or not of the type preferred by the source, it may create a new view and 
  /// add the displayto new view. \c dont_create_view can be used to 
  /// override this behaviour.
  virtual pqConsumerDisplay* createPreferredDisplay(
    pqPipelineSource* source, pqGenericViewModule* view,
    bool dont_create_view) const;

  /// Set the visibility of the source in the given view. 
  /// Current implementation creates a new display for the source, if possible, 
  /// if none exists. If view is NULL, then a new view of "suitable" type will 
  /// be created for the source. Since custom applications may not necessarily
  /// create new views, we provide this as part of display policy which can 
  /// be easily overridden by creating a new subclass.
  virtual pqConsumerDisplay* setDisplayVisibility(
    pqPipelineSource* source, pqGenericViewModule* view, bool visible) const;

protected:
  /// Determines the type of view that's preferred by the \c source. If \c view
  /// is of the preferred type, returns it. Otherwise a new view of the preferred 
  /// type may be created and returned.
  virtual pqGenericViewModule* getPreferredView(pqPipelineSource* source,
    pqGenericViewModule* view) const;
};

#endif

