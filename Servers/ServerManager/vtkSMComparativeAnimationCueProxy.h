/*=========================================================================

  Program:   ParaView
  Module:    vtkSMComparativeAnimationCueProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMComparativeAnimationCueProxy - cue used for parameter animation by
// the comparative view.
// .SECTION Description
// vtkSMComparativeAnimationCueProxy is a animation cue used for parameter
// animation by the vtkSMComparativeViewProxy. It provides a non-conventional
// API i.e. without using properties to allow the user to setup parameter
// values over the comparative grid.

#ifndef __vtkSMComparativeAnimationCueProxy_h
#define __vtkSMComparativeAnimationCueProxy_h

#include "vtkSMAnimationCueProxy.h"

class VTK_EXPORT vtkSMComparativeAnimationCueProxy : public vtkSMAnimationCueProxy
{
public:
  static vtkSMComparativeAnimationCueProxy* New();
  vtkTypeMacro(vtkSMComparativeAnimationCueProxy, vtkSMAnimationCueProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Methods use to fill up the values for the parameter over the comparative
  // grid. These are order dependent methods i.e. the result of calling
  // UpdateXRange() and then UpdateYRange() are different from calling
  // UpdateYRange() and then UpdateXRange().
  // These methods are convenience methods when the value can only be a single
  // value.
  void UpdateXRange(int y, double minx, double maxx)
    { this->UpdateXRange(y, &minx, &maxx, 1); }
  void UpdateYRange(int x, double miny, double maxy)
    { this->UpdateYRange(x, &miny, &maxy, 1); }
  void UpdateWholeRange(double mint, double maxt)
    { this->UpdateWholeRange(&mint, &maxt, 1); }
  void UpdateValue(int x, int y, double value)
    { this->UpdateValue(x, y, &value, 1); }

  // Description:
  // Use these methods when the parameter can have multiple values eg. IsoValues
  // for the Contour filter. The "AnimatedElement" for such properties must be
  // set to -1, otherwise UpdateAnimatedValue() will raise an error. 
  void UpdateXRange(int y, double *minx, double* maxx, unsigned int numvalues);
  void UpdateYRange(int x, double *minx, double* maxx, unsigned int numvalues);
  void UpdateWholeRange(double *mint, double *maxt, unsigned int numValues)
    {
    this->UpdateWholeRange(mint, maxt, numValues, false);
    }
  void UpdateWholeRange(double *mint, double *maxt, unsigned int numValues,
    bool vertical_first);
  void UpdateValue(int x, int y, double *value, unsigned int numValues);
 
  // Description:
  // Update the animated property's value based on those specified using the
  // Update.* methods. (x,y) is the location in the comparative grid, while
  // (dx, dy) are the dimensions of the comparative grid.
  void UpdateAnimatedValue(int x, int y, int dx, int dy);

  // Description:
  // Computes the value for a particular location in the comparative grid.
  // (x,y) is the location in the comparative grid, while
  // (dx, dy) are the dimensions of the comparative grid.
  double GetValue(int x, int y, int dx, int dy)
    {
    unsigned int numValues=0;
    double* vals = this->GetValues(x, y, dx, dy, numValues);
    if (numValues > 0)
      {
      return vals[0];
      }
    return -1.0;
    }

  // Description:
  // NOTE: Returned values is only valid until the next call to this method.
  // Return value is only valid when numValues > 0.
  double* GetValues(int x, int y, int dx, int dy, unsigned int &numValues);

  // Description:
  // Saves the state of the proxy. 
  virtual vtkPVXMLElement* SaveState(vtkPVXMLElement* root)
    { return this->Superclass::SaveState(root); }

  // Description:
  // Saves the state of the proxy.
  // Overridden to add state for this proxy.
  virtual vtkPVXMLElement* SaveState(
    vtkPVXMLElement* root, vtkSMPropertyIterator *iter, int saveSubProxies);

  // Description:
  // Loads the proxy state from the XML element. Returns 0 on failure.
  // \c locator is used to locate other proxies that may be referred to in the
  // state XML (which happens in case of properties of type vtkSMProxyProperty
  // or subclasses). If locator is NULL, then such properties are left
  // unchanged.
  virtual int LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* locator);

  // Description:
  // Same as LoadState except that the proxy will try to undo the changes
  // recorded in the state. 
  virtual int RevertState(vtkPVXMLElement* element, vtkSMProxyLocator* locator);

//BTX
protected:
  vtkSMComparativeAnimationCueProxy();
  ~vtkSMComparativeAnimationCueProxy();

  double* Values;
private:
  vtkSMComparativeAnimationCueProxy(const vtkSMComparativeAnimationCueProxy&); // Not implemented
  void operator=(const vtkSMComparativeAnimationCueProxy&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif

