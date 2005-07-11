#include "vtkImageViewer2.h"
#include "vtkKWApplication.h"
#include "vtkKWHeaderAnnotationEditor.h"
#include "vtkKWFrame.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWWindow.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkXMLImageDataReader.h"

#include "KWWidgetsTourExampleTypes.h"

class vtkKWHeaderAnnotationEditorItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeVTK; };
  vtkKWHeaderAnnotationEditorItem(vtkKWWidget *parent, vtkKWWindow *);
  virtual ~vtkKWHeaderAnnotationEditorItem();

protected:
  vtkKWRenderWidget *hae_renderwidget;
  vtkXMLImageDataReader *hae_reader;
  vtkImageViewer2 *hae_viewer;
  vtkRenderWindowInteractor *hae_iren;
  vtkKWHeaderAnnotationEditor *hae_anno_editor;
};

KWWidgetsTourItem* vtkKWHeaderAnnotationEditorEntryPoint(vtkKWWidget *parent, vtkKWWindow *window)
{
  return new vtkKWHeaderAnnotationEditorItem(parent, window);
}

vtkKWHeaderAnnotationEditorItem::vtkKWHeaderAnnotationEditorItem(
  vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a render widget
  // Set the header annotation visibility and set some text

  this->hae_renderwidget = vtkKWRenderWidget::New();
  this->hae_renderwidget->SetParent(parent);
  this->hae_renderwidget->Create(app);

  this->hae_renderwidget->HeaderAnnotationVisibilityOn();
  this->hae_renderwidget->SetHeaderAnnotationText("Hello, World!");

  app->Script("pack %s -side right -fill both -expand y -padx 0 -pady 0", 
              this->hae_renderwidget->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create a volume reader

  this->hae_reader = vtkXMLImageDataReader::New();
  this->hae_reader->SetFileName(
    KWWidgetsTourItem::GetPathToExampleData(app, "head100x100x47.vti"));

  // Create an image viewer
  // Use the render window and renderer of the renderwidget

  this->hae_viewer = vtkImageViewer2::New();
  this->hae_viewer->SetRenderWindow(this->hae_renderwidget->GetRenderWindow());
  this->hae_viewer->SetRenderer(this->hae_renderwidget->GetRenderer());
  this->hae_viewer->SetInput(this->hae_reader->GetOutput());

  this->hae_iren = vtkRenderWindowInteractor::New();
  this->hae_viewer->SetupInteractor(this->hae_iren);

  this->hae_renderwidget->ResetCamera();

  // -----------------------------------------------------------------------

  // Create a header annotation editor
  // Connect it to the render widget
  
  this->hae_anno_editor = vtkKWHeaderAnnotationEditor::New();
  this->hae_anno_editor->SetParent(parent);
  this->hae_anno_editor->Create(app);
  this->hae_anno_editor->SetRenderWidget(this->hae_renderwidget);

  app->Script("pack %s -side left -anchor nw -expand n -padx 2 -pady 2", 
              this->hae_anno_editor->GetWidgetName());
}

vtkKWHeaderAnnotationEditorItem::~vtkKWHeaderAnnotationEditorItem()
{
  this->hae_anno_editor->Delete();
  this->hae_reader->Delete();
  this->hae_iren->Delete();
  this->hae_renderwidget->Delete();
  this->hae_viewer->Delete();
}
