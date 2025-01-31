#ifndef ONLINEMP3_H
#define ONLINEMP3_H

#include <QWidget>
#include <QEvent>
#include <QMouseEvent>

#include <QNetworkRequest>          //HTTP的URL管理类
#include <QNetworkAccessManager>    //URL的上传管理
#include <QNetworkReply>            //网页回复数据触发信号的类
#include <QEventLoop>               //提供一种进入和离开事件循环的方法

#include <QMediaPlayer>             //播放音乐相关
#include <QMediaPlaylist>

#include <QSqlDatabase>             //数据库相关
#include <QSqlQuery>
#include <QSqlError>

#include <QMessageBox>
#include <QTime>
#include <math.h>
#include <QMenu>

#include <QSystemTrayIcon>//系统托盘

QT_BEGIN_NAMESPACE
namespace Ui
{
    class OnlineMp3;
}
QT_END_NAMESPACE

static QString kugouSearchApi = "https://complexsearch.kugou.com/v2/search/song?";
static QString kugouDownldadApi = "https://wwwapi.kugou.com/play/songinfo?";

class QListWidgetItem;
class OnlineMp3 : public QWidget
{
    Q_OBJECT

public:
    OnlineMp3(QWidget *parent = nullptr);
    ~OnlineMp3();

    void initDataBase();//初始化数据库
    void initMusic();//初始化音乐

    void httpAccess(QString url);//访问http网页
    QString getSearchUrl(QString musicName);//获取搜索的url
    void hashJsonAnalysis(QByteArray JsonData);//音乐的hash和ablum_id值解析，使用json
    void downloadPlayer(QString encode_album_audio_id);//音乐歌曲的下载和播放
    QString getDownloadUrl(QString encode_album_audio_id);//获取下载的url
    void switchSong();//切换歌曲

signals:
    void finish(QByteArray Data);//请求完成信号
    void lyricShow(QString url);//显示歌词信号

private slots:
    void on_btn_close_clicked(); //关闭窗口
    void on_btn_min_clicked(); //最小化窗口
    void on_btn_search_clicked(); //搜索歌曲
    void on_btn_start_stop_clicked(); //播放
    void on_btn_loop_clicked(); //循环播放
    void on_hs_sound_valueChanged(int value); //音量调节

    void on_hs_songtime_valueChanged(int value);//改变播放进度

    void netReply(QNetworkReply *reply);//读取网络数据

    void on_lw_record_itemDoubleClicked(QListWidgetItem *item);//双击播放历史歌曲
    void on_lw_search_itemDoubleClicked(QListWidgetItem *item);//双击播放搜索歌曲

private:
    Ui::OnlineMp3 *ui;

    QSqlDatabase db;

    QMediaPlayer *player;
    QMediaPlaylist *playerlist;

    QNetworkRequest *request;
    QNetworkAccessManager *manager;
    bool isdoubleClick = false; //判断是否是双击播放
    QString musicUrl;//存储解析出来的url

    QMetaObject::Connection sig; // 声明连接对象
};

class DragFliter: public QObject
{
    // QObject interface
public:
    virtual bool eventFilter(QObject *watched, QEvent *event) override
    {
        auto w = dynamic_cast<QWidget *>(watched); //转换成窗口类型
        if (!w) return false;//返回false这个事件就会继续向下传递
        if (event->type() == QEvent::MouseButtonPress)
        {
            auto e = dynamic_cast<QMouseEvent *>(event);//父类向子类转换
            if (e) //转换成功
            {
                movePoint = e->globalPos() - w->pos();
                mousePress = true;
            }
        }
        else if (event->type() == QEvent::MouseMove)
        {
            auto e = dynamic_cast<QMouseEvent *>(event);
            if (mousePress)
                w->move(e->globalPos() - movePoint); //实现拖动
        }
        else if (event->type() == QEvent::MouseButtonRelease)
            mousePress = false;

        return QObject::eventFilter(watched, event);//调用父类让他去处理其他事件
    }
private:
    QPoint movePoint;
    bool mousePress = false;
};

#endif // ONLINEMP3_H
