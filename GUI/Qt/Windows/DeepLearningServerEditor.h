#ifndef DEEPLEARNINGSERVEREDITOR_H
#define DEEPLEARNINGSERVEREDITOR_H

#include <QDialog>
#include "DeepLearningSegmentationModel.h"

namespace Ui
{
class DeepLearningServerEditor;
}

class DeepLearningServerEditor : public QDialog
{
  Q_OBJECT

public:
  explicit DeepLearningServerEditor(QWidget *parent = nullptr);
  ~DeepLearningServerEditor();

  void SetModel(DeepLearningServerPropertiesModel *model);

private:
  Ui::DeepLearningServerEditor *ui;
  DeepLearningServerPropertiesModel *m_Model;
};

#endif // DEEPLEARNINGSERVEREDITOR_H
