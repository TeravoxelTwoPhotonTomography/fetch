#define QT_COMPILING_QSTRING_COMPAT_CPP
#include <QApplication>
#include <QTcpSocket>
#include <QTcpServer>

#include <QMainWindow>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QPushButton>

#include <functional>

#include "MicroscopeTcpServerDockWidget.h"
#include "devices\microscope.h"
#include "AgentController.h"


namespace fetch {
namespace ui {


typedef QHash<QTcpSocket*, QByteArray*>  buffers_t;
typedef QHash<QTcpSocket*,bool>          validation_state_t;

typedef std::function<void(QTcpSocket*,QList<QString> & args)> callback_t;
typedef QHash<QString,callback_t>        protocol_t;

static void read(QTcpSocket* socket,buffers_t &buffers, QTcpServer &server,const protocol_t& protocol) {
    auto *buffer = buffers.value(socket);
    qDebug()<<"Reading from client";
    while (socket->bytesAvailable() > 0) {
        buffer->append(socket->readAll());
        auto s=QString::fromUtf8(buffer->data());
        auto keeplast=!s.endsWith("\r\n");
        auto cmds=s.split("\r\n",QString::SkipEmptyParts);
        buffer->clear();        
        if(keeplast) {
            auto a=cmds.takeLast();
            auto b=a.toUtf8();
            buffer->append(b);
        }
            
        qDebug()<<"[Socket "<<socket<<"] "<<cmds;
        for(auto cmd:cmds) {
            auto parts=cmd.split(' ');
            auto c=parts.first();
            parts.pop_front();
            if(protocol.contains(c)) {
                qDebug()<<"\tProcessing: "<<cmd;
                qDebug()<<"\t\tCmd : "<<c;
                qDebug()<<"\t\tArgs: "<<parts;
                protocol[c](socket,parts);
                socket->write("Done\r\n");
            } else {
                qDebug()<<"\tReceived: "<<cmd;
                qDebug()<<"\tDid not match any of "<<protocol.keys();
                socket->write("Command not recognized\r\n");
            }
        }        
    }
}

static void disconnect(QTcpSocket* socket, buffers_t &buffers) {
    qDebug()<<"disconnect";
    auto *buffer = buffers.value(socket);
    socket->deleteLater();
    delete buffer;
}

enum server_state_t{
    ServerRunning,
    ServerStopped
};

static const char * get_start_button_label(server_state_t state) {
    const char* s[]={"Stop","Start"};
    return s[state];
}

/* FIXME:
    arm/disarm/track microscope tasks?
*/

class device_t {
    enum device_state_t{
        DeviceRunning=0,
        DeviceStopped
    };
    QHash<Task*,QString> tasknames;
    QHash<QString,Task*> tasksByName;
    device::Microscope *dc;
    AgentController *ac;
public:
    device_t(device::Microscope* dc, AgentController *ac): dc(dc),ac(ac) {
        tasknames[&dc->interaction_task]="Video";
        tasknames[&dc->stack_task]      ="Stack";
        tasknames[&dc->time_series_task]="TimeSeries";
        tasknames[&dc->auto_tile_task]  ="AutoTileAcquisition";
        for(auto e=tasknames.constBegin();e!=tasknames.constEnd();++e)
            tasksByName[e.value()]=e.key();
    }
    inline device_state_t state() const {
        return dc->__self_agent.is_running()?DeviceRunning:DeviceStopped;
    }
    void run()  { ac->run();}
    void stop() { ac->stop();}
    QString task() const { return tasknames[dc->__self_agent._task]; };
    QList<QString> tasks() const { return tasknames.values(); }
    void set_task(const QString taskname) {
        auto t=tasksByName.value(taskname,0);
        if(t)
            ac->arm(t);
    }
};

class server_t {
    server_state_t state_;
    device_t *device;
    protocol_t protocol;
    validation_state_t validation_state; //Keep track of password-validation state for each connection
    QTcpServer s;
    buffers_t buffers; //We need a buffer to store data until command has completely received
public:
    explicit server_t(device_t *device,unsigned port=1024)
        : state_(ServerStopped)
        , device(device)
    {
        make_protocol();
        s.connect(&s,&QTcpServer::newConnection,[this](){this->on_connection();});
    }
    void toggle_state() {
        switch (state()) {
            case ServerRunning: stop(); break;
            case ServerStopped: start(); break;
            default:;
        }
        if(s.isListening())
            state_=ServerRunning;
        else
            state_=ServerStopped;
    }
    inline server_state_t state() const{ return state_; }
private:
    void on_connection() {
        while (s.hasPendingConnections())
        {
            qDebug()<<"Accepting connections...";
            auto *socket = s.nextPendingConnection();
            qDebug()<<"Peer address: "<<socket->peerAddress();
            s.connect(socket, &QTcpSocket::readyRead, [this,socket](){
                read(socket,this->buffers,this->s,this->protocol);
            });
            s.connect(socket, &QTcpSocket::disconnected, [this,socket]() {
                disconnect(socket,this->buffers);
            });
            buffers.insert(socket, new QByteArray());
            qDebug()<<"...accepted"<<socket;
        }
    }
    void start() {        
        qDebug() << "Listening:" << s.listen(QHostAddress::Any,1024);
    }
    void stop() {
        validation_state.empty();
        buffers.empty();
        s.close();
        qDebug()<<"No longer accepting connections.";
    }     
    auto is_validated(QTcpSocket *socket)->bool{
        if(validation_state.contains(socket))
            return validation_state[socket];
        return false;
    };
    void make_protocol() {
        protocol.insert("status",[this](QTcpSocket *socket,QList<QString> & args){
            const char* s[]={
                "running",
                "stopped"
            };
            if(!is_validated(socket)) return;
            auto resp=QString("Task: %1 State: %2\r\n").arg(this->device->task(),s[this->device->state()]);
            socket->write(resp.toUtf8());
        });
        protocol.insert("stop",[this](QTcpSocket *socket,QList<QString> & args) {
            if(!is_validated(socket)) return;
            this->device->stop();
        });
        protocol.insert("run",[this](QTcpSocket *socket,QList<QString> & args) {
            if(!is_validated(socket)) return;
            this->device->run();
        });
        protocol.insert("arm",[this](QTcpSocket *socket,QList<QString> & args) {
            if(!is_validated(socket)) return;            
            this->device->set_task(args[0]);
        });
        protocol.insert("tasks",[this](QTcpSocket *socket,QList<QString> & args) {
            if(!is_validated(socket)) return;
            auto ts=this->device->tasks();
            for(auto t:ts)
                socket->write((t+QString("\r\n")).toUtf8());
        });
        protocol.insert("help",[this](QTcpSocket *socket,QList<QString> & args) {
            if(!is_validated(socket)) return;
            QTextStream cmdlist(socket);
            for(auto k:this->protocol.keys())
                cmdlist<<k<<"\r\n";
        });
        protocol.insert("version0",[this](QTcpSocket* socket,QList<QString> & args) {
            this->validation_state[socket]=true;
        });
    }
};

class server_widget_t : public QWidget {
    server_t server;
public:
    explicit server_widget_t(device_t *device,unsigned port=1024)
        : server(device,port) 
    {        
        auto *layout=new QVBoxLayout;
        setLayout(layout);
        auto *w=new QPushButton(get_start_button_label(server.state()));
        layout->addWidget(w);
        w->connect(w,&QPushButton::clicked,[this,w](){
            qDebug()<<"clicked";
            this->server.toggle_state();
            w->setText(get_start_button_label(this->server.state()));
        });
    }
};


MicroscopeTcpServerDockWidget::MicroscopeTcpServerDockWidget(device::Microscope* dc,  AgentController *ac,MainWindow* parent)
    : QDockWidget("TCP Server",parent)
{
    setWidget(new server_widget_t(new device_t(dc,ac)));
}

}} // fetch::ui