#ifndef COLORWHEEL_H
#define COLORWHEEL_H

/**
 * From: https://github.com/liuyanghejerry/Qt-Plus/tree/master/develop/ColorWheel
 * Imported: 03/16/2013
 */

#include <QWidget>
#include <QPainter>
#include <QResizeEvent>
#include <QStyleOption>
#include <QDebug>
#include <QtCore/qmath.h>

class ColorWheel : public QWidget
{
  Q_OBJECT

  Q_PROPERTY(int wheelWidth READ wheelWidth WRITE setWheelWidth)


public:
  explicit ColorWheel(QWidget *parent = 0);

  virtual QSize sizeHint () const;
  virtual QSize minimumSizeHint () const;
  QColor color();
  void setColor(const QColor &color);

  int wheelWidth() { return m_WheelWidth; }
  void setWheelWidth(int value) { m_WheelWidth = value; this->update(); }

signals:
  QColor colorChange(const QColor &color);

public slots:
  void hueChanged(const int &hue);
  void svChanged(const QColor &newcolor);
protected:
  void mousePressEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
  void resizeEvent(QResizeEvent *event);
  void paintEvent(QPaintEvent *event);
private:
  QSize initSize;
  QImage wheel;
  bool mouseDown;
  QPoint lastPos;
  int margin;
  int m_WheelWidth;
  QRegion wheelRegion;
  QRegion squareRegion;
  QColor current;
  bool inWheel;
  bool inSquare;
  QColor posColor(const QPoint &point);
  void drawWheel(const QSize &newSize);
  void drawIndicator(const int &hue);
  void drawPicker(const QColor &color);
  void drawSquare(const int &hue);
};

#endif // COLORWHEEL_H
