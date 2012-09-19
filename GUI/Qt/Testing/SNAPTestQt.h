#ifndef SNAPTESTQT_H
#define SNAPTESTQT_H

#include <SNAPCommon.h>

class MainImageWindow;
class GlobalUIModel;

class SNAPTestQt
{
public:

  enum ReturnCode {
    SUCCESS = 0,
    EXCEPTION_CAUGHT,
    REGRESSION_TEST_FAILURE,
    NO_SUCH_TEST,
    UNKNOWN_ERROR
    };


  void Initialize(MainImageWindow *, GlobalUIModel *, std::string datadir);


  ReturnCode RunTest(std::string test);

protected:

  ReturnCode ListTests();

  ReturnCode TestSnakeWithThresholding();

  MainImageWindow *m_Window;
  GlobalUIModel *m_Model;
  std::string m_DataDir;

};

#endif // SNAPTESTQT_H
