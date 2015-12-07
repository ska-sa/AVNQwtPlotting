#ifndef CURSOR_CENTRED_QWT_PLOT_MAGNIFIER_H
#define CURSOR_CENTRED_QWT_PLOT_MAGNIFIER_H

//System includes

//Library includes
#include <qwt_plot_magnifier.h>
#include <QWheelEvent>
#include <QPoint>
#include <QPointF>

//Local includes

class cCursorCentredQwtPlotMagnifier : public QwtPlotMagnifier
{
    Q_OBJECT
public:
#if QWT_VERSION < 0x060100 //Account for Ubuntu's typically outdated package versions
    explicit cCursorCentredQwtPlotMagnifier(QwtPlotCanvas* pCanvas);
#else
    explicit cCursorCentredQwtPlotMagnifier(QWidget* pCanvas);
#endif


protected:
    virtual void    widgetWheelEvent(QWheelEvent *pWheelEvent);
    virtual void    rescale(double dFactor);

    QPoint          m_oMousePosition;

signals:

public slots:

};

#endif // CURSOR_CENTRED_QWT_PLOT_MAGNIFIER_H
