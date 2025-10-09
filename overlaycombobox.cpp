#include "overlaycombobox.h"





PopupOverlay::PopupOverlay(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::NoFocus);
    setStyleSheet("background: rgba(0,0,0,50);");
}

void PopupOverlay::setListWidget(QListWidget *l) { list_ = l; }

void PopupOverlay::mousePressEvent(QMouseEvent *ev) {
    if (list_ && list_->geometry().contains(ev->pos())) {
        QWidget::mousePressEvent(ev);
    } else {
        emit clickedOutside();
    }
}

OverlayComboBox::OverlayComboBox(QWidget *parent)
    : QWidget(parent)
{
    // setFixedHeight(28);
    // setMinimumWidth(100);
    setFocusPolicy(Qt::StrongFocus);

    button_ = new QPushButton(this);
    button_->setText("");
    button_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button_->setStyleSheet(R"(
            QPushButton {
                text-align: left;
                padding-left: 6px;
                background-color: white;
                border: 1px solid #888;
                border-radius: 3px;
            }
            QPushButton:hover {
                border-color: #3399ff;
            }
        )");

    QVBoxLayout *l = new QVBoxLayout(this);
    l->setContentsMargins(0,0,0,0);
    l->addWidget(button_);

    list_ = new QListWidget();
    list_->setSelectionMode(QAbstractItemView::SingleSelection);
    list_->installEventFilter(this);

    list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    list_->setTextElideMode(Qt::ElideRight);
    list_->setWordWrap(false);


    // ✅ 列表样式美化
    list_->setStyleSheet(R"(
            QListWidget {
                background-color: white;
                border: 1px solid #888;
                outline: none;
            }
            QListWidget::item {
                padding: 4px 8px;
            }
            QListWidget::item:hover {
                background-color: #e6f0ff;
            }
            QListWidget::item:selected {
                background-color: #cce0ff;
            }
        )");

    // ✅ 阴影
    auto *shadow = new QGraphicsDropShadowEffect(list_);
    shadow->setBlurRadius(12);
    shadow->setOffset(0, 2);
    shadow->setColor(QColor(0, 0, 0, 80));
    list_->setGraphicsEffect(shadow);

    connect(button_, &QPushButton::clicked, this, &OverlayComboBox::togglePopup);
    connect(list_, &QListWidget::itemClicked, this, &OverlayComboBox::onItemClicked);
}

void OverlayComboBox::addItem(const QString &text, const QVariant &userData) {
    QListWidgetItem *it = new QListWidgetItem(text);
    it->setData(Qt::UserRole, userData);
    list_->addItem(it);
    if (currentIndex_ == -1) setCurrentIndex(0);
}

void OverlayComboBox::addItems(const QList<QPair<QString, QVariant> > &items) {
    for (auto &p : items) addItem(p.first, p.second);
}

int OverlayComboBox::currentIndex() const { return currentIndex_; }

QString OverlayComboBox::currentText() const {
    if (currentIndex_ >=0 && currentIndex_ < list_->count()) return list_->item(currentIndex_)->text();
    return QString();
}

QVariant OverlayComboBox::currentData() const {
    if (currentIndex_ >=0 && currentIndex_ < list_->count()) return list_->item(currentIndex_)->data(Qt::UserRole);
    return QVariant();
}

void OverlayComboBox::setCurrentIndex(int idx) {
    if (idx < 0 || idx >= list_->count()) return;
    currentIndex_ = idx;
    button_->setText(list_->item(idx)->text());
    list_->setCurrentRow(idx);
    emit currentIndexChanged(idx);
    emit currentTextChanged(button_->text());
    emit currentDataChanged(currentData());
}

bool OverlayComboBox::eventFilter(QObject *obj, QEvent *ev) {
    if (obj == list_ && ev->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(ev);
        if (ke->key() == Qt::Key_Escape) {
            hidePopup();
            return true;
        } else if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            QListWidgetItem *it = list_->currentItem();
            if (it) onItemClicked(it);
            return true;
        }
    }
    return QWidget::eventFilter(obj, ev);
}

void OverlayComboBox::paintEvent(QPaintEvent *e) {
    QWidget::paintEvent(e);

    // 🎯 绘制下拉箭头（使用系统 QStyle）
    QStyleOptionComboBox opt;
    opt.initFrom(this);
    opt.rect = QRect(width() - 20, 0, 20, height());
    opt.state |= QStyle::State_Enabled;
    if (overlay_ && overlay_->isVisible())
        opt.state |= QStyle::State_On;
    else
        opt.state |= QStyle::State_Off;

    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_IndicatorArrowDown, &opt, &p, this);
}

void OverlayComboBox::togglePopup() {
    if (overlay_ && overlay_->isVisible()) { hidePopup(); return; }

    QWidget *top = this->window();
    if (!top) top = QApplication::activeWindow();
    if (!top) { qWarning() << "No top-level window found for OverlayComboBox"; return; }

    if (!overlay_) {
        overlay_ = new PopupOverlay(top);
        overlay_->setListWidget(list_);
        connect(overlay_, &PopupOverlay::clickedOutside, this, &OverlayComboBox::hidePopup);
    }
    overlay_->setGeometry(top->rect());

    list_->setParent(overlay_);
    list_->setWindowFlags(Qt::Widget);

    int rowCount = list_->count();
    int rowH = (rowCount>0) ? list_->sizeHintForRow(0) : 24;
    int visible = qMin(rowCount, 8);
    int popupH = rowH * visible + 4;

    QPoint globalPos = mapToGlobal(QPoint(0, height()));
    QPoint posInTop = top->mapFromGlobal(globalPos);

    int y = posInTop.y();
    if (y + popupH > top->height()) {
        y = posInTop.y() - popupH - height();
        if (y < 0) y = 0;
    }

    list_->setGeometry(posInTop.x(), y, width(), popupH);

    overlay_->show();
    overlay_->raise();
    list_->show();
    list_->setFocus();

    update(); // 触发重绘，让箭头高亮
}

void OverlayComboBox::hidePopup() {
    if (overlay_) {
        list_->hide();
        overlay_->hide();
    }
    update(); // 触发重绘，让箭头恢复
}

void OverlayComboBox::onItemClicked(QListWidgetItem *it) {
    if (!it) return;
    int idx = list_->row(it);
    setCurrentIndex(idx);
    hidePopup();
}
