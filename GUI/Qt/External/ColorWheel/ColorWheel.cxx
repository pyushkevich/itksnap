#include "ColorWheel.h"

ColorWheel::ColorWheel(QWidget *parent) :
  QWidget(parent),
  initSize(300,300),
  mouseDown(false),
  margin(5),
  m_WheelWidth(30),
  current(Qt::red),
  inWheel(false),
  inSquare(false)
{
  resize(initSize);
  //    setSizeIncrement(10,10);
  setMinimumSize(200,200);
  //    setMaximumSize(400,400);
  setCursor(Qt::CrossCursor);

  wheel = QImage(initSize, QImage::Format_ARGB32_Premultiplied);
  wheel.fill(Qt::transparent);
}

QColor ColorWheel::color()
{
  return current;
}

void ColorWheel::setColor(const QColor &color)
{
  if(color.hue() != current.hue()){
    hueChanged(color.hue());
    }

  if( color.saturation() != current.saturation()
      || color.value() != current.value() ){
    svChanged(color);
    }
}


QColor ColorWheel::posColor(const QPoint &point)
{
  // if( ! wheel.valid(point) ) return QColor();
  if(inWheel)
    {
    qreal hue = 0;
    int r = qMin(width(), height()) / 2;
    if( point.x() > r )
      {
      if(point.y() < r )
        {
        //1
        hue = 90 - (qAtan2( (point.x() - r) , (r - point.y()) )  / 3.14 / 2 * 360);
        }
      else
        {
        //4
        hue = 270 + (qAtan2( (point.x() - r) , (point.y() - r ) )  / 3.14 / 2 * 360);
        }
      }
    else
      {
      if(point.y() < r )
        {
        //2
        hue =  90 + (qAtan2( (r - point.x()) , (r - point.y()) )  / 3.14 / 2 * 360);
        }
      else
        {
        //3
        hue =  270 - (qAtan2( (r - point.x()) , (point.y() - r ))  / 3.14 / 2 * 360);
        }
      }
    return QColor::fromHsv(hue, current.saturation(), current.value() );
    }
  if(inSquare)
    {
    // region of the widget
    int w = qMin(width(), height());
    // radius of outer circle
    qreal r = w/2-margin;
    // radius of inner circle
    qreal ir = r-m_WheelWidth;
    // left corner of square
    qreal m = w/2.0-ir/qSqrt(2);
    QPoint p = point - QPoint(m, m);
    qreal SquareWidth = 2*ir/qSqrt(2);
    return QColor::fromHsvF( current.hueF(),
                             std::min(std::max(p.x()/SquareWidth, 0.0), 1.0),
                             std::min(std::max(p.y()/SquareWidth, 0.0), 1.0) );
    }
  return QColor();
}

QSize ColorWheel::sizeHint () const
{
  return QSize(height(),height());
}
QSize ColorWheel::minimumSizeHint () const
{
  return QSize(30,30);
}

void ColorWheel::mousePressEvent(QMouseEvent *event)
{
  lastPos = event->pos();
  if(wheelRegion.contains(lastPos)){
    inWheel = true;
    inSquare = false;
    QColor color = posColor(lastPos);
    hueChanged(color.hue());
    }else if(squareRegion.contains(lastPos)){
    inWheel = false;
    inSquare = true;
    QColor color = posColor(lastPos);
    svChanged(color);
    }
  mouseDown = true;
}

void ColorWheel::mouseMoveEvent(QMouseEvent *event)
{
  lastPos = event->pos();
  if( !mouseDown ) return;
  if(inWheel)
    {
    QColor color = posColor(lastPos);
    hueChanged(color.hue());
    }
  else if(inSquare)
    {
    QColor color = posColor(lastPos);
    svChanged(color);
    }
}

void ColorWheel::mouseReleaseEvent(QMouseEvent *event)
{
  mouseDown = false;
  inWheel = false;
  inSquare = false;
}

void ColorWheel::resizeEvent(QResizeEvent *event)
{
  wheel = QImage(event->size(), QImage::Format_ARGB32_Premultiplied);
  wheel.fill(Qt::transparent);
  //    source = wheel;
  //    source.fill(Qt::transparent);

  drawWheel(event->size());
  drawSquare(current.hue());
  drawIndicator(current.hue());
  drawPicker(current);
  update();
}

void ColorWheel::paintEvent(QPaintEvent *event)
{
  QPainter painter(this);
  QStyleOption opt;
  opt.init(this);
  painter.drawImage(0,0,wheel);
  style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);
}

#include <QPalette>
void ColorWheel::drawWheel(const QSize &newSize)
{

  int r = qMin(newSize.width(), newSize.height());

  QPainter painter(&wheel);
  painter.setRenderHint(QPainter::Antialiasing);
  // wheel.fill(Qt::white);
  wheel.fill(Qt::transparent);

  QConicalGradient conicalGradient(0, 0, 0);
  conicalGradient.setColorAt(0.0, Qt::red);
  conicalGradient.setColorAt(60.0/360.0, Qt::yellow);
  conicalGradient.setColorAt(120.0/360.0, Qt::green);
  conicalGradient.setColorAt(180.0/360.0, Qt::cyan);
  conicalGradient.setColorAt(240.0/360.0, Qt::blue);
  conicalGradient.setColorAt(300.0/360.0, Qt::magenta);
  conicalGradient.setColorAt(1.0, Qt::red);

  /* outer circle */
  painter.translate(r / 2, r / 2);

  QBrush brush(conicalGradient);
  // painter.setPen(Qt::NoPen);

  painter.setPen(Qt::darkGray);
  painter.setBrush(brush);
  painter.drawEllipse(QPoint(0,0),r/2-margin,r/2-margin);

  /* inner circle */
  painter.setBrush(palette().background().color());
  painter.drawEllipse(QPoint(0,0),r/2-margin-m_WheelWidth,r/2-margin-m_WheelWidth);


  //    QPainter painter2(&wheel);
  //    painter2.drawImage(0,0,source);

  //caculate wheel region
  wheelRegion = QRegion(r/2, r/2, r-2*margin, r-2*margin, QRegion::Ellipse);
  wheelRegion.translate(-(r-2*margin)/2, -(r-2*margin)/2);

  int tmp = 2*(margin+m_WheelWidth);
  QRegion subRe( r/2, r/2, r-tmp, r-tmp, QRegion::Ellipse );
  subRe.translate(-(r-tmp)/2, -(r-tmp)/2);
  wheelRegion -= subRe;
}

void ColorWheel::drawSquare(const int &hue)
{
  QPainter painter(&wheel);
  painter.setRenderHint(QPainter::Antialiasing);

  // region of the widget
  int w = qMin(width(), height());
  // radius of outer circle
  qreal r = w/2-margin;
  // radius of inner circle
  qreal ir = r-m_WheelWidth;
  // left corner of square
  qreal m = w/2.0-ir/qSqrt(2);
  painter.translate(m, m);
  // painter.setPen(Qt::NoPen);
  painter.setPen(Qt::darkGray);

  QImage square(255,255,QImage::Format_ARGB32_Premultiplied);
  QColor color;
  QRgb vv;
  for(int i=0;i<255;++i){
    for(int j=0;j<255;++j){
      color = QColor::fromHsv(hue,i,j);
      vv = qRgb(color.red(),color.green(),color.blue());
      square.setPixel(i,j,vv);
      }
    }
  qreal SquareWidth = 2*ir/qSqrt(2);
  square = square.scaled(SquareWidth,SquareWidth);
  painter.drawImage(0,0,square);
  painter.drawRect(0,0,SquareWidth,SquareWidth);

  //    QPainter painter2(&wheel);
  //    painter2.drawImage(0,0,source);

  squareRegion = QRegion(m, m, SquareWidth, SquareWidth);
}

void ColorWheel::drawIndicator(const int &hue)
{
  QPainter painter(&wheel);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(Qt::black);
  painter.setBrush(Qt::NoBrush);

  QPen pen = painter.pen();
  pen.setWidth(2);
  painter.setPen(pen);
  qreal r = qMin(height(), width()) / 2.0;
  painter.translate(r, r);
  painter.rotate( -hue );
  r = qMin(height(), width()) / 2.0 - margin - m_WheelWidth/2;
  painter.drawEllipse(QPointF(r,0.0),4,4);
}

void ColorWheel::drawPicker(const QColor &color)
{
  QPainter painter(&wheel);
  painter.setRenderHint(QPainter::Antialiasing);
  QPen pen;

  // region of the widget
  int w = qMin(width(), height());
  // radius of outer circle
  qreal r = w/2-margin;
  // radius of inner circle
  qreal ir = r-m_WheelWidth;
  // left corner of square
  qreal m = w/2.0-ir/qSqrt(2);
  painter.translate(m-4, m-4);
  qreal SquareWidth = 2*ir/qSqrt(2);
  qreal S = color.saturationF()*SquareWidth;
  qreal V = color.valueF()*SquareWidth;

  if(color.saturation() > 30 ||color.value() < 50){
    pen.setColor(Qt::white);
    }
  pen.setWidth(2);
  painter.setPen(pen);
  painter.drawEllipse(S,V,8,8);
}

void ColorWheel::hueChanged(const int &hue)
{
  if( hue<0 )return;
  int s = current.saturation();
  int v = current.value();
  current.setHsv(hue, s, v);
  drawWheel(size());
  drawSquare(hue);
  drawIndicator(hue);
  drawPicker(current);
  repaint();
  emit colorChange(current);
}

void ColorWheel::svChanged(const QColor &newcolor)
{
  int hue = current.hue();
  current.setHsv(hue, newcolor.saturation(), newcolor.value());
  drawWheel(size());
  drawSquare(hue);
  drawIndicator(hue);
  drawPicker(newcolor);
  repaint();
  emit colorChange(current);
}
