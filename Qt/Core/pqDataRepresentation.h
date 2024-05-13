// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDataRepresentation_h
#define pqDataRepresentation_h

#include "pqRepresentation.h"

class pqDataRepresentationInternal;
class pqOutputPort;
class pqPipelineSource;
class pqScalarsToColors;
class vtkPVArrayInformation;
class vtkPVDataInformation;
class vtkPVProminentValuesInformation;
class vtkPVTemporalDataInformation;

/**
 * pqDataRepresentation is the superclass for a display for a pqPipelineSource
 * i.e. the input for this display proxy is a pqPiplineSource.
 * This class manages the linking between the pqPiplineSource
 * and pqDataRepresentation.
 */
class PQCORE_EXPORT pqDataRepresentation : public pqRepresentation
{
  Q_OBJECT
  typedef pqRepresentation Superclass;

public:
  pqDataRepresentation(const QString& group, const QString& name, vtkSMProxy* display,
    pqServer* server, QObject* parent = nullptr);
  ~pqDataRepresentation() override;

  /**
   * Get the source/filter of which this is a display.
   */
  pqPipelineSource* getInput() const;

  /**
   * Returns the input pqPipelineSource's output port to which this
   * representation is connected.
   */
  pqOutputPort* getOutputPortFromInput() const;

  /**
   * Returns the data information for the data coming into the representation
   * as input.
   */
  vtkPVDataInformation* getInputDataInformation() const;

  /**
   * Returns the temporal data information for the input. This can be a very
   * slow process. Use with extreme caution!!!
   */
  vtkPVTemporalDataInformation* getInputTemporalDataInformation() const;

  /**
   * Returns rank-specific data information from input.
   */
  vtkPVDataInformation* getInputRankDataInformation(int rank) const;

  /**
   * Returns the represented data information. Depending on the representation
   * this may differ from the input data information eg. if the representation
   * shows an outline of the data, the this method will return the information
   * about the polydata forming the outline not the input dataset.
   */
  vtkPVDataInformation* getRepresentedDataInformation(bool update = true) const;

  /**
   * Get the data bounds for the input of this display.
   * Returns if the operation was successful.
   */
  bool getDataBounds(double bounds[6]);

  ///@{
  /**
   * Returns the lookuptable proxy, if any.
   * Most consumer displays take a lookup table. This method provides access to
   * the Lookup table, if one exists.
   *
   * @note the singular form of this method returns the first lookup table proxy
   */
  virtual std::vector<vtkSMProxy*> getLookupTableProxies(
    int selectedPropertiesType = 0 /*Representation*/) const;
  virtual vtkSMProxy* getLookupTableProxy(int selectedPropertiesType = 0 /*Representation*/) const;
  ///@}

  /**
   * Returns the pqScalarsToColors object for the lookup table proxy if any.
   * Most consumer displays take a lookup table. This method provides access to
   * the Lookup table, if one exists.
   *
   * @note returns the first lookup table
   */
  virtual pqScalarsToColors* getLookupTable(
    int selectedPropertiesType = 0 /*Representation*/) const;

  /**
   * Returns the data size for the full-res data.
   * This may trigger a pipeline update to obtain correct data sizes.
   */
  unsigned long getFullResMemorySize();

  /**
   * This is convenience method to return first representation for the
   * upstream stream filter/source in the same view as this representation.
   * This is only applicable, if this representation is connected to a
   * data-filter which has a valid input.
   */
  pqDataRepresentation* getRepresentationForUpstreamSource() const;

Q_SIGNALS:
  /**
   * Fired when the representation proxy fires the vtkCommand::UpdateDataEvent.
   */
  void dataUpdated();

  ///@{
  /**
   * Fired to indicate that the "LookupTable" property (if any) on the
   * representation was modified.
   */
  void colorTransferFunctionModified();
  void blockColorTransferFunctionModified();
  ///@}

  ///@{
  /**
   * Signal fired to indicate that the "ColorArrayName" property (if any) on
   * the representation was modified. This property controls the scalar
   * coloring settings on the representation.
   */
  void colorArrayNameModified();
  void blockColorArrayNameModified();
  ///@}

  /**
   * Signal fired to indicate that the rendering attribute arrays properties
   * (Normals, TCoords, Tangents) were modified.
   * These properties control the shading and texture mapping.
   */
  void attrArrayNameModified();

  /**
   * Signal fired to indicate the representation type has changed. ("Volume", "Surface", ..)
   */
  void representationTypeModified();

  /**
   * Signal fired to indicate the use separate opacity array property has changed.
   */
  void useSeparateOpacityArrayModified();

  /**
   * Signal fired to indicate the use 2D transfer function property has changed.
   */
  void useTransfer2DModified();

public Q_SLOTS:
  /**
   * Slot to update the lookup table if the application setting to
   * reset it on visibility changes is on.
   */
  virtual void updateLookupTable();

  virtual void resetAllTransferFunctionRangesUsingCurrentData();

  /**
   * Overridden to set the VisibilityChangedSinceLastUpdate flag.
   */
  void onVisibilityChanged() override;

protected Q_SLOTS:
  /**
   * called when input property on display changes. We must detect if
   * (and when) the display is connected to a new proxy.
   */
  virtual void onInputChanged();

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Use this method to initialize the pqObject state using the
   * underlying vtkSMProxy. This needs to be done only once,
   * after the object has been created.
   */
  void initialize() override
  {
    this->Superclass::initialize();
    this->onInputChanged();
  }

private:
  Q_DISABLE_COPY(pqDataRepresentation)

  pqDataRepresentationInternal* Internal;
};

#endif
