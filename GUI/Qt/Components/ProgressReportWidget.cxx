#include "ProgressReportWidget.h"

#include "ProgressReportWidget.h"
#include <QLabel>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QTimer>
#include <QEvent>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QCoreApplication>


// === ProgressReportWidget =====================================

ProgressReportWidget::ProgressReportWidget(QWidget *parent)
  : QWidget(parent)
{
  setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
  // setAttribute(Qt::WA_TranslucentBackground);
  setAttribute(Qt::WA_TransparentForMouseEvents, false);

  setStyleSheet(R"(
    ProgressReportWidget {
        background-color: rgba(30,30,30,180);
        border-radius: 8px;
    }

    QLabel {
        color : rgba(255,255,255,180);
    }

)");

  // Set background color and minimum size
  parent->installEventFilter(this);

  m_layout = new QVBoxLayout(this);
  m_layout->setContentsMargins(6, 6, 6, 6);
  m_layout->setSpacing(6);
  m_layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

  setVisible(false);

  auto *eff = new QGraphicsOpacityEffect(this);
  eff->setEnabled(false);
  setGraphicsEffect(eff);

  // Create the fade-ine and fade-out animations
  m_fadeInAnimation = new QPropertyAnimation(eff, "opacity");
  m_fadeInAnimation->setDuration(250);
  m_fadeInAnimation->setStartValue(0.0);
  m_fadeInAnimation->setEndValue(1.0);

  m_fadeOutAnimation = new QPropertyAnimation(eff, "opacity");
  m_fadeOutAnimation->setDuration(250);
  m_fadeOutAnimation->setStartValue(1.0);
  m_fadeOutAnimation->setEndValue(0.0);

  // When the fadeout finishes, the widget should be hidden

  // When fading in, the opacity effect should be disabled
  connect(m_fadeInAnimation, &QPropertyAnimation::finished, this, [=](){ this->graphicsEffect()->setEnabled(false); });
  connect(m_fadeOutAnimation, &QPropertyAnimation::finished, this, [this] { setVisible(false); });
}

void
ProgressReportWidget::repositionToParent()
{
  if (auto *p = parentWidget())
  {
    QPoint pos = p->rect().bottomRight();
    pos.setX(pos.x() - width() - 32);
    pos.setY(pos.y() - height() - 48);
    move(p->mapToGlobal(pos));
  }
}


void
ProgressReportWidget::setAutoRemoveSecs(int secs)
{
  m_autoRemoveSecs = secs;
}
void
ProgressReportWidget::setSpinnerDelayMs(int ms)
{
  m_spinnerDelayMs = ms;
}

bool
ProgressReportWidget::event(QEvent *event)
{
  // Let parent handle the event
  bool rc = QWidget::event(event);

  // After resize, adjust own position relative to parent
  if (event->type() == QEvent::Resize)
  {
    this->repositionToParent();
    // auto *re = dynamic_cast<QResizeEvent *>(event);
    // qDebug() << "Resize event from: " << re->oldSize() << " to " << re->size();
  }
  return QWidget::event(event);
}

bool
ProgressReportWidget::eventFilter(QObject *obj, QEvent *event)
{
  if (obj == parentWidget())
  {
    if (event->type() == QEvent::Close || event->type() == QEvent::Hide ||
        event->type() == QEvent::Destroy)
    {
      this->hide();
    }
    else if (event->type() == QEvent::Move || event->type() == QEvent::Resize)
    {
      repositionToParent();
    }
  }
  return QWidget::eventFilter(obj, event);
}

void
ProgressReportWidget::fadeIn()
{
  // The effect that we are animating
  auto *eff = static_cast<QGraphicsOpacityEffect *>(graphicsEffect());

  // First of all, we need to stop the fade-out animation if it is running
  m_fadeOutAnimation->stop();

  // If the widget is at full opacity, or if we are already fading it in, do nothing
  if (isVisible() &&
      (eff->opacity() == 1.0 || m_fadeInAnimation->state() == QAbstractAnimation::Running))
    return;

  // Set up the fade in animation to bring us back to full visibility
  setVisible(true);
  double op = eff->opacity();
  eff->setEnabled(true);
  m_fadeInAnimation->setStartValue(op);
  m_fadeInAnimation->setDuration(static_cast<int>(250 * (1 - op)));
  m_fadeInAnimation->start();
}

void
ProgressReportWidget::fadeOutIfEmpty()
{
  if (!m_tasks.isEmpty())
    return;

  qDebug() << "ProgressReportWidget empty, fading out";

  // The effect that we are animating
  auto *eff = static_cast<QGraphicsOpacityEffect *>(graphicsEffect());
  eff->setEnabled(true);
  m_fadeOutAnimation->start();
}

void
ProgressReportWidget::startTask(const QString &id, const QString &label, bool reportsProgress)
{
  qDebug() << "Starting new task " << id << " label " << label
           << " progress-reporting: " << reportsProgress;
  fadeIn();

  ProgressTask *task;
  if (!m_tasks.contains(id))
  {
    task = new ProgressTask;
    task->id = id;
    m_tasks[id] = task;
    createTaskUI(*task);
  }
  else
  {
    task = m_tasks[id];
  }

  task->label = label;
  task->reportsProgress = reportsProgress;
  task->widget->setLabel(label);

  // Restart auto-removal
  task->autoRemoveTimer.stop();
  task->autoRemoveTimer.setSingleShot(true);
  task->autoRemoveTimer.start(m_autoRemoveSecs * 1000);
  connect(&task->autoRemoveTimer, &QTimer::timeout, this, [=] { removeTask(id); });

  // Spinner delay if no progress
  if (!reportsProgress)
  {
    task->spinnerDelayTimer.stop();
    task->spinnerDelayTimer.setSingleShot(true);
    task->spinnerDelayTimer.start(m_spinnerDelayMs);
    connect(&task->spinnerDelayTimer, &QTimer::timeout, this, [=] {
      if (task->progress < 0)
        task->widget->showSpinner();
    });
  }
}

void
ProgressReportWidget::recomputeSizeAndReposition()
{
  // Ensure layout recomputes
  m_layout->invalidate();
  m_layout->activate();

  // Resizing with m_layout->totalSizeHint() works!
  auto sz = m_layout->totalSizeHint();
  auto minsz = this->minimumSize();
  sz = sz.expandedTo(minsz);
  resize(sz);
  updateGeometry();
}

void
ProgressReportWidget::createTaskUI(ProgressTask &task)
{
  task.widget = new ProgressTaskWidget(task.label, task.reportsProgress, this);
  m_layout->addWidget(task.widget);
  recomputeSizeAndReposition();
}

void
ProgressReportWidget::updateTaskLabel(const QString &id, const QString &label)
{
  if (!m_tasks.contains(id))
    return;
  m_tasks[id]->label = label;
  m_tasks[id]->widget->setLabel(label);
}

void
ProgressReportWidget::updateTaskWithoutProgress(const QString &taskId)
{
  if (!m_tasks.contains(taskId))
    return;

  // There is nothing to update for the task, but we should still process events
  // and in the future, offer a way to abort the running computation

  // Restart removal timer
  auto *task = m_tasks[taskId];
  task->autoRemoveTimer.start(m_autoRemoveSecs * 1000);

  // Process events before we go back to the blocking computation
  QCoreApplication::processEvents();
}

void
ProgressReportWidget::updateTaskProgress(const QString &id, int pct)
{
  if (!m_tasks.contains(id))
    return;
  auto *task = m_tasks[id];

  // If the task doesn't report progress, then just treat this as a regular update
  if (!task->reportsProgress)
  {
    updateTaskWithoutProgress(id);
    return;
  }

  // Report the actuall progress
  task->progress = pct;
  task->widget->showProgress(pct);

  // Restart removal timer
  task->autoRemoveTimer.start(m_autoRemoveSecs * 1000);

  // Process events before we go back to the blocking computation
  QCoreApplication::processEvents();
}

void
ProgressReportWidget::finishTask(const QString &id)
{
  if (!m_tasks.contains(id))
    return;
  auto *task = m_tasks[id];

  // Mark task as finished
  if(task->reportsProgress)
  {
    task->progress = 100;
    task->widget->showProgress(100);
  }
  else
  {
    task->widget->markSpinnerComplete();
  }

  // If at this point we are still fading in, then disable that effect,
  // so we don't have stacking effects
  auto *effExisting = static_cast<QGraphicsOpacityEffect *>(graphicsEffect());
  if(effExisting->isEnabled())
  {
    effExisting->setOpacity(1.0);
    effExisting->setEnabled(false);
    m_fadeInAnimation->stop();
  }

  // Start a timer to fade the task out and remove it
  auto *eff = new QGraphicsOpacityEffect(task->widget);
  task->widget->setGraphicsEffect(eff);
  eff->setOpacity(1.0);

  // Restart auto-removal
  task->fadeAndRemoveTimer.stop();
  task->fadeAndRemoveTimer.setSingleShot(true);
  task->fadeAndRemoveTimer.start(50);
  connect(&task->fadeAndRemoveTimer, &QTimer::timeout, this, [=] {
    double opacity = eff->opacity() - 0.05;
    if (opacity > 0)
    {
      eff->setOpacity(opacity);
      task->widget->repaint();
      qDebug() << "Fading out task " << id << " to opacity " << opacity;
      task->fadeAndRemoveTimer.start(50);
    }
    else
    {
      task->widget->setGraphicsEffect(nullptr);
      task->widget->setVisible(false);
      removeTask(id);
    }
  });

  // Let the auto-remove timer remove the task
  // task->autoRemoveTimer.start(m_autoRemoveSecs * 1000);
}

void
ProgressReportWidget::removeTask(const QString &id)
{
  if (!m_tasks.contains(id))
    return;

  auto *task = m_tasks[id];
  removeTaskUI(*task);
  m_tasks.remove(id);
  delete task;

  fadeOutIfEmpty();
}

void
ProgressReportWidget::removeTaskUI(ProgressTask &task)
{
  qDebug() << "Removing task UI " << task.id;

  // task.widget->fadeOutAndDelete();
  m_layout->removeWidget(task.widget);
  task.widget->deleteLater();
  recomputeSizeAndReposition();
}

ProgressTaskWidget::ProgressTaskWidget(const QString &label, bool reportsProgress, QWidget *parent)
  : QWidget(parent)
{
  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(0,0,0,0);
  textLabel = new QLabel(label, this);

  progressBar = new QProgressBar(this);
  progressBar->setRange(0, 100);
  progressBar->setVisible(reportsProgress);

  progressBar->setFixedWidth(100);
  progressBar->setFixedHeight(16);

  spinner = new Spinner(this);
  spinner->setVisible(false);

  layout->addWidget(textLabel);
  layout->addWidget(progressBar);
  layout->addWidget(spinner);

  // Fade in
  /*
  auto *eff = new QGraphicsOpacityEffect(this);
  setGraphicsEffect(eff);
  auto *anim = new QPropertyAnimation(eff, "opacity");
  anim->setDuration(200);
  anim->setStartValue(0.0);
  anim->setEndValue(1.0);
  anim->start(QAbstractAnimation::DeleteWhenStopped);
*/
}

void
ProgressTaskWidget::setLabel(const QString &txt)
{
  textLabel->setText(txt);
}

void
ProgressTaskWidget::showProgress(int pct)
{
  progressBar->setVisible(true);
  spinner->setVisible(false);
  progressBar->setValue(pct);
}

void
ProgressTaskWidget::showSpinner()
{
  progressBar->setVisible(false);
  spinner->setVisible(true);
}

void
ProgressTaskWidget::markSpinnerComplete()
{
  spinner->setComplete();
}

void
ProgressTaskWidget::fadeOutAndDelete()
{
  /*
  auto *eff = new QGraphicsOpacityEffect(this);
  setGraphicsEffect(eff);

  auto *anim = new QPropertyAnimation(eff, "opacity");
  anim->setDuration(200);
  anim->setStartValue(1.0);
  anim->setEndValue(0.0);

  // connect(anim, &QPropertyAnimation::finished, this, [this] { delete this; });
  connect(anim, &QPropertyAnimation::finished, [=]() {
    this->setVisible(false);
    this->setGraphicsEffect(nullptr);
    this->deleteLater();
  });

  anim->start(QAbstractAnimation::DeleteWhenStopped);
*/
  this->setVisible(false);
  this->deleteLater();
}

QSize
ProgressTaskWidget::minimumSizeHint() const
{
  return QSize(300, 16);
}

Spinner::Spinner(QWidget *parent)
  : QWidget(parent)
  , angle(0)
{
  timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, [this] {
    angle = (angle + 30) % 360;
    update();
  });
  timer->start(100);
  setFixedSize(16, 16);
  complete = false;
}

QSize
Spinner::sizeHint() const
{
  return QSize(16, 16);
}

QSize
Spinner::minimumSizeHint() const
{
  return sizeHint();
}

void
Spinner::setComplete()
{
  this->complete = true;
}

void
Spinner::paintEvent(QPaintEvent *)
{
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);
  if(!complete)
  {
    p.translate(width() / 2, height() / 2);
    p.rotate(angle);
    p.setBrush(Qt::gray);
    p.drawEllipse(QPoint(6, 0), 3, 3);
  }
  else
  {
    p.setBrush(Qt::green);
    p.drawEllipse(rect().center(), 6, 6);
  }
}
