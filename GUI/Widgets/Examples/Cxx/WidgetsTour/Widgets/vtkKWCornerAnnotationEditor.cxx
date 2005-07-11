#include "vtkImageViewer2.h"
#include "vtkKWApplication.h"
#include "vtkKWCornerAnnotationEditor.h"
#include "vtkKWFrame.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWWindow.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkXMLImageDataReader.h"
#include "vtkCornerAnnotation.h"

#include "KWWidgetsTourExampleTypes.h"

class vtkKWCornerAnnotationEditorItem : public KWWidgetsTourItem
{
public:
  virtual int GetType() { return KWWidgetsTourItem::TypeVTK; };
  vtkKWCornerAnnotationEditorItem(vtkKWWidget *parent, vtkKWWindow *);
  virtual ~vtkKWCornerAnnotationEditorItem();

protected:
  vtkKWRenderWidget *cae_renderwidget;
  vtkXMLImageDataReader *cae_reader;
  vtkImageViewer2 *cae_viewer;
  vtkRenderWindowInteractor *cae_iren;
  vtkKWCornerAnnotationEditor *cae_anno_editor;
};

KWWidgetsTourItem* vtkKWCornerAnnotationEditorEntryPoint(vtkKWWidget *parent, vtkKWWindow *window)
{
  return new vtkKWCornerAnnotationEditorItem(parent, window);
}

vtkKWCornerAnnotationEditorItem::vtkKWCornerAnnotationEditorItem(
  vtkKWWidget *parent, vtkKWWindow *)
{
  vtkKWApplication *app = parent->GetApplication();

  // -----------------------------------------------------------------------

  // Create a render widget
  // Set the corner annotation visibility

  this->cae_renderwidget = vtkKWRenderWidget::New();
  this->cae_renderwidget->SetParent(parent);
  this->cae_renderwidget->Create(app);
  this->cae_renderwidget->CornerAnnotationVisibilityOn();

  app->Script("pack %s -side right -fill both -expand y -padx 0 -pady 0", 
              this->cae_renderwidget->GetWidgetName());

  // -----------------------------------------------------------------------

  // Create a volume reader

  this->cae_reader = vtkXMLImageDataReader::New();
  this->cae_reader->SetFileName(
    KWWidgetsTourItem::GetPathToExampleData(app, "head100x100x47.vti"));

  // Create an image viewer
  // Use the render window and renderer of the renderwidget

  this->cae_viewer = vtkImageViewer2::New();
  this->cae_viewer->SetRenderWindow(this->cae_renderwidget->GetRenderWindow());
  this->cae_viewer->SetRenderer(this->cae_renderwidget->GetRenderer());
  this->cae_viewer->SetInput(this->cae_reader->GetOutput());

  this->cae_iren = vtkRenderWindowInteractor::New();
  this->cae_viewer->SetupInteractor(this->cae_iren);

  // Reset the window/level and the camera

  this->cae_reader->Update();
  double *range = this->cae_reader->GetOutput()->GetScalarRange();
  this->cae_viewer->SetColorWindow(range[1] - range[0]);
  this->cae_viewer->SetColorLevel(0.5 * (range[1] + range[0]));

  this->cae_renderwidget->ResetCamera();

  // -----------------------------------------------------------------------

  // The corner annotation has the ability to parse "tags" and fill
  // them with information gathered from other objects.
  // For example, let's display the slice and window/level in one corner
  // by connecting the corner annotation to our image actor and
  // image mapper

  vtkCornerAnnotation *ca = this->cae_renderwidget->GetCornerAnnotation();
  ca->SetImageActor(this->cae_viewer->GetImageActor());
  ca->SetWindowLevel(this->cae_viewer->GetWindowLevel());
  ca->SetText(2, "<slice>");
  ca->SetText(3, "<window>\n<level>");
  ca->SetText(1, "Hello, World!");

  // -----------------------------------------------------------------------

  // Create a corner annotation editor
  // Connect it to the render widget
  
  this->cae_anno_editor = vtkKWCornerAnnotationEditor::New();
  this->cae_anno_editor->SetParent(parent);
  this->cae_anno_editor->Create(app);
  this->cae_anno_editor->SetRenderWidget(this->cae_renderwidget);

  app->Script("pack %s -side left -anchor nw -expand n -padx 2 -pady 2", 
              this->cae_anno_editor->GetWidgetName());
}

vtkKWCornerAnnotationEditorItem::~vtkKWCornerAnnotationEditorItem()
{
  this->cae_anno_editor->Delete();
  this->cae_reader->Delete();
  this->cae_iren->Delete();
  this->cae_renderwidget->Delete();
  this->cae_viewer->Delete();
}
