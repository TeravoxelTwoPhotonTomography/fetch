#pragma once
#include <QtGui>
#include "devices\Microscope.h"
#include "devices\LinearScanMirror.h"
#include "devices\pockels.h"
#include "common.h"

// TODO
// [ ] Update other views when there's a change
//     e.g. when there are multiple line edit controls for one parameter...a change
//          in one is communicated to all, but only one update to the Microscope
//          config is made.
//
// NOTES
//
// The idea here is that the main GUI class instances controllers for
// each parameter.  The controllers are then used as factories to create
// widgets used to build various interface elements.
//
// This controller should facilitate construction of common controls for
// the parameters, event connection, etc... so that the only thing a "user"
// widget needs to do is something like:
//
//   ui::PockelsController *p = app->pock();
//   layout->addWidget(p->createLineEdit();
//
// Events are setup by the controller so that different UI elements providing
// different views of a parameter may be synchronized.
//
// [?]  layers of getters and setters for device parameters
//      1. here (IGetSet)
//      2. Device level
//      3. configuration object level 
//      3. hardware  level
//      
// ADDING A PROPERTY
//      1. Declare your IGetSet class 
//      2. Define your Get_ and Set_ functions
//      3. Define your createValidator_

namespace fetch {
namespace ui {

  class DevicePropControllerBase:public QObject
  {
    Q_OBJECT;
    protected slots:
      void report() {HERE;}
      virtual void setByLineEdit(QWidget *source)=0;
  };

  template<typename TDevice, typename TConfig, class TGetSetInterface> 
  class DevicePropController:public DevicePropControllerBase
  { 
    public:
      DevicePropController(TDevice *dc, char* label);

			QLineEdit *createLineEdit();
      QLineEdit *createLineEditAndAddToLayout(QFormLayout *layout);

		protected:
			void setByLineEdit(QWidget *source);
      
    protected:
			TGetSetInterface interface_;

    private:
      TDevice    *dc_;
      QString     label_;

      QSignalMapper lineEditSignalMapper_;

			QLineEdit  *le_;
			
  };

	template<typename TConfig> inline QString  ValueToQString(TConfig v);
	template<typename TConfig>        TConfig  QStringToValue(QString &s,bool *ok);

  /* Note:
   * Don't really need a virtual base class here because the interface is
   * implemented via templates.  It's mostly just to document what's
   * required.
   */

  template<typename TDevice, typename TConfig>
  struct IGetSet
	{ virtual void Set_(TDevice *dc, TConfig &v) = 0;
    virtual TConfig Get_(TDevice *dc)          = 0;
    virtual QValidator* createValidator_(QObject* parent) = 0;
  };
	
#define	DECL_GETSET_CLASS(NAME,TDC,T) \
	struct NAME:public IGetSet<TDC,T> \
	{ void Set_(TDC *dc, T &v);\
    T Get_(TDC *dc);\
    QValidator* createValidator_(QObject* parent);\
  }

  // Video

	DECL_GETSET_CLASS(GetSetResonantTurn,device::Microscope,f64);
	DECL_GETSET_CLASS(GetSetLines,device::Microscope,i32);
	DECL_GETSET_CLASS(GetSetLSMVerticalRange,device::LinearScanMirror,f64);
	DECL_GETSET_CLASS(GetSetPockels,device::Pockels,u32);

	typedef DevicePropController<device::Microscope,f64,GetSetResonantTurn>           ResonantTurnController;
	typedef DevicePropController<device::Microscope,i32,GetSetLines>                  LinesController;
	typedef DevicePropController<device::LinearScanMirror,f64,GetSetLSMVerticalRange> LSMVerticalRangeController;
	typedef DevicePropController<device::Pockels,u32,GetSetPockels>                   PockelsController;

  // Stack

  DECL_GETSET_CLASS(GetSetZPiezoMin ,device::ZPiezo,f64);
  DECL_GETSET_CLASS(GetSetZPiezoMax ,device::ZPiezo,f64);
  DECL_GETSET_CLASS(GetSetZPiezoStep,device::ZPiezo,f64);

  typedef DevicePropController<device::ZPiezo,f64,GetSetZPiezoMin>              ZPiezoMinController;
  typedef DevicePropController<device::ZPiezo,f64,GetSetZPiezoMax>              ZPiezoMaxController;
  typedef DevicePropController<device::ZPiezo,f64,GetSetZPiezoStep>             ZPiezoStepController;

}} //end fetch::ui

  ////////////////////////////////////////////////////////////////////////////
  //
  // Implementation
  //
  ////////////////////////////////////////////////////////////////////////////

namespace fetch {
namespace ui {
  
  template<typename TDevice, typename TConfig, class TGetSetInterface> 
  DevicePropController<TDevice,TConfig,TGetSetInterface>::DevicePropController(TDevice *dc, char* label)
	: dc_(dc)
  , label_(label)
  , le_(NULL)
  { 
    connect(&lineEditSignalMapper_,SIGNAL(mapped(QWidget*)),this,SLOT(setByLineEdit(QWidget*)));
  }

  template<typename TDevice, typename TConfig, class TGetSetInterface>
	void DevicePropController<TDevice,TConfig,TGetSetInterface>::setByLineEdit(QWidget* source)
	{ 
    bool ok;    
    QLineEdit* le = qobject_cast<QLineEdit*>(source);
    Guarded_Assert(le);

		TConfig v = QStringToValue<TConfig>(le->text(),&ok);
		if(ok)
			interface_.Set_(dc_,v);
	}

  template<typename TDevice, typename TConfig, class TGetSetInterface>
	QLineEdit* DevicePropController<TDevice,TConfig,TGetSetInterface>::createLineEdit()
	{ le_ = new QLineEdit(ValueToQString(interface_.Get_(dc_)));
		QValidator *v = interface_.createValidator_(le_);
		if(v)
			le_->setValidator(v);

    lineEditSignalMapper_.setMapping(le_,le_);
		connect(le_,SIGNAL(editingFinished()),&lineEditSignalMapper_,SLOT(map()));
    //connect(le_,SIGNAL(editingFinished()),this,SLOT(report()));
		return le_;
	}


  template<typename TDevice, typename TConfig, class TGetSetInterface>
  QLineEdit* DevicePropController<TDevice,TConfig,TGetSetInterface>::createLineEditAndAddToLayout( QFormLayout *layout )
  { QLineEdit *le = createLineEdit();
    layout->addRow(label_,le);
    return le;
  }


	template<typename TConfig> inline QString ValueToQString(TConfig v) {return QString().setNum(v);}
}} //end fetch::ui
