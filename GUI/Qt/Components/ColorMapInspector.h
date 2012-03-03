#ifndef COLORMAPINSPECTOR_H
#define COLORMAPINSPECTOR_H

#include <QWidget>

namespace Ui {
    class ColorMapInspector;
}

class ColorMapInspector : public QWidget
{
    Q_OBJECT

public:
    explicit ColorMapInspector(QWidget *parent = 0);
    ~ColorMapInspector();

private:
    Ui::ColorMapInspector *ui;
};

#endif // COLORMAPINSPECTOR_H
