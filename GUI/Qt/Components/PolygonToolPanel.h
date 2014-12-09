#ifndef POLYGONTOOLPANEL_H
#define POLYGONTOOLPANEL_H

#include <QWidget>

namespace Ui {
  class PolygonToolPanel;
}

class GlobalUIModel;

class PolygonToolPanel : public QWidget
{
  Q_OBJECT

public:
  explicit PolygonToolPanel(QWidget *parent = 0);
  ~PolygonToolPanel();

  void SetModel(GlobalUIModel *model);

private:
  Ui::PolygonToolPanel *ui;
  GlobalUIModel *m_Model;
};

#endif // POLYGONTOOLPANEL_H
