// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSignalAdaptorKeyFrameType_h
#define pqSignalAdaptorKeyFrameType_h

#include "pqComponentsModule.h"
#include "pqSignalAdaptors.h"

class vtkSMProxy;
class pqPropertyLinks;
class QLabel;
class pqKeyFrameTypeWidget;

/**
 * pqSignalAdaptorKeyFrameType is adaptor for the "Type" property of the
 * vtkSMCompositeKeyFrameProxy. For certain types of keyframes,
 * we have additional properties that must be exposed in the GUI,
 * this class manages that.
 * Use this to connect a QComboBox with type listing
 * to the Type property on the proxy.
 */
class PQCOMPONENTS_EXPORT pqSignalAdaptorKeyFrameType : public pqSignalAdaptorComboBox
{
  Q_OBJECT
public:
  /**
   * Constructor.
   * \c combo is the combo-box that chooses the type, while frame is
   * the frame in which this adaptor can put additional widgets, if needed.
   * \c valueLabel is the label that is used for the keyframe value,
   * since based on the type  the value label may change.
   * Note that this class will toggle the visibility of this frame as needed.
   */
  pqSignalAdaptorKeyFrameType(
    pqKeyFrameTypeWidget* widget, pqPropertyLinks* links, QLabel* valueLabel = nullptr);
  ~pqSignalAdaptorKeyFrameType() override;

  /**
   * \c keyframe is the proxy for the key frame. It typically is
   * a vtkSMCompositeKeyFrameProxy or subclass.
   * This is needed to setup the links with the other properties.
   */
  void setKeyFrameProxy(vtkSMProxy* keyframe);
  vtkSMProxy* getKeyFrameProxy() const;

private Q_SLOTS:
  /**
   * Called when the combo-box changes
   */
  void onTypeChanged();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqSignalAdaptorKeyFrameType)

  class pqInternals;
  pqInternals* Internals;
};

#endif
