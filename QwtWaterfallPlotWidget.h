#ifndef QWT_WATERFALL_PLOT_WIDGET_H
#define QWT_WATERFALL_PLOT_WIDGET_H
//System includes

//Library includes
#include <QWidget>

//Local includes

namespace Ui {
class cQwtWaterfallPlotWidget;
}

class cQwtWaterfallPlotWidget : public QWidget
{
    Q_OBJECT

public:
    explicit cQwtWaterfallPlotWidget(QWidget *pParent = 0);
    ~cQwtWaterfallPlotWidget();

    void addData(QVector<float> qvfData);

private:
    Ui::cQwtWaterfallPlotWidget *m_pUI;
};

#endif // QWT_WATERFALL_PLOT_WIDGET_H
