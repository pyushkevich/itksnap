#ifndef QCURSOROVERRIDE_H
#define QCURSOROVERRIDE_H

#include <QCursor>
#include <QApplication>

/**
  A small object that can be allocated on the stack to change the appearance
  of the Qt cursor. It is conventient to use inside of try/catch blocks

  try
    {
    CusrorOverride co;
    // do stuff
    }
  catch(...)
    {
    // cursor restored at this point
    }

*/

class QtCursorOverride
{
public:
  QtCursorOverride(const QCursor &cursor)
  {
    QApplication::setOverrideCursor(cursor);
  }

  QtCursorOverride(Qt::CursorShape shape = Qt::WaitCursor)
  {
    QApplication::setOverrideCursor(QCursor(shape));
  }

  ~QtCursorOverride()
  {
    QApplication::restoreOverrideCursor();
  }
};

#endif // QCURSOROVERRIDE_H
