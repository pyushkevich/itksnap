#ifndef IMAGEINFOINSPECTOR_H
#define IMAGEINFOINSPECTOR_H

#include <SNAPComponent.h>

class ImageInfoModel;

namespace Ui {
class ImageInfoInspector;
}

class ImageInfoInspector : public SNAPComponent
{
  Q_OBJECT

public:
  explicit ImageInfoInspector(QWidget *parent = 0);
  ~ImageInfoInspector();

  void SetModel(ImageInfoModel *model);

private:

  ImageInfoModel *m_Model;

  Ui::ImageInfoInspector *ui;
};

#endif // IMAGEINFOINSPECTOR_H
