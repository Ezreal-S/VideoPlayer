#ifndef OVERLAYCOMBOBOX_H
#define OVERLAYCOMBOBOX_H

#include <QWidget>
#include <QPushButton>
#include <QListWidget>
#include <QVBoxLayout>
#include <QApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QGraphicsDropShadowEffect>
#include <QStyleOptionComboBox>
#include <QPainter>
#include <QDebug>

class PopupOverlay : public QWidget {
    Q_OBJECT
public:
    explicit PopupOverlay(QWidget *parent = nullptr);
    void setListWidget(QListWidget *l);
signals:
    void clickedOutside();
protected:
    void mousePressEvent(QMouseEvent *ev) override;
private:
    QListWidget *list_ = nullptr;
};


class OverlayComboBox : public QWidget {
    Q_OBJECT
public:
    explicit OverlayComboBox(QWidget *parent = nullptr);

    void addItem(const QString &text, const QVariant &userData = QVariant());

    void addItems(const QList<QPair<QString, QVariant>> &items);

    int currentIndex() const;
    QString currentText() const;
    QVariant currentData() const;

    void setCurrentIndex(int idx);

signals:
    void currentIndexChanged(int idx);
    void currentTextChanged(const QString &text);
    void currentDataChanged(const QVariant &data);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

    void paintEvent(QPaintEvent *e) override;

private slots:
    void togglePopup();

    void hidePopup();

    void onItemClicked(QListWidgetItem *it);

private:
    QPushButton *button_ = nullptr;
    QListWidget *list_ = nullptr;
    PopupOverlay *overlay_ = nullptr;
    int currentIndex_ = -1;
};

#endif // OVERLAYCOMBOBOX_H
