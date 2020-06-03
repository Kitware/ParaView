/*=========================================================================

   Program: ParaView
   Module:    pqDataRepresentation.h

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
    pqServer* server, QObject* parent = 0);
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

  /**
  * Returns the lookuptable proxy, if any.
  * Most consumer displays take a lookup table. This method
  * provides access to the Lookup table, if one exists.
  */
  virtual vtkSMProxy* getLookupTableProxy();

  /**
  * Returns the pqScalarsToColors object for the lookup table
  * proxy if any.
  * Most consumer displays take a lookup table. This method
  * provides access to the Lookup table, if one exists.
  */
  virtual pqScalarsToColors* getLookupTable();

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

  /**
  * Fired to indicate that the "LookupTable" property (if any) on the
  * representation was modified.
  */
  void colorTransferFunctionModified();

  /**
  * Signal fired to indicate that the "ColorArrayName" property (if any) on
  * the representation was modified. This property controls the scalar
  * coloring settings on the representation.
  */
  void colorArrayNameModified();

  /**
  * Signal fired to indicate that the rendering attribute arrays properties
  * (Normals, TCoords, Tangents) were modified.
  * These properties control the shading and texture mapping.
  */
  void attrArrayNameModified();

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

protected:
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
