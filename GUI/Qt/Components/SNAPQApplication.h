/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Filename.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/18 11:25:44 $
  Version:   $Revision: 1.12 $
  Copyright (c) 2011 Paul A. Yushkevich

  This file is part of ITK-SNAP

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

=========================================================================*/
#ifndef __SNAPQApplication_h_
#define __SNAPQApplication_h_

#include <QApplication>
#include <QStringList>
#include <QTime>

class MainImageWindow;

/** Class to handle exceptions in Qt callbacks */
class SNAPQApplication : public QApplication
{
  Q_OBJECT

public:
  SNAPQApplication(int &argc, char **argv);

  void setMainWindow(MainImageWindow *mainwin);

  bool notify(QObject *object, QEvent *event);
  virtual bool event(QEvent *event);

public slots:

  void quitWithReturnCode(int rc);

private:
  MainImageWindow *m_MainWindow;
  QStringList m_Args;
  QTime m_StartupTime;
};

#endif
