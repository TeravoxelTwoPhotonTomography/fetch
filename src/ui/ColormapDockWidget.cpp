#include "ColormapDockWidget.h"
#include "MainWindow.h"

namespace fetch {
namespace ui {

#define ENDL "\r\n"
#define TRY(e) if (!(e)) qFatal("%s(%d): "ENDL "\tExpression evaluated as false."ENDL "\t%s",__FILE__,__LINE__,#e)

  const char fetch::ui::ColormapDockWidget::defaultSettingsRoot[] = "Colormaps/Config/";

  ColormapDockWidget::ColormapDockWidget(MainWindow *parent)
    :QDockWidget("Color Mapping",parent)
  { QWidget* w = new QWidget(this);
    //QLayout* layout = new QVBoxLayout;
    QFormLayout *layout = new QFormLayout;

    _display = new QLabel;
    _display->setScaledContents(true);
    TRY( connect(this,SIGNAL(setCmap(const QPixmap&)),_display,SLOT(setPixmap(const QPixmap&))) );
    layout->addWidget(_display);
  
    QHBoxLayout* row = new QHBoxLayout;    
    _selector = new QComboBox;
    _selector->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Fixed);
    // Add the built-in colormap resources
    QDir dir(":/cmap");
    QFileInfoList cmaps = dir.entryInfoList(QDir::Files);
    foreach(const QFileInfo& info,cmaps)
    { const QString name = info.absoluteFilePath();
      QPixmap im(name,"r");
      QIcon   icon(im.scaled(32,32));
      _selector->addItem(icon,name);                 
    }
    TRY(connect(_selector,SIGNAL(currentIndexChanged(const QString&)),
                this,    SIGNAL(colormapSelectionChanged(const QString&))));
    TRY(connect(_selector,SIGNAL(currentIndexChanged(const QString&)),
                this,    SLOT(updateLabel(const QString&))));
    row->addWidget(_selector);

    QPushButton* btnLoad = new QPushButton("Open");
    btnLoad->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Fixed);
    row->addWidget(btnLoad); //,0,Qt::AlignRight);
    TRY(connect(btnLoad,SIGNAL(clicked()),
                this,   SLOT(loadCmapFromFileDialog())));
    layout->addRow("Colormap: ",row);

    //QFormLayout *form = new QFormLayout;
    _gamma = new QLineEdit("1.0");
    _gamma->setValidator(new QDoubleValidator(0.0,1.0,2,_gamma));       // Add validated input
    QCompleter *c = new QCompleter(new QStringListModel(_gamma),_gamma);// Add completion based on history
    c->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    c->setModelSorting(QCompleter::UnsortedModel);
    _gamma->setCompleter(c);
    TRY(connect(_gamma,SIGNAL(editingFinished()),
                this,  SLOT(gammaEditingFinshed())));
    layout->addRow("Gamma: ",_gamma);

    //layout->addItem(form);
          
    w->setLayout(layout);
    setWidget(w);
  }

  void ColormapDockWidget::updateLabel(const QString& name)
  { QPixmap im(name,"r");
    _display->setPixmap(im);
  }

  void ColormapDockWidget::loadCmapFromFileDialog()
  { QSettings settings;
    QString key = QString(defaultSettingsRoot)+QString("LastLoad"); 
    QString filter("Images (");

    QList<QByteArray> fmts = QImageReader::supportedImageFormats();
    foreach(QByteArray fmt,fmts)
    { filter += QString("*.%1 ").arg(QString(fmt));
    }
    filter += ");;All files (*.*)";
    qDebug() << filter;
    QString filename = QFileDialog::getOpenFileName(this,
      tr("Load Colormap"),
      settings.value(key,QDir::currentPath()).toString(),
      filter);
    if(filename.isEmpty())
      return;

    loadColormapFromFile(filename);    
    settings.setValue(key, QDir::current().absoluteFilePath(filename));
  }

  void ColormapDockWidget::loadColormapFromFile(const QString& filename)
  { int count = _selector->count();
    // 1. add to combo box
    QPixmap im(filename,"r");
    QIcon   icon(im.scaled(32,32));
    _selector->addItem(icon,filename); 
    // 2. select it in combobox
    _selector->setCurrentIndex(count);
  }

  void ColormapDockWidget::gammaEditingFinshed()
  { bool ok;
    double v = QStringToValue<double>(_gamma->text(),&ok);
    if(ok)
      emit gammaChanged((float)v);
  }

  
//end namespace fetch::ui
}
}
