//System includes

//Library includes

//Local includes
#include "BasicQwtLinePlotPositionPicker.h"

#if QWT_VERSION < 0x060100 //Account for Ubuntu's typically outdated package versions
cBasicQwtLinePlotPositionPicker::cBasicQwtLinePlotPositionPicker(int iXAxis, int iYAxis, RubberBand oRubberBand, DisplayMode oTrackerMode, QwtPlotCanvas *pCanvas) :
    QwtPlotPicker(iXAxis, iYAxis, oRubberBand, oTrackerMode, pCanvas)
  #else
cBasicQwtLinePlotPositionPicker::cBasicQwtLinePlotPositionPicker(int iXAxis, int iYAxis, RubberBand oRubberBand, DisplayMode oTrackerMode, QWidget *pCanvas) :
    QwtPlotPicker(iXAxis, iYAxis, oRubberBand, oTrackerMode, pCanvas)
  #endif

{
    setMousePattern( QwtEventPattern::MouseSelect1, Qt::NoButton);
}

void cBasicQwtLinePlotPositionPicker::setXUnit(const QString &qstrXUnit)
{
    m_qstrXUnit = qstrXUnit;
}

void cBasicQwtLinePlotPositionPicker::setYUnit(const QString &qstrYUnit)
{
    m_qstrYUnit = qstrYUnit;
}

QwtText cBasicQwtLinePlotPositionPicker::trackerTextF(const QPointF &oPosition) const
{
    QwtText oTrackerText(QString("%1 %2\n%3 %4").arg(QString::number(oPosition.x())).arg(m_qstrXUnit).arg(QString::number(oPosition.y())).arg(m_qstrYUnit));

    QColor oBackgroundColour(Qt::black);
    oBackgroundColour.setAlphaF(0.5);
    oTrackerText.setBackgroundBrush(QBrush(oBackgroundColour));

    return oTrackerText;
}
