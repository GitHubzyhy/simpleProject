#include "mainwindow.h"
#include <QApplication>
#include "page_login.h"//登录

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    Page_Login m_dlgLogin;
    m_dlgLogin.show();

    //调用connect函数进行连接
    QObject::connect(&m_dlgLogin, &Page_Login::SendLoginSuccess,
                     &w, [&]
    {
        m_dlgLogin.close();
        w.show();//显示主窗口
    });

    return a.exec();
}
