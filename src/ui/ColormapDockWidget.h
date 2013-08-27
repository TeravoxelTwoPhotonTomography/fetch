#pragma once
#include <QtWidgets>



namespace fetch{
namespace ui{
    class MainWindow;

    class ColormapDockWidget:public QDockWidget
    {
      Q_OBJECT

      static const char defaultSettingsRoot[];
    public:
      ColormapDockWidget(MainWindow* parent);
      
      QString cmap();

    protected slots:     
      void loadColormapFromFile(const QString& filename);
      void updateLabel();
      void updateLabel(const QString &name);
      void loadCmapFromFileDialog();
      void gammaEditingFinshed();

    signals:
      void colormapSelectionChanged(const QString& name);
      void gammaChanged(float gamma);

      void setCmap(const QPixmap& pixmap);

    private:
      QLabel    *_display;
      QLineEdit *_gamma;
      QComboBox *_selector;
    };

//end namespace fetch::ui
}
}
