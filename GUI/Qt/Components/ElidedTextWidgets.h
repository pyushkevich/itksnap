#ifndef ELIDEDTEXTWIDGETS_H
#define ELIDEDTEXTWIDGETS_H

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QHeaderView>

/*
 * Widgets that show text too long for their width with "..." in the middle,
 * e.g. "T1_3D_0.8x…_3.nii.gz", so that both the start and the end of a long
 * filename stay visible. The widget keeps the full text, so text() and the
 * couplings work as with the parent class; only painting changes. While the
 * text is elided, the tooltip shows the full text.
 *
 * To use them in Designer, promote a QLabel, QLineEdit or QComboBox to the
 * class below with header "ElidedTextWidgets.h". Couplings need the widget
 * cast to the parent class, e.g. makeCoupling((QLabel *) ui->label, model).
 *
 * ElidedLabel and ElidedLineEdit have an "elidePath" property (settable in
 * Designer) for text that is a file path or URL. The directory and the
 * filename are then elided separately, so that the start of both stays
 * visible, e.g. "/Users/pauly/Li…/Cache/T1_3D_0.8x…_3.nii.gz".
 */

/**
 * Elide text to width pixels in the middle. With elide_path, text is a file
 * path or URL, and its directory and filename are elided separately.
 */
QString elideText(const QFontMetrics &fm, const QString &text, int width, bool elide_path);

/** A single-line QLabel. Rich text and word-wrapped labels are not elided. */
class ElidedLabel : public QLabel
{
  Q_OBJECT
  Q_PROPERTY(bool elidePath READ elidePath WRITE setElidePath)

public:
  explicit ElidedLabel(QWidget *parent = nullptr);

  /** Elide the text as a file path */
  bool elidePath() const { return m_ElidePath; }
  void setElidePath(bool value);

  /** Allows the layout to make the label narrower than its text */
  QSize minimumSizeHint() const override;

  /** Whether the text is currently shown elided */
  bool isElided() const;

protected:
  void paintEvent(QPaintEvent *event) override;
  bool event(QEvent *event) override;

private:
  bool canElide() const;
  QRect textRect() const;

  bool m_ElidePath = false;
};

/**
 * A QLineEdit that is elided while it does not have keyboard focus. With
 * focus, the full text is shown so that it can be selected or edited.
 */
class ElidedLineEdit : public QLineEdit
{
  Q_OBJECT
  Q_PROPERTY(bool elidePath READ elidePath WRITE setElidePath)

public:
  explicit ElidedLineEdit(QWidget *parent = nullptr);

  /** Elide the text as a file path */
  bool elidePath() const { return m_ElidePath; }
  void setElidePath(bool value);

  /** Whether the text is currently shown elided */
  bool isElided() const;

protected:
  void paintEvent(QPaintEvent *event) override;
  bool event(QEvent *event) override;

private:
  QRect textRect() const;

  bool m_ElidePath = false;
};

/** A non-editable QComboBox whose current item is elided. */
class ElidedComboBox : public QComboBox
{
  Q_OBJECT

public:
  explicit ElidedComboBox(QWidget *parent = nullptr);

  /** Whether the current text is shown elided */
  bool isElided() const;

protected:
  void paintEvent(QPaintEvent *event) override;
  bool event(QEvent *event) override;

private:
  int textWidth() const;
};

/**
 * A QHeaderView that elides each line of a multi-line section title on its
 * own, in the middle. QHeaderView elides a multi-line title as a whole, which
 * clips long lines instead. Install with QTableView::setHorizontalHeader.
 */
class ElidedHeaderView : public QHeaderView
{
  Q_OBJECT

public:
  explicit ElidedHeaderView(Qt::Orientation orientation, QWidget *parent = nullptr);

protected:
  void initStyleOptionForIndex(QStyleOptionHeader *option, int logicalIndex) const override;
};

/**
 * Sets the title of a window so that, when it is too long for the title bar,
 * the names in it (e.g. filenames) are elided in the middle rather than the
 * title being cut off at the end by the window system. The title is given as
 * a format such as "%1 - %2 - ITK-SNAP" plus the names that replace %1, %2.
 * It is elided again whenever the window is resized.
 *
 * The title bar is drawn by the window system, so the available width is an
 * estimate: the window width less room for the title bar buttons, measured in
 * the system title font.
 */
class WindowTitleElider : public QObject
{
  Q_OBJECT

public:
  /** The elider is owned by the window */
  explicit WindowTitleElider(QWidget *window);

  void setTitle(const QString &format, const QStringList &names = QStringList());

  /** The title with the names in full */
  QString fullTitle() const;

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  void update();

  QWidget    *m_Window;
  QString     m_Format;
  QStringList m_Names;
};

#endif // ELIDEDTEXTWIDGETS_H
