// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
  ///@{
  /**
   * Fired when the producer changes. These signals are fired after the change
   * has been made i.e. `this->producer()` will return the new producer for both
   * signals.
   */
  void producerAdded(pqServerManagerModelItem* producer, pqExtractor* self);
  void producerRemoved(pqServerManagerModelItem* producer, pqExtractor* self);
  ///@}

  /**
   * Fired when the enabled state for the extractor changes.
   */
  void enabledStateChanged();

protected:
  void initialize() override;

private Q_SLOTS:
  void producerChanged();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqExtractor);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
