#ifndef QT__SCRIPT_TEST_1_H
#define QT__SCRIPT_TEST_1_H

#include <QObject>

class MainImageWindow;
class GlobalUIModel;
class QScriptEngine;
class QTimer;
class QSpinBox;
class QAction;

class CursorInspector;
class ImageIOWizard;

class QtScriptTest1 : public QObject
{
    Q_OBJECT
public:
    QtScriptTest1();
    ~QtScriptTest1();

    void Initialize(MainImageWindow *, GlobalUIModel *, std::string datadir);

    void Run( QScriptEngine * apEngine);

public slots:

    //void RunQTimerTriggered();
    void Run_ActionOpenMainTriggered();
    void Run_SetFileDetails();
    void Run_ValidateNext();
    void Run_ValidateFinish();

private:

    MainImageWindow *m_Window;
    GlobalUIModel *m_Model;
    std::string m_DataDir;
    QScriptEngine * m_pEngine;

    QTimer * m_pQTimer;


    CursorInspector *ci;
    QSpinBox *inX;
    QSpinBox *outId;

    QAction * pActionOpenMain;
    ImageIOWizard * m_pWiz;

    //The next function will be deleted
    void Former_RunQTimerTriggered();

};
#endif // QT__SCRIPT_TEST_1_H
