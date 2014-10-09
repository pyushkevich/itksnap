/*
part of Click'n'Join mode, which was contributed by Roman Grothausmann
*/

#ifndef JOINDATAPANEL_H
#define JOINDATAPANEL_H

#include <SNAPComponent.h>

class GlobalUIModel;

namespace Ui {
class JoinDataPanel;
}

class JoinDataPanel : public SNAPComponent
{
  Q_OBJECT

public:
  explicit JoinDataPanel(QWidget *parent = 0);
  ~JoinDataPanel();

  void SetModel(GlobalUIModel *);

private slots:

  void on_btnStartCnJ_clicked();

  void on_btnCancel_clicked();

  void on_btnFinishCnJ_clicked();

  void on_btnCopySeg_clicked();

  void on_btnClearSeg_clicked();

private:
  Ui::JoinDataPanel *ui;

  GlobalUIModel *m_Model;
};


#endif // JOINDATAPANEL_H
