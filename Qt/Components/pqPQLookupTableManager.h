/*=========================================================================

   Program: ParaView
   Module:    pqPQLookupTableManager.h

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
#ifndef __pqPQLookupTableManager_h
#define __pqPQLookupTableManager_h

#include "pqLookupTableManager.h"
#include "pqComponentsExport.h"

class vtkSMProxy;

/// pqPQLookupTableManager is an implementation specific to ParaView.
/// A lookup table is shared among all arrays with same name and
/// same number of components.
class PQCOMPONENTS_EXPORT pqPQLookupTableManager : public pqLookupTableManager
{
  Q_OBJECT
public:
  pqPQLookupTableManager(QObject* parent=0);
  virtual ~pqPQLookupTableManager();

  /// Get a LookupTable for the array with name \c arrayname
  /// and component. component = -1 represents magnitude.
  /// This subclass associates a LUT with arrayname:component
  /// pair. If  none exists, a new one will be created.
  virtual pqScalarsToColors* getLookupTable(pqServer* server, const QString& arrayname,
    int number_of_components, int component);
    
  /// Returns the pqScalarOpacityFunction object for the piecewise
  /// function used to map scalars to opacity.
  virtual pqScalarOpacityFunction* getScalarOpacityFunction(pqServer* server, 
    const QString& arrayname, int number_of_components, int component);

  /// Saves the state of the lut/opacity-function so that the next time a new 
  /// LUT/opacity-function is created, it
  /// will have the same state as this one.
  virtual void saveLUTAsDefault(pqScalarsToColors*);
  virtual void saveOpacityFunctionAsDefault(pqScalarOpacityFunction*);

  /// save the state of the scalar bar, so that the next time a new scalar bar
  /// is created its properties are setup using the defaults specified.
  virtual void saveScalarBarAsDefault(pqScalarBarRepresentation*);

  /// Used to get the array the \c lut is associated with.
  /// Return false if no such association exists.
  virtual bool getLookupTableProperties(pqScalarsToColors* lut,
    QString& arrayname, int &numComponents, int &component);

  /// Setting key used to save the default lookup table.
  static const char* DEFAULT_LOOKUPTABLE_SETTING_KEY()
    {
    return "/lookupTable/DefaultLUT";
    }
    
  /// Setting key used to save the default opacity function.
  static const char* DEFAULT_OPACITYFUNCTION_SETTING_KEY()
    {
    return "/lookupTable/DefaultOpacity";
    }

  // Setting key used to save the default scalar bar
  static const char* DEFAULT_SCALARBAR_SETTING_KEY()
    {
    return "/lookupTable/DefaultScalarBar";
    }

public slots:
  /// Called to update scalar ranges of all lookup tables.
  virtual void updateLookupTableScalarRanges();

protected:
  /// Called when a new LUT pq object is created.
  /// This happens as a result of either the GUI or python
  /// registering a LUT proxy.
  virtual void onAddLookupTable(pqScalarsToColors* lut);

  /// Called when a LUT is removed.
  virtual void onRemoveLookupTable(pqScalarsToColors* lut);

  /// set default property values for LUT.
  void setLUTDefaultState(vtkSMProxy* lut);
  
  /// Called when a new ScalarOpacityFunction pq object is created.
  /// This happens as a result of either the GUI or python
  /// registering a ScalarOpacityFunction proxy.
  virtual void onAddOpacityFunction(pqScalarOpacityFunction* opFunc);
  
  /// Called when a ScalarOpacityFunction is removed.
  virtual void onRemoveOpacityFunction(pqScalarOpacityFunction* opFunc);
  
  /// set default property values for ScalarOpacityFunction.
  void setOpacityFunctionDefaultState(vtkSMProxy* opFunc);

  /// creates a new LUT.
  pqScalarsToColors* createLookupTable(pqServer* server,
    const QString& arrayname, int number_of_components, int component);
    
  /// Returns the proxy for the piecewise function used to
  /// map scalars to opacity.
  pqScalarOpacityFunction* createOpacityFunction(pqServer* server,
    const QString& arrayname, int number_of_components, int component);

  /// called when a new scalar is created so that subclasses have a change to
  /// change the default values as needed.
  virtual void initialize(pqScalarBarRepresentation*);
  
private:
  pqPQLookupTableManager(const pqPQLookupTableManager&); // Not implemented.
  void operator=(const pqPQLookupTableManager&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif
