/*=========================================================================

   Program: ParaView
   Module:  pqExtractor.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#ifndef pqExtractor_h
#define pqExtractor_h

#include "pqProxy.h"
#include <QScopedPointer>

class pqView;

/**
 * @class pqExtractor
 * @brief pqProxy subclass for extractors
 *
 * pqExtractor is an Qt-adaptor for an extractor proxy.
 * Extractors are used in ParaView to generate data or image extracts.
 * This class is used to represent such a proxy. It simply provides convenience
 * API to make it easier to monitor an extractor proxy in the Qt code.
 */
class PQCORE_EXPORT pqExtractor : public pqProxy
{
  Q_OBJECT
  typedef pqProxy Superclass;

public:
  pqExtractor(const QString& group, const QString& name, vtkSMProxy* proxy, pqServer* server,
    QObject* parent = nullptr);
  ~pqExtractor() override;

  /**
   * Returns the producer that provides the data or rendering results to this
   * extractor. When non-null, the returned value may be a
   * pqOutputPort for generators producing data extracts or pqView for
   * generators producing image extracts.
   */
  pqServerManagerModelItem* producer() const;

  /**
   * Returns true if this extractor is an image extract producer.
   */
  bool isImageExtractor() const;

  /**
   * Returns true if this extractor is a data extractor.
   */
  bool isDataExtractor() const;

  /**
   * Returns true if this extractor is enabled.
   */
  bool isEnabled() const;

  /**
   * Convenience method to toggle enabled state. If `view` is non-null and the
   * extractor is an image-extractor, then the view must be same as the producer
   * for the extractor for this method to have any effect. For data-extractors,
   * view is simply ignored.
   */
  void toggleEnabledState(pqView* view);

Q_SIGNALS:
  //@{
  /**
   * Fired when the producer changes. These signals are fired after the change
   * has been made i.e. `this->producer()` will return the new producer for both
   * signals.
   */
  void producerAdded(pqServerManagerModelItem* producer, pqExtractor* self);
  void producerRemoved(pqServerManagerModelItem* producer, pqExtractor* self);
  //@}

  /**
   * Fired when the enabled state for the extractor changes.
   */
  void enabledStateChanged();

protected:
  void initialize() override;

private Q_SLOTS:
  void producerChanged();

private:
  Q_DISABLE_COPY(pqExtractor);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
