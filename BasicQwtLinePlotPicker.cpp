//System includes

//Library includes

//Local includes
#include "BasicQwtLinePlotPicker.h"

cBasicQwtLinePlotPicker::cBasicQwtLinePlotPicker(int iXAxis, int iYAxis, RubberBand oRubberBand, DisplayMode oTrackerMode, QWidget *pCanvas) :
    QwtPlotPicker(iXAxis, iYAxis, oRubberBand, oTrackerMode, pCanvas)
{
}

void cBasicQwtLinePlotPicker::setXUnit(const QString &qstrXUnit)
{
    m_qstrXUnit = qstrXUnit;
}

void cBasicQwtLinePlotPicker::setYUnit(const QString &qstrYUnit)
{
    m_qstrYUnit = qstrYUnit;
}

//QwtText cBasicQwtLinePlotPicker::trackerText(const QPoint &oPosition) const
//{
//    return QwtText(QString("%1 %2, %3 %4").arg(QString::number(oPosition.x())).arg(m_qstrXUnit).arg(QString::number(oPosition.y())).arg(m_qstrYUnit));
//}

QwtText cBasicQwtLinePlotPicker::trackerTextF(const QPointF &oPosition) const
{
    return QwtText(QString("%1 %2, %3 %4").arg(QString::number(oPosition.x())).arg(m_qstrXUnit).arg(QString::number(oPosition.y())).arg(m_qstrYUnit));
}
