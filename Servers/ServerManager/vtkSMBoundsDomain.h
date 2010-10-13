/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBoundsDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMBoundsDomain - double range domain based on data set bounds
// .SECTION Description
// vtkSMBoundsDomain is a subclass of vtkSMDoubleRangeDomain. In its Update
// method, it determines the minimum and maximum coordinates of each dimension
// of the bounding  box of the data set with which it is associated. It
// requires a vtkSMSourceProxy to do this.
// .SECTION See Also
// vtkSMDoubleRangeDomain

#ifndef __vtkSMBoundsDomain_h
#define __vtkSMBoundsDomain_h

#include "vtkSMDoubleRangeDomain.h"

class vtkPVDataInformation;
class vtkSMProxyProperty;

class VTK_EXPORT vtkSMBoundsDomain : public vtkSMDoubleRangeDomain
{
public:
  static vtkSMBoundsDomain* New();
  vtkTypeMacro(vtkSMBoundsDomain, vtkSMDoubleRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Update self checking the "unchecked" values of all required
  // properties. Overwritten by sub-classes.
  virtual void Update(vtkSMProperty*);

  // Description:
  // A vtkSMProperty is often defined with a default value in the
  // XML itself. However, many times, the default value must be determined
  // at run time. To facilitate this, domains can override this method
  // to compute and set the default value for the property.
  // Note that unlike the compile-time default values, the
  // application must explicitly call this method to initialize the
  // property.
  virtual int SetDefaultValues(vtkSMProperty*);

  // Description:
  vtkSetClampMacro(Mode, int, 0, 3);
  vtkGetMacro(Mode, int);

  // Description:
  void SetInputInformation(vtkPVDataInformation* input);

//BTX
  // Description:
  // SCALED_EXTENT: is used for vtkPVScaleFactorEntry.
  enum Modes
  {
    NORMAL,
    MAGNITUDE,
    ORIENTED_MAGNITUDE,
    SCALED_EXTENT
  };

  enum DefaultModes
    {
    MIN,
    MAX,
    MID
    };
//ETX

  vtkSetMacro(ScaleFactor, double);
  vtkGetMacro(ScaleFactor, double);

  vtkSetMacro(DefaultMode, int);
  vtkGetMacro(DefaultMode, int);
protected:
  vtkSMBoundsDomain();
  ~vtkSMBoundsDomain();

  void Update(vtkSMProxyProperty *pp);
  virtual void UpdateFromInformation(vtkPVDataInformation* information);
  void UpdateOriented();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);
  
  int Mode;
  int DefaultMode; // Used only in Normal mode while settings the default value.

  vtkPVDataInformation* InputInformation;

  double ScaleFactor; // Used only in SCALED_EXTENT mode.

  // Obtain the data information from the requried property with
  // function "Input", if any.
  vtkPVDataInformation* GetInputInformation();
private:
  vtkSMBoundsDomain(const vtkSMBoundsDomain&); // Not implemented
  void operator=(const vtkSMBoundsDomain&); // Not implemented

  void SetDomainValues(double bounds[6]);
};

#endif
