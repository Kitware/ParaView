/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContextView.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVContextView
 *
 * vtkPVContextView adopts vtkContextView so that it can be used in ParaView
 * configurations.
*/

#ifndef vtkPVContextView_h
#define vtkPVContextView_h

#include "vtkNew.h"                               // needed for vtkNew.
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVView.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.

class vtkAbstractContextItem;
class vtkChart;
class vtkChartRepresentation;
class vtkPVContextInteractorStyle;
class vtkContextView;
class vtkCSVExporter;
class vtkInformationIntegerKey;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkSelection;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVContextView : public vtkPVView
{
public:
  vtkTypeMacro(vtkPVContextView, vtkPVView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Triggers a high-resolution render.
   * \note CallOnAllProcesses
   */
  void StillRender() override;

  /**
   * Triggers a interactive render. Based on the settings on the view, this may
   * result in a low-resolution rendering or a simplified geometry rendering.
   * \note CallOnAllProcesses
   */
  void InteractiveRender() override;

  //@{
  /**
   * Get the context view.
   */
  vtkGetObjectMacro(ContextView, vtkContextView);
  //@}

  /**
   * Get the context item.
   */
  virtual vtkAbstractContextItem* GetContextItem() = 0;

  //@{
  /**
   * Set the interactor. Client applications must set the interactor to enable
   * interactivity. Note this method will also change the interactor styles set
   * on the interactor.
   */
  virtual void SetupInteractor(vtkRenderWindowInteractor*);
  vtkRenderWindowInteractor* GetInteractor();
  //@}

  /**
   * Representations can use this method to set the selection for a particular
   * representation. Subclasses override this method to pass on the selection to
   * the chart using annotation link. Note this is meant to pass selection for
   * the local process alone. The view does not manage data movement for the
   * selection.
   */
  virtual void SetSelection(vtkChartRepresentation* repr, vtkSelection* selection) = 0;

  /**
   * Get the current selection created in the view. This will call
   * this->MapSelectionToInput() to transform the selection every time a new
   * selection is available. Subclasses should override MapSelectionToInput() to
   * convert the selection, as appropriate.
   */
  vtkSelection* GetSelection();

  /**
   * Export the contents of this view using the exporter.
   * Called vtkChartRepresentation::Export() on all visible representations.
   * This is expected to called only on the client side after a render/update.
   * Thus all data is expected to available on the local process.
   */
  virtual bool Export(vtkCSVExporter* exporter);

  //@{
  /**
   * Get/Set the title.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  vtkSetStringMacro(Title);
  vtkGetStringMacro(Title);
  //@}

  //@{
  /**
   * Get/Set the font of the title.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  virtual void SetTitleFont(const char* family, int pointSize, bool bold, bool italic) = 0;
  virtual void SetTitleFontFamily(const char* family) = 0;
  virtual void SetTitleFontSize(int pointSize) = 0;
  virtual void SetTitleBold(bool bold) = 0;
  virtual void SetTitleItalic(bool italic) = 0;
  virtual void SetTitleFontFile(const char* file) = 0;
  virtual const char* GetTitleFontFamily() = 0;
  virtual int GetTitleFontSize() = 0;
  virtual int GetTitleFontBold() = 0;
  virtual int GetTitleFontItalic() = 0;
  //@}

  //@{
  /**
   * Get/Set the color of the title.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  virtual void SetTitleColor(double red, double green, double blue) = 0;
  virtual double* GetTitleColor() = 0;
  //@}

  //@{
  /**
   * Get/Set the alignement of the title.
   * These methods should not be called directly. They are made public only so
   * that the client-server-stream-interpreter can invoke them. Use the
   * corresponding properties to change these values.
   */
  virtual void SetTitleAlignment(int alignment) = 0;
  virtual int GetTitleAlignment() = 0;

protected:
  vtkPVContextView();
  ~vtkPVContextView() override;

  /**
   * Actual rendering implementation.
   */
  virtual void Render(bool interactive);

  /**
   * Called to transform the selection. This is only called on the client-side.
   * Subclasses should transform the selection in place as needed. Default
   * implementation simply goes to the first visible representation and asks it
   * to transform (by calling vtkChartRepresentation::MapSelectionToInput()).
   * We need to extend the infrastructrue to work properly when making
   * selections in views showing multiple representations, but until that
   * happens, this naive approach works for most cases.
   * Return false on failure.
   */
  virtual bool MapSelectionToInput(vtkSelection*);

  /**
   * Method to get the Formatted title after replacing some key strings
   * eg: ${TIME}
   * Child class should inherit this to add their own key strings
   */
  virtual std::string GetFormattedTitle();

  vtkContextView* ContextView;

private:
  vtkPVContextView(const vtkPVContextView&) = delete;
  void operator=(const vtkPVContextView&) = delete;

  char* Title = nullptr;

  // Used in GetSelection to avoid modifying the selection obtained from the
  // annotation link.
  vtkSmartPointer<vtkSelection> SelectionClone;
  vtkNew<vtkPVContextInteractorStyle> InteractorStyle;

  template <class T>
  vtkSelection* GetSelectionImplementation(T* chart);
};

#endif
