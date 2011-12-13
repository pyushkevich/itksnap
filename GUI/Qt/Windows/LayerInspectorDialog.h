#ifndef LAYERINSPECTORDIALOG_H
#define LAYERINSPECTORDIALOG_H

#include <QDialog>

class IntensityCurveBox;
class ContrastInspector;

namespace Ui {
    class LayerInspectorDialog;
}

class LayerInspectorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LayerInspectorDialog(QWidget *parent = 0);
    ~LayerInspectorDialog();

  ContrastInspector *GetContrastInspector();

private:
    Ui::LayerInspectorDialog *ui;
};

#endif // LAYERINSPECTORDIALOG_H
