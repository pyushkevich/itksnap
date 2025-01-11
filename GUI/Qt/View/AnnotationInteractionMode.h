#ifndef ANNOTATIONINTERACTIONMODE_H
#define ANNOTATIONINTERACTIONMODE_H

#include <SliceWindowInteractionDelegateWidget.h>
#include <SNAPCommon.h>

class GenericSliceModel;
class GenericSliceView;
class AnnotationModel;
class AnnotationRenderer;
class SliceViewPanel;

class AnnotationInteractionMode : public SliceWindowInteractionDelegateWidget
{
  Q_OBJECT

public:
  explicit AnnotationInteractionMode(QWidget *parent, QWidget *canvasWidget);
  ~AnnotationInteractionMode();

  irisGetMacro(Model, AnnotationModel *)
  void SetModel(AnnotationModel *model);

  irisGetMacro(Renderer, AnnotationRenderer *)

  virtual void mousePressEvent(QMouseEvent *ev) override;
  virtual void mouseMoveEvent(QMouseEvent *ev) override;
  virtual void mouseReleaseEvent(QMouseEvent *ev) override;

public slots:

  void onAcceptAction();

  virtual void onModelUpdate(const EventBucket &bucket) override;

  void onTextInputRequested();

protected:

  AnnotationModel *m_Model;
  SmartPtr<AnnotationRenderer> m_Renderer;
};

#endif // ANNOTATIONINTERACTIONMODE_H
