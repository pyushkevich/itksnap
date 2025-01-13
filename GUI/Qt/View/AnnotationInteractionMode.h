#ifndef ANNOTATIONINTERACTIONMODE_H
#define ANNOTATIONINTERACTIONMODE_H

#include <SliceWindowInteractionDelegateWidget.h>
#include <SNAPCommon.h>

class GenericSliceModel;
class AnnotationModel;
class SliceViewPanel;

class AnnotationInteractionMode : public SliceWindowInteractionDelegateWidget
{
  Q_OBJECT

public:
  explicit AnnotationInteractionMode(QWidget *parent, QWidget *canvasWidget);
  ~AnnotationInteractionMode();

  irisGetMacro(Model, AnnotationModel *)
  void SetModel(AnnotationModel *model);

  virtual void mousePressEvent(QMouseEvent *ev) override;
  virtual void mouseMoveEvent(QMouseEvent *ev) override;
  virtual void mouseReleaseEvent(QMouseEvent *ev) override;

public slots:

  void onAcceptAction();

  virtual void onModelUpdate(const EventBucket &bucket) override;

  void onTextInputRequested();

protected:

  AnnotationModel *m_Model;
};

#endif // ANNOTATIONINTERACTIONMODE_H
