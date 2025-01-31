#include "onlinemp3.h"
#include "ui_onlinemp3.h"
#include <QPainter>
#include <QDebug>
#include <QSqlRecord>
#include <QJsonArray>//封装JSON数组
#include <QJsonObject>//封装JSON对象
#include <QJsonDocument>
#include <QButtonGroup>

OnlineMp3::OnlineMp3(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::OnlineMp3)
{
    ui->setupUi(this);

    setWindowFlag(Qt::FramelessWindowHint); //无边框
    installEventFilter(new DragFliter);

    initDataBase();//初始化数据库
    initMusic();//初始化音乐
    switchSong();//切换歌曲
}

OnlineMp3::~OnlineMp3()
{
    delete ui;
}

//初始化数据库
void OnlineMp3::initDataBase()
{
    // 1、连接数据库
    db = QSqlDatabase::addDatabase("QSQLITE"); // 如果不存在，则添加一个SQLite数据库连接
    db.setDatabaseName("song.db"); // 设置数据库文件名为"song.db"

    // 2、打开数据库，读取数据表
    if (!db.open()) // 尝试打开数据库
    {
        // 打开数据库失败，显示错误信息
        QMessageBox::critical(nullptr, "错误", db.lastError().text());
    }
    else
    {
        // 3、定义查询对象，执行数据库操作
        QSqlQuery query; // 定义数据库查询对象
        QString qstl = "create table if not exists songlist(id integer , FileName text , EMixSongID text);"; // 创建歌曲列表表格的SQL语句
        int ret = query.exec(qstl); // 执行SQL语句
        if (!ret) // 检查SQL执行是否成功
        {
            // SQL执行失败，显示错误信息
            QMessageBox::critical(nullptr, "create table songlist", query.lastError().text());
            qDebug() << query.lastError();
        }

        // 创建歌曲记录表格
        qstl = "create table if not exists songhistory(id integer primary key autoincrement, FileName text , EMixSongID text);";
        ret = query.exec(qstl);
        if (!ret)
            QMessageBox::critical(nullptr, "create table songhistory", query.lastError().text());

        // 查询歌曲历史记录表中的数据并显示
        qstl = "select * from songhistory";
        if (!query.exec(qstl)) // 执行查询操作
        {
            // 查询失败，显示错误信息
            QMessageBox::critical(nullptr, "错误", query.lastError().text());
        }
        while (query.next()) // 遍历查询结果
        {
            QSqlRecord rec = query.record(); // 获取查询结果的记录
            int singer_song_namekey = rec.indexOf("FileName"); // 获取歌曲名字段在查询结果中的索引
            QString strshow = rec.value(singer_song_namekey).toString(); // 构造要显示的字符串
            QListWidgetItem *item = new QListWidgetItem(strshow); // 创建列表项
            ui->lw_record->addItem(item); // 添加列表项到列表控件中
        }
    }
}

//初始化音乐
void OnlineMp3::initMusic()
{
    //播放操作
    player = new QMediaPlayer(this);
    playerlist = new QMediaPlaylist(this);
    player->setVolume(50);// 设置音量
    ui->hs_sound->setValue(50); // 设置音量滚动条

    //更新播放进度条
    connect(player, &QMediaPlayer::positionChanged,
            this, [&](qint64 duration)
    {
        if (ui->hs_songtime->isSliderDown())//正处于 手动调整状态，不处理
            return ;
        ui->hs_songtime->blockSignals(true);
        ui->hs_songtime->setSliderPosition(int(duration));// 设置播放进度条的当前值为当前播放位置
        ui->hs_songtime->blockSignals(false);

        // 设置播放进度条的范围为 0 到 播放器的总时长
        ui->hs_songtime->setRange(0, player->duration());
    });

    //绑定显示歌词
    connect(this, &OnlineMp3::lyricShow,
            this, [&](QString str)
    {
        ui->tb_songtext->setText(str);
    });
}

//访问http网页
void OnlineMp3::httpAccess(QString url)
{
    request = new QNetworkRequest;//实例化网络请求操作事项
    request->setUrl(url);//将url网页地址存入request请求中

    manager = new QNetworkAccessManager; //实例化网络管理（访问）
    manager->get(*request); //通过get,上传具体的请求
    //当网页回复消息，出发finish信号，读取数据
    connect(manager, &QNetworkAccessManager::finished,
            this, &OnlineMp3::netReply);
}

//读取网络数据
void OnlineMp3::netReply(QNetworkReply *reply)
{
    // 响应的状态码为200, 表示请求成功
    int status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute)
                      .toInt();

    qDebug() << "请求方式:" << reply->operation(); // 请求方式
    qDebug() << "响应状态码:" << status_code; // 状态码
    qDebug() << "url:" << reply->url(); // 请求的url
    qDebug() << "响应头:" << reply->rawHeaderList(); // header

    if ( reply->error() != QNetworkReply::NoError || status_code != 200 )
        QMessageBox::warning(this, "提示", "请求数据失败！", QMessageBox::Ok);
    else
    {
        // 如果没有发生网络错误，则读取响应数据
        QByteArray data = reply->readAll();
        // 发射自定义的 finish 信号，将响应数据传递给槽函数
        emit finish(data);
    }

    reply->deleteLater();
}

//获取搜索的url
QString OnlineMp3::getSearchUrl(QString musicName)
{
    QDateTime time = QDateTime::currentDateTime();
    // 将当前时间转换为自纪元以来的秒数，并将其转换为字符串
    QString currentTimeString = QString::number(time.toSecsSinceEpoch() * 1000);

    // 构建签名列表
    QStringList signature_list;
    signature_list << "NVPh5oo715z5DIWAeQlhMDsWXXQV4hwt"
                   << "appid=1014"
                   << "bitrate=0"
                   << "callback=callback123"
                   << "clienttime=" + currentTimeString
                   << "clientver=1000"
                   << "dfid=08wNhe1SuXFi3mgjH54fVBZD"
                   << "filter=10"
                   << "inputtype=0"
                   << "iscorrection=1"
                   << "isfuzzy=0"
                   << "keyword=" + musicName
                   << "mid=2059bfd9bdb82110cdc4426ae85da0fd"
                   << "page=1"
                   << "pagesize=30"
                   << "platform=WebFilter"
                   << "privilege_filter=0"
                   << "srcappid=2919"
                   << "token="
                   << "userid=0"
                   << "uuid=2059bfd9bdb82110cdc4426ae85da0fd"
                   << "NVPh5oo715z5DIWAeQlhMDsWXXQV4hwt";

    // 将签名列表中的元素连接成一个字符串
    QString string = signature_list.join("");
    //qDebug()<< string;
    //生成 MD5 哈希
    QByteArray hashedData = QCryptographicHash::hash(string.toUtf8(), QCryptographicHash::Md5);
    QString signaturecode = hashedData.toHex();

    // 根据用户输入的 MP3 名称发起操作请求
    QString url = kugouSearchApi + QString("callback=callback123"
                                           "&srcappid=2919"
                                           "&clientver=1000"
                                           "&clienttime=%1"
                                           "&mid=2059bfd9bdb82110cdc4426ae85da0fd"
                                           "&uuid=2059bfd9bdb82110cdc4426ae85da0fd"
                                           "&dfid=08wNhe1SuXFi3mgjH54fVBZD"
                                           "&keyword=%2"
                                           "&page=1"
                                           "&pagesize=30"
                                           "&bitrate=0"
                                           "&isfuzzy=0"
                                           "&inputtype=0"
                                           "&platform=WebFilter"
                                           "&userid=0"
                                           "&iscorrection=1"
                                           "&privilege_filter=0"
                                           "&filter=10"
                                           "&token="
                                           "&appid=1014"
                                           "&signature=%3"
                                          )
                  .arg(currentTimeString)
                  .arg(musicName)
                  .arg(signaturecode);
    return url;
}

//解析json数据
void OnlineMp3::hashJsonAnalysis(QByteArray JsonData)
{
    musicUrl = "";//置空防止解析错误json时放上一首歌曲
    //移除callback123()
    // 找到第一个左括号 "(" 的位置
    if (!isdoubleClick) //不是双击播放歌曲
    {
        int leftBracketIndex = JsonData.indexOf('(');
        if (leftBracketIndex != -1)
        {
            // 找到最后一个右括号 ")" 的位置
            int rightBracketIndex = JsonData.lastIndexOf(')');
            if (rightBracketIndex != -1)
            {
                // 提取 JSON 数据，只要包裹的部分
                JsonData = JsonData.mid(leftBracketIndex + 1, rightBracketIndex - leftBracketIndex - 1);
            }
        }
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(JsonData, &err);
    if (err.error != QJsonParseError::NoError)
        return;

    if (doc.isObject()) // 如果解析后的对象是一个 JSON 对象
    {
        QJsonObject data = doc.object(); // 获取 JSON 对象中的"data"字段
        if (data.contains("data")) // 如果"data"字段存在
        {
            QJsonObject objectInfo = data.value("data").toObject(); // 获取"data"字段中的对象
            //1--------------------------------------------------------------------------
            // 如果包含歌词，发送歌词显示信号
            if (objectInfo.contains("lyrics"))
                emit lyricShow(objectInfo.value("lyrics").toString());
            // 返回音乐播放 URL
            if (objectInfo.contains("play_url"))
            {
                musicUrl = objectInfo.value("play_url").toString();
                qDebug() << "音乐播放url" << objectInfo.value("play_url").toString();
            }

            //2--------------------------------------------------------------------------
            if (objectInfo.contains("lists")) // 如果"lists"字段存在
            {
                QJsonArray objectHash = objectInfo.value("lists").toArray(); // 获取"lists"字段中的数组
                for (int i = 0; i < objectHash.count(); i++) // 遍历数组中的每个元素
                {
                    QString singer_song_name, EMixSongID;
                    QJsonObject album = objectHash.at(i).toObject(); // 获取数组元素中的对象

                    // 从对象中获取歌曲名、歌手名、专辑 ID 和哈希值
                    if (album.contains("FileName"))
                        singer_song_name = album.value("FileName").toString();
                    if (album.contains("EMixSongID"))
                        EMixSongID = album.value("EMixSongID").toString();
                    // 将解析出的信息插入数据库
                    QSqlQuery query;
                    QString sql = QString("insert into songlist values(%1,'%2','%3');").arg(QString::number(i)).arg(singer_song_name).arg(EMixSongID);
                    if (!query.exec(sql)) // 如果插入数据库失败
                        QMessageBox::critical(nullptr, "插入数据库错误", query.lastError().text());
                    // 在搜索展示框中显示歌曲名称和歌手名称
                    QListWidgetItem *item = new QListWidgetItem(singer_song_name);
                    ui->lw_search->addItem(item);
                }
            }
        }
    }
}

//关闭
void OnlineMp3::on_btn_close_clicked()
{
    close();
}

//搜索歌曲
void OnlineMp3::on_btn_search_clicked()
{
    // 清空搜索队列
    ui->lw_search->clear();

    // 清理数据库中已经存储的 hash 等数据
    QSqlQuery query;
    QString sql = "delete from songlist;" ;

    if (!query.exec(sql))
        QMessageBox::critical(nullptr, "错误", query.lastError().text());

    // 根据用户输入的 MP3 名称发起操作请求
    QString url = getSearchUrl(ui->le_search->text());

    // 发起 HTTP 请求
    httpAccess(url);

    QByteArray JsonData;
    QEventLoop loop;//事件循环类

    // 等待 HTTP 请求完成并获取数据
    auto c = connect(this, &OnlineMp3::finish, [&](const QByteArray &data)
    {
        JsonData = data;
        loop.exit(1);
    });
    loop.exec();
    disconnect(c);

    // 解析获取的 JSON 数据
    hashJsonAnalysis(JsonData);
}

//音乐歌曲的下载和播放
void OnlineMp3::downloadPlayer(QString encode_album_audio_id)
{
    //构建下载歌曲的 URL
    QString url = getDownloadUrl(encode_album_audio_id);

    // 发起 HTTP 请求获取歌曲数据
    httpAccess(url);
    QByteArray jsonData;
    QEventLoop loop;

    //等待 HTTP 请求完成并获取数据
    auto d = connect(this, &OnlineMp3::finish, this, [&](const QByteArray &data)
    {
        jsonData = data;
        loop.exit(1);
    });
    loop.exec();
    disconnect(d);

    isdoubleClick = true;
    hashJsonAnalysis(jsonData);// 解析获取的 JSON 数据
    isdoubleClick = false;

    player->setMedia(QUrl(musicUrl));// 设置媒体并播放音乐

    player->play();// 播放音乐
    ui->btn_start_stop->setIcon(QIcon(":/res/stop.svg"));
}

//获取下载的url
QString OnlineMp3::getDownloadUrl(QString encode_album_audio_id)
{
    //signature---------------------------------
    QDateTime time = QDateTime::currentDateTime();
    // 将当前时间转换为自纪元以来的秒数，并将其转换为字符串
    QString currentTimeString = QString::number(time.toSecsSinceEpoch() * 1000);
    // currentTimeString = "1713782920612";
    // QString encode_album_audio_id = "j5yn384";
    // 构建签名列表
    QStringList signature_list;
    signature_list << "NVPh5oo715z5DIWAeQlhMDsWXXQV4hwt"
                   << "appid=1014"
                   << "clienttime=" + currentTimeString
                   << "clientver=20000"
                   << "dfid=08wNhe1SuXFi3mgjH54fVBZD"
                   << "encode_album_audio_id=" + encode_album_audio_id
                   << "mid=2059bfd9bdb82110cdc4426ae85da0fd"
                   << "platid=4"
                   << "srcappid=2919"
                   << "token="
                   << "userid=0"
                   << "uuid=2059bfd9bdb82110cdc4426ae85da0fd"
                   << "NVPh5oo715z5DIWAeQlhMDsWXXQV4hwt";

    // 将签名列表中的元素连接成一个字符串
    QString string = signature_list.join("");
    //qDebug()<< string;
    //生成 MD5 哈希
    QByteArray hashedData = QCryptographicHash::hash(string.toUtf8(), QCryptographicHash::Md5);

    //开始构建url---------------------------------
    QString signaturecode = hashedData.toHex();// 将哈希数据转换为十六进制字符串
    QString url = kugouDownldadApi + QString("srcappid=2919"
                  "&clientver=20000"
                  "&clienttime=%1"
                  "&mid=2059bfd9bdb82110cdc4426ae85da0fd"
                  "&uuid=2059bfd9bdb82110cdc4426ae85da0fd"
                  "&dfid=08wNhe1SuXFi3mgjH54fVBZD"
                  "&appid=1014"
                  "&platid=4"
                  "&encode_album_audio_id=%2"
                  "&token="
                  "&userid=0"
                  "&signature=%3")
                  .arg(currentTimeString)
                  .arg(encode_album_audio_id)
                  .arg(signaturecode);
    return url;
}

//切换歌曲
void OnlineMp3::switchSong()
{
    QButtonGroup *btnGroup = new QButtonGroup(this);
    btnGroup->addButton(ui->btn_lastsong, 0);
    btnGroup->addButton(ui->btn_nextsong, 1);
    connect(btnGroup, &QButtonGroup::idClicked,
            [ = ](int id)
    {
        int curRow = ui->lw_record->currentRow();
        if (id == 0)
        {
            // 当前行数减一
            curRow--;
            // 如果行数为负数，则将其设置为列表的最后一行
            if (curRow < 0)
                curRow = ui->lw_record->count() - 1;
        }
        else
        {
            // 当前行数减一
            curRow++;

            // 如果行数为边界下面，则将其设置为列表的第一行
            if (curRow == ui->lw_record->count())
                curRow = 0;
        }
        ui->lw_record->setCurrentRow(curRow);;//选中当要播放的行
        // 执行数据库查询
        QSqlQuery query;
        // 构建查询语句，查询指定行的歌曲信息
        QString sql = QString("select * from songhistory where id = %1;").arg(curRow + 1);
        if (!query.exec(sql))
            QMessageBox::critical(nullptr, "上一曲", query.lastError().text());

        QString EMixSongID;
        // 循环读取查询结果
        while (query.next())
        {
            // 获取查询结果中"EMixSongID"字段的值
            QSqlRecord lastrecord = query.record();
            int EMixSongIDkey = lastrecord.indexOf("EMixSongID");
            EMixSongID = query.value(EMixSongIDkey).toString();
        }
        // 调用下载播放器函数，传入EMixSongID参数
        downloadPlayer(EMixSongID);
    });
}

//播放或者暂停
void OnlineMp3::on_btn_start_stop_clicked()
{
    // 如果播放器状态为播放状态，则暂停播放
    if (player->state() == QMediaPlayer::PlayingState)
    {
        ui->btn_start_stop->setIcon(QIcon(":/res/start.svg"));
        player->pause();
    }
    // 如果播放器状态为暂停状态，则开始播放
    else if (player->state() == QMediaPlayer::PausedState)
    {
        ui->btn_start_stop->setIcon(QIcon(":/res/stop.svg"));
        player->play();
    }
}

//循环播放
void OnlineMp3::on_btn_loop_clicked()
{
    bool isChecked = ui->btn_loop->isChecked();

    if (isChecked)
    {
        // 连接播放器的 stateChanged 信号到匿名函数
        // 当播放状态改变时执行相应操作
        sig = connect(player, &QMediaPlayer::stateChanged, this, [ = ](QMediaPlayer::State newState)
        {
            // 如果播放器状态变为停止状态
            if (newState == QMediaPlayer::StoppedState)
            {
                player->setPosition(0);// 将播放位置设置为0
                player->play(); // 重新开始播放
            }
        });
    }
    else // 如果按钮未被选中，断开之前的连接
        disconnect(sig);
}

//音量调节
void OnlineMp3::on_hs_sound_valueChanged(int value)
{
    //设置播放音量
    player->setVolume(value);
    //显示音量字符串
    ui->lb_sound->setText(QString::number(value));
}

//进度条
void OnlineMp3::on_hs_songtime_valueChanged(int value)
{
    //设置时间
    QTime time(0, value / 60000, qRound((value % 60000) / 1000.0));
    ui->lb_time->setText(time.toString("mm:ss"));

    player->blockSignals(true);
    player->setPosition(value);
    player->blockSignals(false);
}

void OnlineMp3::on_btn_min_clicked()
{
    QMessageBox::information(nullptr, "About", "MP3网络搜索引擎\n"
                             "zyh\n");
}

//双击播放历史歌曲
void OnlineMp3::on_lw_record_itemDoubleClicked(QListWidgetItem *item)
{
    Q_UNUSED(item);

    // 获取当前列表中双击的歌曲索引，即数据表的 ID 号
    int curRow = ui->lw_record->currentRow();
    // 执行数据库查询，获取对应 ID 的歌曲信息
    QSqlQuery query;
    QString sql = QString("select * from songhistory where id = %1")
                  .arg(curRow + 1);//自动增长值从1开始自增
    if (!query.exec(sql))
        QMessageBox::critical(nullptr, "Error executing query", query.lastError().text());

    QString EMixSongID; // 用于存储查询到的歌曲的 EMixSongID
    // 遍历查询结果集中的每一条记录
    while (query.next())
    {
        QSqlRecord record = query.record();
        EMixSongID = record.value("EMixSongID").toString(); // 获取 EMixSongID 字段的值，并存储到 EMixSongID 变量中
    }
    // 根据获取到的 EMixSongID 播放选中的音乐
    downloadPlayer(EMixSongID);
}

//双击播放搜索歌曲
void OnlineMp3::on_lw_search_itemDoubleClicked(QListWidgetItem *item)
{
    Q_UNUSED(item);

    // 获取双击的歌曲索引，即数据表的 ID 号
    int searchRow = ui->lw_search->currentRow();

    QSqlQuery query;
    QString sql = QString("select * from songlist where id = %1;").arg(searchRow);
    if (!query.exec(sql))
        QMessageBox::critical(nullptr, "songlist表中找不到歌曲id", query.lastError().text());

    // 将选中的音乐的数据信息存入历史数据表
    QString singer_song_name, EMixSongID;
    if (query.next())
    {
        QSqlRecord record = query.record();

        singer_song_name = query.value("FileName").toString();
        EMixSongID = query.value("EMixSongID").toString();

        // 查询历史数据表中是否已经存在该歌曲的记录
        QSqlQuery checkQuery;
        sql = QString("select EMixSongID from songhistory where EMixSongID = '%1';").arg(EMixSongID);
        if (!checkQuery.exec(sql))
            QMessageBox::critical(nullptr, "select hash from songhistory where EMixSongID =", query.lastError().text());
        // 如果不存在该记录，则将其存入历史数据表
        if (checkQuery.next() == false)
        {
            sql = QString("insert into songhistory values(NULL, '%1', '%2')").arg(singer_song_name).arg(EMixSongID);
            if (!checkQuery.exec(sql))
                QMessageBox::critical(nullptr, "insert error", query.lastError().text());
            // 将歌手和歌名放入历史歌曲表中显示
            QListWidgetItem *item = new QListWidgetItem(singer_song_name);
            ui->lw_record->addItem(item);
        }
    }
    // 播放选中的音乐
    downloadPlayer(EMixSongID);
}
