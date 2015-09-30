//System includes

//Library includes

//Local includes
#include "QwtWaterfallPlotWidget.h"
#include "ui_QwtWaterfallPlotWidget.h"

cQwtWaterfallPlotWidget::cQwtWaterfallPlotWidget(QWidget *pParent) :
    QWidget(pParent),
    m_pUI(new Ui::cQwtWaterfallPlotWidget)
{
    m_pUI->setupUi(this);
}

cQwtWaterfallPlotWidget::~cQwtWaterfallPlotWidget()
{
    delete m_pUI;
}

void cQwtWaterfallPlotWidget::addData(QVector<float> qvfData)
{

}



