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
#include "MainImageWindow.h"
#include "SNAPQtCommon.h"
#include "SNAPQApplication.h"
#include <QFileOpenEvent>
#include <QUrl>

SNAPQApplication
::SNAPQApplication(int &argc, char **argv)
  : QApplication(argc, argv)
{
  this->setApplicationName("ITK-SNAP");
  this->setOrganizationName("itksnap.org");

#if QT_VERSION >= 0x050000
  // Allow @x2 pixmaps for icons for retina displays
  this->setAttribute(Qt::AA_UseHighDpiPixmaps, true);

  // System-supplied DPI screws up widget and font scaling horribly
  this->setAttribute(Qt::AA_Use96Dpi, true);
#endif

  m_MainWindow = NULL;

  // Store the command-line arguments
  for(int i = 1; i < argc; i++)
    m_Args.push_back(QString::fromUtf8(argv[i]));
}

void SNAPQApplication::setMainWindow(MainImageWindow *mainwin)
{
  m_MainWindow = mainwin;
  m_StartupTime = QTime::currentTime();
}

bool SNAPQApplication::notify(QObject *object, QEvent *event)
{
  try { return QApplication::notify(object, event); }
  catch(std::exception &exc)
  {
    // Crash!
    ReportNonLethalException(NULL, exc, "Unexpected Error",
                             "ITK-SNAP has crashed due to an unexpected error");

    // Exit the application
    QApplication::exit(-1);

    return false;
  }
}

bool SNAPQApplication::event(QEvent *event)
{
  // Handle file drops
  if (event->type() == QEvent::FileOpen && m_MainWindow)
    {
    QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
    QString file = openEvent->url().path();

    // MacOS bug - we get these open document events automatically generated
    // from command-line parameters, and I have no idea why. To avoid this,
    // if the event occurs at startup (within a second), we will check if
    // the passed in URL matches the command-line arguments, and ignore it
    // if it does
    if(m_StartupTime.secsTo(QTime::currentTime()) < 1)
      {
      foreach(const QString &arg, m_Args)
        {
        if(arg == file)
          return true;
        }
      }

    // Accept the event
    event->accept();

    // Ok, we passed the check, now it's safe to actually open the file
    m_MainWindow->raise();
    m_MainWindow->LoadDroppedFile(file);
    return true;
    }

  else return QApplication::event(event);
}

void SNAPQApplication::quitWithReturnCode(int rc)
{
  this->exit(rc);
}


