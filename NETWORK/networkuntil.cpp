#include "networkuntil.h"

#include "protocol.h"

#include <DATA/datauntil.h>

#include <QMessageBox>

#include <SHOW/mzplanclientwin.h>

#include <SERVER/timerserver.h>

NetWorkUntil &NetWorkUntil::getInstance()
{
    static NetWorkUntil instance;
    return instance;
}

void NetWorkUntil::regist(QString username,QString password)
{
    this->initTcp();
    PDU pdu;
    pdu.msgType=REGIST_REQUEST;
    QJsonObject data;
    data.insert(USERNAME,username);
    data.insert(PASSWORD,password);
    pdu.data=data;
    this->tcpSocket->write(pdu.toByteArray());
    if(!this->tcpSocket->waitForReadyRead()){
        //连接失败 说明网络不好 销毁并且设置同步信号为false
        QMessageBox::warning(&MZPlanClientWin::getInstance(),"网络连接","连接服务器失败，请检查网络或联系管理员");
        delete this->tcpSocket;
        this->tcpSocket=nullptr;
        DataUntil::getInstance().setSynchronize(false);
    }
}

void NetWorkUntil::login(QString username,QString password)
{
    this->initTcp();
    PDU pdu;
    pdu.msgType=LOGIN_REQUEST;
    QJsonObject data;
    data.insert(USERNAME,username);
    data.insert(PASSWORD,password);
    pdu.data=data;
    this->tcpSocket->write(pdu.toByteArray());
    if(!this->tcpSocket->waitForReadyRead()){
        //连接失败 说明网络不好 销毁并且设置同步信号为false
        QMessageBox::warning(&MZPlanClientWin::getInstance(),"网络连接","连接服务器失败，请检查网络或联系管理员");
        delete this->tcpSocket;
        this->tcpSocket=nullptr;
        DataUntil::getInstance().setSynchronize(false);
    }
}

void NetWorkUntil::cancle()
{
    this->initTcp();
    PDU pdu;
    pdu.msgType=CANCEL_REQUEST;
    QJsonObject data;
    data.insert(USERNAME,DataUntil::getInstance().username);
    pdu.data=data;
    this->tcpSocket->write(pdu.toByteArray());
    if(!this->tcpSocket->waitForReadyRead()){
        //连接失败 说明网络不好 销毁并且设置同步信号为false
        QMessageBox::warning(&MZPlanClientWin::getInstance(),"网络连接","连接服务器失败，请检查网络或联系管理员");
        delete this->tcpSocket;
        this->tcpSocket=nullptr;
        DataUntil::getInstance().setSynchronize(false);
    }
}

void NetWorkUntil::synchronize(bool status)
{
    this->status=status;
    this->initTcp();
    PDU pdu;
    pdu.msgType=SYNOCHRONIZE_PLAN_REQUEST;
    QJsonObject data;
    QString dbdata=DataUntil::getInstance().getDbData().toBase64();
    data.insert(DB_DATA,dbdata);
    data.insert(USERNAME,DataUntil::getInstance().username);
    data.insert(MEDIFYTIME,DataUntil::getInstance().medifyTime);
    pdu.data=data;
    this->tcpSocket->write(pdu.toByteArray());
    if(!this->tcpSocket->waitForReadyRead()){
        //连接失败 说明网络不好 销毁并且设置同步信号为false
        QMessageBox::warning(&MZPlanClientWin::getInstance(),"网络连接","连接服务器失败，请检查网络或联系管理员");
        delete this->tcpSocket;
        this->tcpSocket=nullptr;
        DataUntil::getInstance().setSynchronize(false);
    }
}

void NetWorkUntil::handleRegist(QJsonObject data)
{
    //收到了服务器传来的消息
    bool flag=data.value(CHECK).toBool();
    if(flag){
        //注册成功
        QMessageBox::information(&MZPlanClientWin::getInstance(),"注册","注册成功");
    }else{
        //注册失败
        QMessageBox::critical(&MZPlanClientWin::getInstance(),"注册","注册失败:"+data.value(MSG_STRING).toString());
    }
}

void NetWorkUntil::handleLogin(QJsonObject data)
{
    //收到了服务器传来的消息
    bool flag=data.value(CHECK).toBool();
    if(flag){
        //登陆成功
        //切换用户
        DataUntil::getInstance().changeUser(data.value(USERNAME).toString());
        DataUntil::getInstance().updateUser();
        //同步一次
        if(DataUntil::getInstance().isSynchronize)
            this->synchronize();
        //切换页面
        MZPlanClientWin::getInstance().on_userBtn_clicked();
    }else{
        //登陆失败
        QMessageBox::critical(&MZPlanClientWin::getInstance(),"登陆","登陆失败:"+data.value(MSG_STRING).toString());
    }
}

void NetWorkUntil::handleCancle(QJsonObject data)
{
    //收到了服务器传来的消息
    bool flag=data.value(CHECK).toBool();
    if(flag){
        //注销成功
        //删除当前用户的所有文件 在更换用户前进行删除
        DataUntil::getInstance().removeUserDir();
        //切换到本地用户
        DataUntil::getInstance().changeUser("local");
        DataUntil::getInstance().updateUser();

        //切换页面
        MZPlanClientWin::getInstance().on_userBtn_clicked();
        //登出
        this->logout();
        QMessageBox::information(&MZPlanClientWin::getInstance(),"注销","注销成功");
    }else{
        QMessageBox::critical(&MZPlanClientWin::getInstance(),"注销","注销失败:"+data.value(MSG_STRING).toString());
    }
}

void NetWorkUntil::handleSynchronize(QJsonObject data)
{
    if(data.contains(MEDIFYTIME)){
        //覆盖本地文件 覆盖之前要记得断开数据库
        QByteArray dbdata=QByteArray::fromBase64(data.value(DB_DATA).toString().toUtf8());
        DataUntil::getInstance().writeDbData(dbdata);
        //更新时间
        DataUntil::getInstance().updateMedifyTime(data.value(MEDIFYTIME).toString());
        //刷新数据
        QDate date=QDate::currentDate();
        if(!this->status){
            //用选择时间 初始化完毕
            date=MZPlanClientWin::getInstance().choiceDate;
            DataUntil::getInstance().reloadChoicePlan(date);
            DataUntil::getInstance().reloadCurrentPlan();
            //刷新界面
            MZPlanClientWin::getInstance().updateInterface();
        }
    }
    qDebug()<<data.value(MSG_STRING).toString();
}

void NetWorkUntil::logout()
{
    if(!this->tcpSocket){
        delete this->tcpSocket;
        this->tcpSocket=nullptr;
    }
}

NetWorkUntil::NetWorkUntil()
    :QObject(nullptr)
{
}

NetWorkUntil::~NetWorkUntil()
{

}

void NetWorkUntil::initTcp()
{
    if(!this->tcpSocket){
        this->tcpSocket=new QTcpSocket();
        this->tcpSocket->connectToHost(QHostAddress("192.168.63.1"),1314);
        //连接信号
        connect(this->tcpSocket,&QTcpSocket::disconnected,[this]{
            //失去连接就删除该对象
            if(this->tcpSocket){
                delete this->tcpSocket;
                this->tcpSocket=nullptr;
            }
        });
        connect(this->tcpSocket,&QTcpSocket::readyRead,this,&NetWorkUntil::handleTcpSocketReadyRead);//读数据
    }
    if(this->tcpSocket->state()!=QAbstractSocket::ConnectedState){
        DataUntil::getInstance().setSynchronize(false);
    }
}

void NetWorkUntil::handleTcpSocketReadyRead()
{
    PDU pdu=PDU(this->tcpSocket->readAll());
    switch(pdu.msgType){
    case REGIST_RESPONSE:
        this->handleRegist(pdu.data);
        break;
    case LOGIN_RESPONSE:
        this->handleLogin(pdu.data);
        break;
    case CANCEL_RESPONSE:
        this->handleCancle(pdu.data);
        break;
    case SYNOCHRONIZE_PLAN_RESPONSE:
        this->handleSynchronize(pdu.data);
        break;
    default:
        qDebug()<<"未知消息";
        break;
    }
}
