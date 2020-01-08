/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVComparativeAnimationCue
 * @brief   cue used for parameter animation by the
 * comparative view.
 *
 * vtkPVComparativeAnimationCue is a animation cue used for parameter
 * animation by the ComparativeView. It provides a non-conventional
 * API i.e. without using properties to allow the user to setup parameter
 * values over the comparative grid.
*/

#ifndef vtkPVComparativeAnimationCue_h
#define vtkPVComparativeAnimationCue_h

#include "vtkObject.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class vtkSMDomain;
class vtkSMProperty;
class vtkSMProxy;
class vtkPVXMLElement;

class VTKREMOTINGVIEWS_EXPORT vtkPVComparativeAnimationCue : public vtkObject
{
public:
  static vtkPVComparativeAnimationCue* New();
  vtkTypeMacro(vtkPVComparativeAnimationCue, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set/Get the animated proxy.
   */
  void SetAnimatedProxy(vtkSMProxy*);
  vtkGetObjectMacro(AnimatedProxy, vtkSMProxy);
  void RemoveAnimatedProxy();
  //@}

  //@{
  /**
   * Set/Get the animated property name.
   */
  vtkSetStringMacro(AnimatedPropertyName);
  vtkGetStringMacro(AnimatedPropertyName);
  //@}

  //@{
  /**
   * Set/Get the animated domain name.
   */
  vtkSetStringMacro(AnimatedDomainName);
  vtkGetStringMacro(AnimatedDomainName);
  //@}

  //@{
  /**
   * The index of the element of the property this cue animates.
   * If the index is -1, the cue will animate all the elements
   * of the animated property.
   */
  vtkSetMacro(AnimatedElement, int);
  vtkGetMacro(AnimatedElement, int);
  //@}

  //@{
  /**
   * Enable/Disable the cue.
   */
  vtkSetMacro(Enabled, bool);
  vtkGetMacro(Enabled, bool);
  vtkBooleanMacro(Enabled, bool);
  //@}

  /**
   * Methods use to fill up the values for the parameter over the comparative
   * grid. These are order dependent methods i.e. the result of calling
   * UpdateXRange() and then UpdateYRange() are different from calling
   * UpdateYRange() and then UpdateXRange().
   * These methods are convenience methods when the value can only be a single
   * value.
   */
  void UpdateXRange(int y, double minx, double maxx) { this->UpdateXRange(y, &minx, &maxx, 1); }
  void UpdateYRange(int x, double miny, double maxy) { this->UpdateYRange(x, &miny, &maxy, 1); }
  void UpdateWholeRange(double mint, double maxt) { this->UpdateWholeRange(&mint, &maxt, 1); }
  void UpdateValue(int x, int y, double value) { this->UpdateValue(x, y, &value, 1); }

  //@{
  /**
   * Use these methods when the parameter can have multiple values eg. IsoValues
   * for the Contour filter. The "AnimatedElement" for such properties must be
   * set to -1, otherwise UpdateAnimatedValue() will raise an error.
   */
  void UpdateXRange(int y, double* minx, double* maxx, unsigned int numvalues);
  void UpdateYRange(int x, double* minx, double* maxx, unsigned int numvalues);
  void UpdateWholeRange(double* mint, double* maxt, unsigned int numValues)
  {
    this->UpdateWholeRange(mint, maxt, numValues, false);
  }
  void UpdateWholeRange(double* mint, double* maxt, unsigned int numValues, bool vertical_first);
  void UpdateValue(int x, int y, double* value, unsigned int numValues);
  //@}

  /**
   * Update the animated property's value based on those specified using the
   * Update.* methods. (x,y) is the location in the comparative grid, while
   * (dx, dy) are the dimensions of the comparative grid.
   */
  void UpdateAnimatedValue(int x, int y, int dx, int dy);

  //@{
  /**
   * Computes the value for a particular location in the comparative grid.
   * (x,y) is the location in the comparative grid, while
   * (dx, dy) are the dimensions of the comparative grid.
   */
  double GetValue(int x, int y, int dx, int dy)
  {
    unsigned int numValues = 0;
    double* vals = this->GetValues(x, y, dx, dy, numValues);
    if (numValues > 0)
    {
      return vals[0];
    }
    return -1.0;
  }
  //@}

  /**
   * NOTE: Returned values is only valid until the next call to this method.
   * Return value is only valid when numValues > 0.
   */
  double* GetValues(int x, int y, int dx, int dy, unsigned int& numValues);

  vtkPVXMLElement* AppendCommandInfo(vtkPVXMLElement* proxyElem);
  int LoadCommandInfo(vtkPVXMLElement* proxyElement);

protected:
  vtkPVComparativeAnimationCue();
  ~vtkPVComparativeAnimationCue() override;

  /**
   * Get the property being animated.
   */
  vtkSMProperty* GetAnimatedProperty();

  /**
   * Get the domain being animated.
   */
  vtkSMDomain* GetAnimatedDomain();

  vtkSMProxy* AnimatedProxy;
  int AnimatedElement;
  char* AnimatedPropertyName;
  char* AnimatedDomainName;
  double* Values;
  bool Enabled;

private:
  vtkPVComparativeAnimationCue(const vtkPVComparativeAnimationCue&) = delete;
  void operator=(const vtkPVComparativeAnimationCue&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
};

#endif
