#ifndef QT__SCRIPT_TEST_1_H
#define QT__SCRIPT_TEST_1_H

#include <QObject>

class MainImageWindow;
class GlobalUIModel;
class QScriptEngine;
class QTimer;
class QtScriptTest1 : public QObject
{
    Q_OBJECT
public:
    QtScriptTest1();
    ~QtScriptTest1();

    void Initialize(MainImageWindow *, GlobalUIModel *, std::string datadir);

    void Run( QScriptEngine * apEngine);

public slots:

    void RunQTimerTriggered();

private:

    MainImageWindow *m_Window;
    GlobalUIModel *m_Model;
    std::string m_DataDir;
    QScriptEngine * m_pEngine;

    QTimer * m_pQTimer;
};
#endif // QT__SCRIPT_TEST_1_H
