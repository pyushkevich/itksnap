#include "SNAPTestQt.h"
#include "MainImageWindow.h"

#include <QAction>
#include <QLineEdit>

using namespace std;

void SNAPTestQt::Initialize(
    MainImageWindow *win,
    GlobalUIModel *model,
    std::string datadir)
{
  m_Window = win;
  m_Model = model;
  m_DataDir = datadir;

  // TODO: Check the data directory

}

SNAPTestQt::ReturnCode
SNAPTestQt::RunTest(std::string test)
{
  if(test == "list")
    return ListTests();
  else if(test == "SnakeThreshQt")
    return TestSnakeWithThresholding();
  else
    {
    cerr << "Unknown test specified" << std::endl;
    return NO_SUCH_TEST;
    }
}

SNAPTestQt::ReturnCode
SNAPTestQt::ListTests()
{
  cout << "Available Tests" << endl;
  cout << "  SnakeThreshQt    : Test segmentation with thresholding option" << endl;
  return SUCCESS;
}

SNAPTestQt::ReturnCode SNAPTestQt::TestSnakeWithThresholding()
{
  // Use the GUI to open a window
  qFindChild<QAction *>(m_Window, "actionOpenGrey")->trigger();
  // qFindChild<QLineEdit *>(m_Window, "inFilename")->setText("test");

  return SUCCESS;
}
