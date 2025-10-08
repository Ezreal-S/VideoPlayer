#include "PlaylistWidget.h"
#include <QStandardPaths>
#include <QRandomGenerator>
#include <QAction>
#include <QDebug>

PlaylistWidget::PlaylistWidget(QWidget *parent)
    : QListWidget(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &QListWidget::itemDoubleClicked,
            this, &PlaylistWidget::onItemDoubleClicked);

    connect(this, &QListWidget::customContextMenuRequested,
            this, &PlaylistWidget::showContextMenu);
}

void PlaylistWidget::loadFromFile(const QString &filePath) {
    QFileInfo fileInfo(filePath);
    currentDir = fileInfo.absolutePath();
    QDir dir(currentDir);

    QStringList filters = {"*.mp4", "*.avi", "*.mkv", "*.mov", "*.flv"};
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    clear();
    currentPlayingIndex = -1;

    for (int i = 0; i < fileList.size(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(fileList.at(i).fileName());
        item->setData(Qt::UserRole, fileList.at(i).absoluteFilePath());
        addItem(item);

        if (fileList.at(i).absoluteFilePath() == filePath) {
            setPlayingItem(i);
        }
    }
}

void PlaylistWidget::playNext() {
    if (count() == 0) return;

    int newIndex = (currentPlayingIndex + 1) % count();
    qDebug()<<playMode;
    switch (playMode) {
    case Sequential:
        if (currentPlayingIndex < count() - 1)
            newIndex = currentPlayingIndex + 1;
        else
            return; // 到最后一首就停止
        break;

    case LoopOne:
        newIndex = currentPlayingIndex; // 重复当前
        break;

    case LoopAll:
        newIndex = (currentPlayingIndex + 1) % count();
        break;

    case Random:
        newIndex = QRandomGenerator::global()->bounded(count());
        break;
    default:
        break;
    }

    QListWidgetItem *item = this->item(newIndex);
    if (item) {
        setPlayingItem(newIndex);
        emit playRequested(item->data(Qt::UserRole).toString());
    }
}

void PlaylistWidget::playPrevious() {
    if (count() == 0) return;

    int newIndex = (currentPlayingIndex - 1 + count()) % count();

    switch (playMode) {
    case Sequential:
        if (currentPlayingIndex > 0)
            newIndex = currentPlayingIndex - 1;
        else
            return; // 第一首就停止
        break;

    case LoopOne:
        newIndex = currentPlayingIndex;
        break;

    case LoopAll:
        newIndex = (currentPlayingIndex - 1 + count()) % count();
        break;

    case Random:
        newIndex = QRandomGenerator::global()->bounded(count());
        break;
    default:
        break;
    }

    QListWidgetItem *item = this->item(newIndex);
    if (item) {
        setPlayingItem(newIndex);
        emit playRequested(item->data(Qt::UserRole).toString());
    }
}

void PlaylistWidget::onItemDoubleClicked(QListWidgetItem *item) {
    if (!item) return;
    QString path = item->data(Qt::UserRole).toString();
    int index = row(item);
    setPlayingItem(index);
    emit playRequested(path);
}

void PlaylistWidget::showContextMenu(const QPoint &pos) {
    QListWidgetItem *item = itemAt(pos);
    if (!item) return;

    QMenu menu;
    QAction *playAction = menu.addAction("播放");
    QAction *removeAction = menu.addAction("移除");
    QAction *openDirAction = menu.addAction("打开所在文件夹");

    QAction *selected = menu.exec(viewport()->mapToGlobal(pos));
    if (selected == playAction) {
        onItemDoubleClicked(item);
    } else if (selected == removeAction) {
        delete item;
    } else if (selected == openDirAction) {
        QString path = item->data(Qt::UserRole).toString();
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
    }
}

void PlaylistWidget::setPlayingItem(int newIndex) {
    // 恢复上一个播放项样式
    if (currentPlayingIndex >= 0) {
        QListWidgetItem *oldItem = item(currentPlayingIndex);
        if (oldItem) {
            QFont font = oldItem->font();
            font.setBold(false);
            oldItem->setFont(font);
            oldItem->setForeground(Qt::black);
            //oldItem->setIcon(QIcon());
        }
    }

    // 设置新的播放项样式
    QListWidgetItem *newItem = item(newIndex);
    if (newItem) {
        QFont font = newItem->font();
        font.setBold(true);
        newItem->setFont(font);
        newItem->setForeground(Qt::red);
        //newItem->setIcon(QIcon(":/icons/playing.png")); // 你可以换成自己的图标
    }

    currentPlayingIndex = newIndex;
}
