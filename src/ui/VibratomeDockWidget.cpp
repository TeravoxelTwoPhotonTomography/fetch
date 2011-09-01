#include "VibratomeDockWidget.h"
#include "devices\microscope.h"
#include "AgentController.h"

namespace fetch {
namespace ui {



  VibratomeDockWidget::VibratomeDockWidget(device::Microscope *dc, MainWindow *parent)
    :QDockWidget("Vibratome",parent)
    ,dc_(dc)
    ,cmb_feed_axis_(NULL)
  {
    QWidget *formwidget = new QWidget(this);
    QFormLayout *form = new QFormLayout;
    formwidget->setLayout(form);
    setWidget(formwidget);

    parent->_vibratome_amp_controller->createLineEditAndAddToLayout(form);
    parent->_vibratome_feed_distance_controller->createLineEditAndAddToLayout(form);
    parent->_vibratome_feed_velocity_controller->createLineEditAndAddToLayout(form);

    cmb_feed_axis_ = new QComboBox;
    cmb_feed_axis_->addItem("X",(unsigned int)device::Vibratome::FeedAxis::Vibratome_VibratomeFeedAxis_X);
    cmb_feed_axis_->addItem("Y",(unsigned int)device::Vibratome::FeedAxis::Vibratome_VibratomeFeedAxis_Y);
    cmb_feed_axis_->setEditable(false);
    switch(dc->vibratome()->_config->feed_axis())
    { case device::Vibratome::FeedAxis::Vibratome_VibratomeFeedAxis_X: cmb_feed_axis_->setCurrentIndex(0); break;
      case device::Vibratome::FeedAxis::Vibratome_VibratomeFeedAxis_Y: cmb_feed_axis_->setCurrentIndex(1); break;
      default:
        error("%s(%d) Invalid value.  Should not be here."ENDL);
    }
    form->addRow("Feed Axis",cmb_feed_axis_);
    connect(cmb_feed_axis_,SIGNAL(currentIndexChanged(int)),this,SLOT(feedAxisChanged(int)));
    
    QHBoxLayout *row = new QHBoxLayout();
    
    QPushButton *b = new QPushButton("&Start",this);
    connect(b,SIGNAL(clicked()),this,SLOT(start()));
    row->addWidget(b);

    b = new QPushButton("Sto&p",this);
    connect(b,SIGNAL(clicked()),this,SLOT(stop()));
    row->addWidget(b);

    form->addRow(row);

    AgentControllerButtonPanel *btns = new AgentControllerButtonPanel(&parent->_scope_state_controller,&dc->cut_task);
    form->addRow(btns);

    connect(btns->controller(),SIGNAL(onRun()),this,SIGNAL(onRun()));
  }


  void VibratomeDockWidget::feedAxisChanged(int n)
  { 
    QVariant v = cmb_feed_axis_->itemData(n);
    device::Vibratome::FeedAxis a = (device::Vibratome::FeedAxis)v.toInt();
    dc_->vibratome()->setFeedAxis(a);
  }

}} //end namespace fetch::ui
