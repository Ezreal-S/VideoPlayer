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
    enum PlayMode {
        Once,         // 只播一次
        Sequential,   // 顺序播放
        LoopOne,      // 单曲循环
        LoopAll,      // 列表循环
        Random        // 随机播放
    };
    explicit PlaylistWidget(QWidget* parent = nullptr);

    // 加载目录下所有视频文件，并选中当前文件
    void loadFromFile(const QString& filePath);

public:
    void playNext();
    void playPrevious();
    void setPlayMode(PlayMode mode) { playMode = mode; }
    PlayMode getPlayMode() const { return playMode; }

signals:
    void playRequested(const QString& filePath);
private slots:
    void onItemDoubleClicked(QListWidgetItem *item);
    void showContextMenu(const QPoint &pos);
private:
    void setPlayingItem(int newIndex);

    int currentPlayingIndex = -1;
    QString currentDir;
    PlayMode playMode = Once;  // 默认顺序播放
};

#endif // PLAYLISTWIDGET_H
