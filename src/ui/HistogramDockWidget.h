#pragma once
#include <QtWidgets>
namespace mylib{
#include "array.h"
}

class QCustomPlot;

namespace fetch {
namespace ui {

class HistogramDockWidget: public QDockWidget
{ Q_OBJECT
	QCustomPlot      *plot_;
	size_t            ichan_;
	mylib::Array     *last_;
	bool              is_live_;
	QVector<double>   x_,pdf_,cdf_;
	double            minX_,maxX_,perct_;
	QLineEdit         *leMin_, *leMax_, *lePerct_;
	public:
		HistogramDockWidget(QWidget *parent=NULL);

	signals:
		void change_chan(int ichan);
		
	public slots:
		void set(mylib::Array *im);
		void set_ichan(int ichan);
		void set_live(bool is_live);
		void rescale_axes();
	void reset_minmax();
	void set_percentile(QString);
	private:
		void swap(mylib::Array *im);
		int  check_chan(mylib::Array *im);
		void compute(mylib::Array *im);

};

}} // namespace fetch::ui
