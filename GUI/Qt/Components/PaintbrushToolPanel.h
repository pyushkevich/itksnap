#ifndef PAINTBRUSHTOOLPANEL_H
#define PAINTBRUSHTOOLPANEL_H

#include <QWidget>

class PaintbrushSettingsModel;

namespace Ui {
class PaintbrushToolPanel;
}

class PaintbrushToolPanel : public QWidget
{
  Q_OBJECT
  
public:
  explicit PaintbrushToolPanel(QWidget *parent = 0);
  ~PaintbrushToolPanel();

  void SetModel(PaintbrushSettingsModel *model);
  
private:
  Ui::PaintbrushToolPanel *ui;
  PaintbrushSettingsModel *m_Model;
};

#endif // PAINTBRUSHTOOLPANEL_H
