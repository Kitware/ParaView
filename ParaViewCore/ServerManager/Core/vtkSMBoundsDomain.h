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
/**
 * @class   vtkSMBoundsDomain
 * @brief   double range domain based on data set bounds
 *
 * vtkSMBoundsDomain extends vtkSMDoubleRangeDomain to add support to determine
 * the valid range for the values based on the dataset bounds. There are several
 * \c Modes which can be used to control how the range is computed based on the
 * data bounds (defined by the vtkSMBoundsDomain::Modes enum).
 * \li \c NORMAL : this is the basic mode where the domain will have 3 ranges which
 * are the min and max for the bounds along each of the coordinate axis.
 * \li \c MAGNITUDE: the domain has a single range set to (-magn/2.0, +magn/2.0)
 * where magn is the magnitude of the diagonal.
 * \li \c ORIENTED_MAGNITUDE:  same as MAGNITUDE, but instead of the diagonal, a
 * vector determined using two additional required properties with functions
 * Normal, and Origin is used.
 * \li \c COMPONENT_MAGNITUDE: similar to MAGNITUDE except suitable for 3
 * component properties where each component is given range for the
 * corresponding axis i.e. `(0, xmax-xmin), (0, ymax-ymin), (0, zmax-zmin)`.
 * \li \c SCALED_EXTENT: the range is set to (0, maxbounds * this->ScaleFactor)
 * where maxbounds is the length of the longest axis for the bounding box.
 * \li \c ARRAY_SCALED_EXTENT: the range is set to (0, (arrayMagnitude / maxbounds) *
 * this->ScaleFactor)
 * where maxbounds is the length of the longest axis for the bounding box.
 * and arrayMagnitude the maximum magnitude of the array.
 * \li \c APPROXIMATE_CELL_LENGTH: approximation for cell length computed using the
 * \li \c DATA_BOUNDS: this mode for a 6 tuple property that takes the data
 * bounds. The range will have 6 ranges:
 * (xmin,xmax), (xmin,xmax), (ymin,ymax), (ymin,ymax), (zmin,zmax), and (zmin,zmax).
 * If default_mode is not specified, then "min,max,min,max,min,max" is assumed.
 * \li \c EXTENTS: this mode for a property that takes a value between 0 and (max-min) for
 * each component.
 *
 * To determine the input data bounds, this domain depends on a required
 * property with function \c Input. The data-information from the source-proxy
 * set as the value for that property is used to determine the bounds.
 *
 * Supported XML attributes:
 * \li \c mode : used to specify the Mode. Value can be "normal", "magnitude",
 * "oriented_magnitude", "scaled_extent", "array_scaled_extent", or "approximate_cell_length",
 * "data_bounds".
 * \li \c scale_factor : used in SCALED_EXTENT, ARRAY_SCALED_EXTENT and APPROXIMATE_CELL_LENGTH
 * mode.
 * Value is a floating point number that is used as the scale factor.
*/

#ifndef vtkSMBoundsDomain_h
#define vtkSMBoundsDomain_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMDoubleRangeDomain.h"

class vtkPVDataInformation;
class vtkSMProxyProperty;
class vtkSMArrayRangeDomain;
class vtkSMIntVectorProperty;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMBoundsDomain : public vtkSMDoubleRangeDomain
{
public:
  static vtkSMBoundsDomain* New();
  vtkTypeMacro(vtkSMBoundsDomain, vtkSMDoubleRangeDomain);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Update self checking the "unchecked" values of all required
   * properties. Overwritten by sub-classes.
   */
  void Update(vtkSMProperty*) override;

  //@{
  vtkSetClampMacro(Mode, int, 0, 3);
  vtkGetMacro(Mode, int);
  //@}

  /**
   * SCALED_EXTENT: is used for vtkPVScaleFactorEntry.
   */
  enum Modes
  {
    NORMAL,
    MAGNITUDE,
    ORIENTED_MAGNITUDE,
    SCALED_EXTENT,
    ARRAY_SCALED_EXTENT,
    APPROXIMATE_CELL_LENGTH,
    DATA_BOUNDS,
    EXTENTS,
    COMPONENT_MAGNITUDE,
  };

  vtkGetMacro(ScaleFactor, double);

  enum Axes
  {
    X_AXIS = 1,
    Y_AXIS = 2,
    Z_AXIS = 4,
    X_AND_Y_AXES = X_AXIS | Y_AXIS,
    Y_AND_Z_AXES = Y_AXIS | Z_AXIS,
    X_AND_Z_AXES = X_AXIS | Z_AXIS,
    X_Y_AND_Z_AXES = X_AXIS | Y_AXIS | Z_AXIS
  };

  vtkGetMacro(AxisFlags, int);

  /**
   * Overridden to handle APPROXIMATE_CELL_LENGTH.
   */
  int SetDefaultValues(vtkSMProperty* property, bool use_unchecked_values) override;

protected:
  vtkSMBoundsDomain();
  ~vtkSMBoundsDomain() override;

  /**
   * Set the appropriate ivars from the xml element. Should
   * be overwritten by subclass if adding ivars.
   */
  int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element) override;

  // Obtain the data information from the required property with
  // function "Input", if any.
  vtkPVDataInformation* GetInputInformation();

  // Obtain the information for the "AxisFlags" required property.
  vtkSMIntVectorProperty* GetAxisFlagsInformation();

  bool IsAxisEnabled(int axis);

  void SetDomainValues(double bounds[6]);

  void UpdateOriented();

  int Mode;
  double ScaleFactor; // Used only in SCALED_EXTENT and APPROXIMATE_CELL_LENGTH mode.
  int AxisFlags;

private:
  vtkSMBoundsDomain(const vtkSMBoundsDomain&) = delete;
  void operator=(const vtkSMBoundsDomain&) = delete;

  vtkSMArrayRangeDomain* ArrayRangeDomain;
};

#endif
