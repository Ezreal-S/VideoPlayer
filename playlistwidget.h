#ifndef PLAYLISTWIDGET_H
#define PLAYLISTWIDGET_H

#include <QListWidget>
#include <QFileInfo>
#include <QDir>
#include <QMenu>
#include <QDesktopServices>
#include <QUrl>



class PlaylistWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit PlaylistWidget(QWidget* parent = nullptr);

    // 加载目录下所有视频文件，并选中当前文件
    void loadFromFile(const QString& filePath);

public:
    void playNext();
    void playPrevious();

signals:
    void playRequested(const QString& filePath);
private slots:
    void onItemDoubleClicked(QListWidgetItem *item);
    void showContextMenu(const QPoint &pos);
private:
    void setPlayingItem(int newIndex);

    int currentPlayingIndex = -1;
    QString currentDir;
};

#endif // PLAYLISTWIDGET_H
