/*=========================================================================

  Program:   ParaView
  Module:    vtkPythonView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPythonView
 *
 * vtkPythonView is a view for displaying data via a Python script
 * that uses matplotlib to generate a plot.
*/

#ifndef vtkPythonView_h
#define vtkPythonView_h

#include "vtkPVView.h"

#include "vtkImageData.h"                               // needed for member variable
#include "vtkPVClientServerCorePythonRenderingModule.h" //needed for exports
#include "vtkSmartPointer.h"                            //needed for member variables

class vtkImageData;
class vtkInformationRequestKey;
class vtkPythonRepresentation;
class vtkRenderer;
class vtkRenderWindow;
class vtkTexture;

class VTKPVCLIENTSERVERCOREPYTHONRENDERING_EXPORT vtkPythonView : public vtkPVView
{
public:
  static vtkPythonView* New();
  vtkTypeMacro(vtkPythonView, vtkPVView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * This is a pass for delivering data from the server nodes to the client.
   */
  static vtkInformationRequestKey* REQUEST_DELIVER_DATA_TO_CLIENT();

  /**
   * Overrides the base class method to request an addition pass that moves data from the
   * server to the client.
   */
  void Update() override;

  /**
   * Gets the renderer for this view.
   */
  virtual vtkRenderer* GetRenderer();

  // Sets the renderer for this view.
  virtual void SetRenderer(vtkRenderer* ren);

  //@{
  /**
   * Get/Set the Python script.
   */
  vtkSetStringMacro(Script);
  vtkGetStringMacro(Script);
  //@}

  //@{
  /**
   * Magnification is needed to inform Python of the requested size of the plot
   */
  vtkSetVector2Macro(Magnification, int);
  vtkGetVector2Macro(Magnification, int);
  //@}

  /**
   * Gets the number of visible data objects in the view.
   */
  int GetNumberOfVisibleDataObjects();

  /**
   * Get the representation for the visible data object at the given index.
   */
  vtkPythonRepresentation* GetVisibleRepresentation(int visibleObjectIndex);

  /**
   * Get a local copy of the visible data object at an index. The
   * index must be between 0 and this->GetNumberOfVisibleDataObjects().
   * If outside this range, returns NULL. This will return a shallow
   * copy of the data object input to the representation.

   * WARNING: this method is intended to be called only from within
   * the setup_data() function in the Python Script set for this instance.
   */

  vtkDataObject* GetVisibleDataObjectForSetup(int visibleObjectIndex);

  /**
   * Get the client's copy of the visible data object at an index. The
   * index must be between 0 and this->GetNumberOfVisibleDataObjects().
   * If outside this range, returns NULL.

   * WARNING: this method should be called only from within the render()
   * function in the Python Script set for this instance.
   */
  vtkDataObject* GetVisibleDataObjectForRendering(int visibleObjectIndex);

  /**
   * Get number of arrays in an attribute (e.g., vtkDataObject::POINT,
   * vtkDataObject::CELL, vtkDataObject::ROW, vtkDataObject::FIELD_DATA)
   * for the visible object at the given index.
   */
  int GetNumberOfAttributeArrays(int visibleObjectIndex, int attributeType);

  /**
   * From the visible object at the given index, get the name of
   * attribute array at index for the given attribute type.
   */
  const char* GetAttributeArrayName(int visibleObjectIndex, int attributeType, int arrayIndex);

  /**
   * Set the array status for the visible object at the given index. A
   * status of 1 means that the array with the given name for the
   * given attribute will be copied to the client. A status of 0 means
   * the array will not be copied to the client. The status is 0 by
   * default.
   */
  void SetAttributeArrayStatus(
    int visibleObjectIndex, int attributeType, const char* name, int status);

  /**
   * Get the status indicating whether the array with the given name
   * and attribute type in the visible object will be copied to the
   * client. Status is 0 by default.
   */
  int GetAttributeArrayStatus(int visibleObjectIndex, int attributeType, const char* name);

  /**
   * Enable all attribute arrays.
   */
  void EnableAllAttributeArrays();

  /**
   * Disable all attribute arrays.
   */
  void DisableAllAttributeArrays();

  void StillRender() override;

  void InteractiveRender() override;

  //@{
  /**
   * Set the vtkImageData that will be displayed. This is an internal
   * method meant only to be called from the python side, but must be
   * exposed to be wrapped.
   */
  vtkSetObjectMacro(ImageData, vtkImageData);
  //@}

protected:
  vtkPythonView();
  ~vtkPythonView() override;

  vtkSmartPointer<vtkTexture> RenderTexture;
  vtkSmartPointer<vtkRenderer> Renderer;

  // Needed to handle rendering at different magnifications
  int Magnification[2];

  /**
   * Is local data available?
   */
  bool IsLocalDataAvailable();

private:
  vtkPythonView(const vtkPythonView&) = delete;
  void operator=(const vtkPythonView&) = delete;

  class vtkInternals;
  vtkInternals* Internals;

  // The Python script
  char* Script;

  // The image data to be displayed in the view
  vtkImageData* ImageData;
};

#endif
