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
// vtkSMBoundsDomain extends vtkSMDoubleRangeDomain to add support to determine
// the valid range for the values based on the dataset bounds. There are several
// \c Modes which can be used to control how the range is computed based on the
// data bounds (defined by the vtkSMBoundsDomain::Modes enum).
// \li \c NORMAL : this is the basic mode where the domain will have 3 ranges which
// are the min and max for the bounds along each of the coordinate axis.
// \li \c MAGNITUDE: the domain has a single range set to (-magn/2.0, +magn/2.0)
// where magn is the magnitude of the diagonal.
// \li \c ORIENTED_MAGNITUDE:  same as MAGNITUDE, but instead of the dialog, a
// vector determined using two additional required properties with functions
// Normal, and Origin is used.
// li \c SCALED_EXTENT: the range is set to (0, maxbounds * this->ScaleFactor)
// where maxbounds is the length of the longest axis for the bounding box.
// li \c APPROXIMATE_CELL_LENGTH: approximation for cell length computed using the
// expression (diameter/ (cube_root(numCells)) * this->ScaleFactor.
//
// To determine the input data bounds, this domain depends on a required
// property with function \c Input. The data-information from the source-proxy
// set as the value for that property is used to determine the bounds.
//
// Supported XML attributes:
// \li \c mode : used to specify the Mode. Value can be "normal", "magnitude",
// "oriented_magnitude", "scaled_extent", or "approximate_cell_length".
// \li \c scale_factor : used in SCALED_EXTENT and APPROXIMATE_CELL_LENGTH mode.
// Value is a floating point number that is used as the scale factor.

#ifndef __vtkSMBoundsDomain_h
#define __vtkSMBoundsDomain_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMDoubleRangeDomain.h"

class vtkPVDataInformation;
class vtkSMProxyProperty;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMBoundsDomain : public vtkSMDoubleRangeDomain
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
  vtkSetClampMacro(Mode, int, 0, 3);
  vtkGetMacro(Mode, int);

  // Description:
  // SCALED_EXTENT: is used for vtkPVScaleFactorEntry.
  enum Modes
  {
    NORMAL,
    MAGNITUDE,
    ORIENTED_MAGNITUDE,
    SCALED_EXTENT,
    APPROXIMATE_CELL_LENGTH
  };

  vtkGetMacro(ScaleFactor, double);

  // Description:
  //  Overridden to handle APPROXIMATE_CELL_LENGTH.
  virtual int SetDefaultValues(vtkSMProperty* property);

protected:
  vtkSMBoundsDomain();
  ~vtkSMBoundsDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  // Obtain the data information from the requried property with
  // function "Input", if any.
  vtkPVDataInformation* GetInputInformation();

  void SetDomainValues(double bounds[6]);

  void UpdateOriented();

  int Mode;
  double ScaleFactor; // Used only in SCALED_EXTENT and APPROXIMATE_CELL_LENGTH mode.
private:
  vtkSMBoundsDomain(const vtkSMBoundsDomain&); // Not implemented
  void operator=(const vtkSMBoundsDomain&); // Not implemented
};

#endif
