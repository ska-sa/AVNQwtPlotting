//System includes

//Library includes

//Local includes
#include "QwtPlotPositionPicker.h"
#include "AVNUtilLibs/Timestamp/Timestamp.h"

#if QWT_VERSION < 0x060100 //Account for Ubuntu's typically outdated package versions
cQwtPlotPositionPicker::cQwtPlotPositionPicker(int iXAxis, int iYAxis, RubberBand oRubberBand, DisplayMode oTrackerMode, QwtPlotCanvas *pCanvas) :
    QwtPlotPicker(iXAxis, iYAxis, oRubberBand, oTrackerMode, pCanvas),
  #else
cQwtPlotPositionPicker::cQwtPlotPositionPicker(int iXAxis, int iYAxis, RubberBand oRubberBand, DisplayMode oTrackerMode, QWidget *pCanvas) :
    QwtPlotPicker(iXAxis, iYAxis, oRubberBand, oTrackerMode, pCanvas),
  #endif
    m_bXIsTime(false),
    m_bYIsTime(false)
{
    setMousePattern( QwtEventPattern::MouseSelect1, Qt::NoButton);
}

void cQwtPlotPositionPicker::setXUnit(const QString &qstrXUnit)
{
    m_qstrXUnit = qstrXUnit;
}

void cQwtPlotPositionPicker::setYUnit(const QString &qstrYUnit)
{
    m_qstrYUnit = qstrYUnit;
}

void cQwtPlotPositionPicker::setXIsTime(bool bIsTime)
{
    m_bXIsTime = bIsTime;
}

void cQwtPlotPositionPicker::setYIsTime(bool bIsTime)
{
    m_bYIsTime = bIsTime;
}

QwtText cQwtPlotPositionPicker::trackerTextF(const QPointF &oPosition) const
{
    QString qstrX;
    QString qstrY;

    if(m_bXIsTime)
    {
        qstrX = QString(AVN::stringFromTimestamp_HHmmssuuuuuu(oPosition.x() * 1e6).c_str());
    }
    else
    {
        qstrX = QString("%1 %2").arg(QString::number(oPosition.x())).arg(m_qstrXUnit);
    }

    if(m_bYIsTime)
    {
        qstrY = QString(AVN::stringFromTimestamp_HHmmssuuuuuu(oPosition.y() * 1e6).c_str());
    }
    else
    {
        qstrY = QString("%1 %2").arg(QString::number(oPosition.y())).arg(m_qstrYUnit);
    }

    QwtText oTrackerText(QString("%1\n%2").arg(qstrX).arg(qstrY));

    QColor oBackgroundColour(Qt::black);
    oBackgroundColour.setAlphaF(0.5);
    oTrackerText.setBackgroundBrush(QBrush(oBackgroundColour));

    return oTrackerText;
}
