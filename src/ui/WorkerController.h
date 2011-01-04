#pragma once
#include <QtGui>

// TODO
// [ ] Refactor to ParamController or some such..it's not just for workers
//     something like: DeviceParamController
// [ ] Update other views when there's a change
//     e.g. when there are multiple line edit controlls for one parameter...a change
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

namespace fetch {
namespace ui {

  template<typename TDevice, typename TConfig, class TGetSetInterface> 
  class WorkerController:public QObject
  { Q_OBJECT
    public:
      WorkerController(TDevice *dc, char* label, QValidator *validator=NULL);

			QLineEdit *createLineEdit(QValidator *validator=NULL);

		protected slots:
			void setByLineEdit();
      
    protected:
			TGetSetInterface interface_;
      //virtual void    Set_(TConfig value) = 0;      
      //virtual TConfig Get_()              = 0;


    private:
      TDevice    *dc_;
      QString     label_;
      QValidator *validator_;

			QLineEdit  *le_;

			inline QValidator* chooseValidator_(QValidator *optional_) {return optional_?optional_:validator_;}
  };

	template<typename TConfig> inline QString& ValueToQString(TConfig v);
	template<typename TConfig>        TConfig  QStringToValue(QString &s,bool *ok);

	template<typename TDevice, typename TConfig>
  struct IGetSet
	{ virtual void Set_(TDevice *dc, TConfig &v) = 0;
    virtual TConfig& Get_(TDevice *dc)         = 0;
  }
	
#define	DECL_GETSET_CLASS(NAME,TDC,T) \
	struct NAME:IGetSet<T> \
	{ virtual void Set_(TDC *dc, T &v);\
    virtual T& Get_(TDC *dc);\
  }

	DECL_GETSET_CLASS(GetSetResonantTurn,device::Microscope,f64);
	DECL_GETSET_CLASS(GetSetLines,device::Microscope,i32);
	DECL_GETSET_CLASS(GetSetLSMVerticalRange,device::LinearScanMirror,f64);
	DECL_GETSET_CLASS(GetSetPockels,device::Pockels,u32);

	typedef template<> WorkerController<device::Microscope,f64,GetSetResonantTurn>           ResonantTurnController;
	typedef template<> WorkerController<device::Microscope,i32,GetSetLines>                  LinesController;
	typedef template<> WorkerController<device::LinearScanMirror,f64,GetSetLSMVerticalRange> LSMVerticalRangeController;
	typedef template<> WorkerController<device::Pockels,u32,GetSetPockels>                   PockelsController;

}} //end fetch::ui

  ////////////////////////////////////////////////////////////////////////////
  //
  // Implementation
  //
  ////////////////////////////////////////////////////////////////////////////

namespace fetch {
namespace ui {
  
  template<typename TDevice, typename TConfig> 
  WorkerController::WorkerController(TDevice *dc, char* label, QValidator *validator)
	: dc_(dc)
  , label_(label)
  , validator_(validator)
  , le_(NULL)
  { 
  }

  template<typename TDevice, typename TConfig> 
	void WorkerController::setByLineEdit(void)
	{ bool ok;
		TConfig v = QStringToValue<TConfig>(le_.text(),&ok);
		if(ok)
			interface_.Set_(dc_,v);
	}

  template<typename TDevice, typename TConfig> 
	QLineEdit* WorkerController::createLineEdit(QValidator *validator)
	{ le_ = new QLineEdit(ValueToQString(interface.Get_(dc_)));
	  
		QValidator *v = chooseValidator_(validator);
		if(v)
			le_->setValidator(v);

		connect(le_,SIGNAL(editingFinished()),this,SLOT(setByLineEdit()));
		return le_;
	}

	template<typename TConfig> inline QString& ValueToQString(TConfig v) {return QString().setVal(v);}
}} //end fetch::ui
