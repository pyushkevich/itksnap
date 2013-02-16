#ifndef QT__SCRIPT_TEST_1_H
#define QT__SCRIPT_TEST_1_H

//#include <SNAPCommon.h>

class MainImageWindow;
class GlobalUIModel;
class QScriptEngine;

class QtScriptTest1
{
public:
    void Initialize(MainImageWindow *, GlobalUIModel *, std::string datadir);

    void Run( QScriptEngine * apEngine);

private:

    MainImageWindow *m_Window;
    GlobalUIModel *m_Model;
    std::string m_DataDir;
};
#endif // QT__SCRIPT_TEST_1_H
