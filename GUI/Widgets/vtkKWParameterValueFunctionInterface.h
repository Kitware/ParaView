/*=========================================================================

  Module:    vtkKWParameterValueFunctionInterface.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWParameterValueFunctionInterface - a parameter/value function editor/interface
// .SECTION Description
// A widget that allows the user to edit a parameter/value function 
// interactively. This abstract class is the first abstract stage of 
// vtkKWParameterValueFunctionEditor, which in turns provides most of the 
// user-interface functionality.
// This class was created to take into account the amount of code and
// the complexity of vtkKWParameterValueFunctionEditor, most of which should
// not be a concern for developpers. 
// As a superclass it emphasizes and tries to document which pure virtual
// methods *needs* to be implemented in order to create an editor tailored
// for a specific kind of parameter/value function. It only describes 
// the low-level methods that are required to manipulate a function in 
// a user-interface independent way. For example, given the id (rank) of a
// point in the function, how to retrieve its corresponding parameter and/or
// value(s) ; how to retrieve the dimensionality of a point ; how to
// interpolate the value of a point over the parameter range, etc.
// Its subclass vtkKWParameterValueFunctionEditor uses those methods
// to create and manage a graphical editor, without concrete knowledge of
// what specific class the function relates to.
// The subclasses of vtkKWParameterValueFunctionEditor provide a concrete
// implementation of vtkKWParameterValueFunctionEditor by tying up a
// specific class of function to the methods below. For example, the class
// vtkKWPiecewiseFunctionEditor manipulates instances of vtkPiecewiseFunction
// internally as functions: the methods below are implemented as proxy to the
// vtkPiecewiseFunction methods.
// Same goes vtkKWColorTransferFunctionEditor, which manipulates instances of
// vtkColorTransferFunction internally.
// .SECTION See Also
// vtkKWParameterValueFunctionEditor vtkKWPiecewiseFunctionEditor vtkKWColorTransferFunctionEditor

#ifndef __vtkKWParameterValueFunctionInterface_h
#define __vtkKWParameterValueFunctionInterface_h

#include "vtkKWLabeledWidget.h"

class VTK_EXPORT vtkKWParameterValueFunctionInterface : public vtkKWLabeledWidget
{
public:
  vtkTypeRevisionMacro(vtkKWParameterValueFunctionInterface,vtkKWLabeledWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Return 1 if there is a function associated to the editor.
  // It is used, among *other* things, to disable the UI automatically if
  // there is nothing to edit at the moment.
  virtual int HasFunction() = 0;

  // Description:
  // Return the number of points/elements in the function
  virtual int GetFunctionSize() = 0;

protected:
  vtkKWParameterValueFunctionInterface() {};
  ~vtkKWParameterValueFunctionInterface() {};

  // Description:
  // This probably should not be hard-coded, but will make our life easier.
  // It specificies the maximum dimensionality of a point (not the *number*
  // of points). For example, for a RGB color transfer function editor, each
  // point has a dimensionality of 3 (see GetFunctionPointDimensionality).
  //BTX
  enum 
  {
    MaxFunctionPointDimensionality = 20
  };
  //ETX

  // *******************************************************************
  // The following methods are fast low-level manipulators: do *not* check if
  // points can added/removed or are locked, it is up to the higer-level
  // methods to do it (see vtkKWParameterValueFunctionEditor for example,
  // AddFunctionPointAtParameter() will use the low-level
  // FunctionPointCanBeRemoved() and RemoveFunctionPoint() below).
  // Points are usually accessed by 'id', which is pretty much its rank
  // in the function (i.e., the 0-th point is the first point and has id = 0,
  // the next point has id = 1, etc., up to GetFunctionSize() - 1)
  // *******************************************************************

  // Description:
  // Return the modification time of the function (a monotonically increasing
  // value changing each time the function is modified)
  virtual unsigned long GetFunctionMTime() = 0;

  // Description:
  // Get the 'parameter' at point 'id'
  // Ex: a scalar range, or a instant in a timeline.
  // Return 1 on success, 0 otherwise
  virtual int GetFunctionPointParameter(int id, double *parameter) = 0;

  // Description:
  // Get the dimensionality of the points in the function 
  // (Ex: 3 for a RGB point, 1 for a boolean value or an opacity value)
  virtual int GetFunctionPointDimensionality() = 0;

  // Description:
  // Get the 'n-tuple' value at point 'id' (where 'n' is the dimensionality of
  // the point). Note that 'values' has to be allocated with enough room.
  // Return 1 on success, 0 otherwise
  virtual int GetFunctionPointValues(int id, double *values) = 0;

  // Description:
  // Set the 'n-tuple' value at point 'id' (where 'n' is the dimensionality of
  // the point). Note that the point has to exist.
  // Return 1 on success, 0 otherwise
  virtual int SetFunctionPointValues(int id, const double *values) = 0;

  // Description:
  // Interpolate and get the 'n-tuple' value at a given 'parameter' (where 
  // 'n' is the dimensionality of the point). In other words, compute the 
  // value of a point as if it was located at a given parameter over the
  // parameter range of the function). Note that 'values' has to be allocated
  // with enough room. The interpolation method is function dependent
  // (linear in the vtkKWPiecewiseFunctionEditor class for example).
  // Return 1 on success, 0 otherwise
  virtual int InterpolateFunctionPointValues(double parameter,double *values)=0;
  // Description:
  // Add a 'n-tuple' value at a given 'parameter' over the parameter range 
  // (where 'n' is the dimensionality of the point), and return the 
  // corresponding point 'id' (the rank of the newly added point in the
  // function).
  // Return 1 on success, 0 otherwise
  virtual int AddFunctionPoint(double parameter,const double *values,int *id)=0;
  // Description:
  // Set the 'parameter' *and* 'n-tuple' value at point 'id' (where 'n' is the
  // dimensionality of the point). Note that the point has to exist.
  // It basically *moves* the point to a new location over the parameter range
  // and change its value simultaneously. Note that doing so should really
  // *not* change the rank/id of the point in the function, otherwise things
  // might go wrong (untested). Basically it means that points can
  // not be moved "over" other points, i.e. when you drag a point in the
  // editor, you can not move it "before" or "past" its neighbors, which makes
  // sense anyway (I guess), but make sure the constraint is enforced :)
  // Return 1 on success, 0 otherwise
  virtual int SetFunctionPoint(int id, double parameter, const double *values)=0;

  // Description:
  // Remove a function point 'id'.
  // Note: do not use FunctionPointCanBeRemoved() inside that function, it
  // has been done for you already in higher-level methods.
  // Return 1 on success, 0 otherwise
  virtual int RemoveFunctionPoint(int id) = 0;

  // *******************************************************************
  // The following low-level methods can be reimplemented, but a default 
  // implementation is provided by vtkKWParameterValueFunctionEditor and
  // is working just fine. If you have to reimplement them, make sure to
  // call the corresponding superclass method too (or have a good reason
  // not to). Those methods are used by high-level methods, and should 
  // not be called from the other low-level methods described above 
  // (see vtkKWParameterValueFunctionEditor for example, the high-level
  // AddFunctionPointAtParameter() method will use the low-level
  // below FunctionPointCanBeRemoved() and above RemoveFunctionPoint()).
  // *******************************************************************

  // Description:
  // Return 1 if a point can be added to the function, 0 otherwise.
  // Ex: there might be many reasons why a function could be "locked", it
  // depends on your implementation, but here is the hook.
  virtual int FunctionPointCanBeAdded() = 0;

  // Description:
  // Return 1 if the point 'id' can be removed from the function, 0 otherwise.
  virtual int FunctionPointCanBeRemoved(int id) = 0;

  // Description:
  // Return 1 if the 'parameter' of the point 'id' is locked (can/should 
  // not be changed/edited), 0 otherwise.
  virtual int FunctionPointParameterIsLocked(int id) = 0;

  // Description:
  // Return 1 if the 'n-tuple' value of the point 'id' is locked (can/should 
  // not be changed/edited), 0 otherwise.
  // Note that by default point with dimensionality > 1 will be placed in the
  // center of the editor, as the is no way to edit a n-dimensional point
  // in a 2D editor. Still, some editors (see vtkKWColorTransferFunctionEditor
  // will provide 3 text entries to allow the point value(s) to be edited).
  virtual int FunctionPointValueIsLocked(int id) = 0;

  // Description:
  // Return 1 if the point 'id' can be moved over the parameter range to a
  // new 'parameter', 0 otherwise. 
  // vtkKWParameterValueFunctionEditor provides a default implementation
  // preventing the point to be moved outside the parameter range, or
  // if the parameter is locked, or if it is passing over or before its
  // neighbors.
  virtual int FunctionPointCanBeMovedToParameter(int id, double parameter) = 0;

private:
  vtkKWParameterValueFunctionInterface(const vtkKWParameterValueFunctionInterface&); // Not implemented
  void operator=(const vtkKWParameterValueFunctionInterface&); // Not implemented
};

#endif
