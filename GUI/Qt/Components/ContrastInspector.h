#ifndef CONTRASTINSPECTOR_H
#define CONTRASTINSPECTOR_H

#include <QWidget>

class IntensityCurveBox;
class IntensityCurveModel;

namespace Ui {
    class ContrastInspector;
}

class ContrastInspector : public QWidget
{
    Q_OBJECT

public:
    explicit ContrastInspector(QWidget *parent = 0);
    ~ContrastInspector();

  IntensityCurveBox *GetCurveBox();

  void SetModel(IntensityCurveModel *model);


private:

  IntensityCurveModel *m_Model;

    Ui::ContrastInspector *ui;
};

#endif // CONTRASTINSPECTOR_H
