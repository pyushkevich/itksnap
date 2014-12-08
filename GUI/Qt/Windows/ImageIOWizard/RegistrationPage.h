#ifndef REGISTRATIONPAGE_H
#define REGISTRATIONPAGE_H

#include <QWidget>
#include "ImageIOWizard.h"

namespace Ui {
  class RegistrationPage;
}

namespace imageiowiz {

class RegistrationWorkerThread : public QThread
{
  Q_OBJECT

public:

  void Initialize(ImageIOWizardModel *model);
  void run();
  void OnProgressEvent();

signals:

  void registrationProgress();

private:

  ImageIOWizardModel *m_Model;
};

class RegistrationPage : public AbstractPage
{
  Q_OBJECT

public:
  explicit RegistrationPage(QWidget *parent = 0);
  ~RegistrationPage();

  int nextId() const;
  void initializePage();
  bool validatePage();
  virtual bool isComplete() const;

public slots:

  void onRegistrationProgress();

private slots:
  void on_btnRun_clicked();

private:
  Ui::RegistrationPage *ui;
};

} // end namespace

#endif // REGISTRATIONPAGE_H
