#ifndef QTFLOWLAYOUT_H
#define QTFLOWLAYOUT_H

#include <QWidget>
#include <QLayout>
#include <QStyle>


/** Borrowed from Qt Examples */
class QtFlowLayout : public QLayout
{
  Q_OBJECT

public:
    explicit QtFlowLayout(QWidget *parent, int margin = -1, int hSpacing = -1, int vSpacing = -1);
    explicit QtFlowLayout(int margin = -1, int hSpacing = -1, int vSpacing = -1);
    ~QtFlowLayout();

    void addItem(QLayoutItem *item) override;
    int horizontalSpacing() const;
    int verticalSpacing() const;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int) const override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect &rect) override;
    QSize sizeHint() const override;
    QLayoutItem *takeAt(int index) override;

private:
    int doLayout(const QRect &rect, bool testOnly) const;
    int smartSpacing(QStyle::PixelMetric pm) const;

    QList<QLayoutItem *> itemList;
    int m_hSpace;
    int m_vSpace;
};

#endif // QTFLOWLAYOUT_H
