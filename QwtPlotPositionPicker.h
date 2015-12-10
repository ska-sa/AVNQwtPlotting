#ifndef QWT_PLOT_POSITION_PICKER_H
#define QWT_PLOT_POSITION_PICKER_H

//System includes

//Library includes
#include <qwt_plot_picker.h>
#include <QString>

//Local includes

class cQwtPlotPositionPicker : public QwtPlotPicker
{
    Q_OBJECT
public:
#if QWT_VERSION < 0x060100 //Account for Ubuntu's typically outdated package versions
    explicit cQwtPlotPositionPicker(int iXAxis, int iYAxis, RubberBand oRubberBand, DisplayMode oTrackerMode, QwtPlotCanvas* pCanvas);
#else
    explicit cQwtPlotPositionPicker(int iXAxis, int iYAxis, RubberBand oRubberBand, DisplayMode oTrackerMode, QWidget* pCanvas);
#endif

    //Overload this function to return the tracker text with our units of choice
    virtual QwtText trackerTextF(const QPointF &oPosition) const;

    void    setXUnit(const QString &qstrXUnit);
    void    setYUnit(const QString &qstrYUnit);
    void    setXIsTime(bool bIsTime);
    void    setYIsTime(bool bIsTime);

protected:
    QString m_qstrXUnit;
    QString m_qstrYUnit;

    bool    m_bXIsTime;
    bool    m_bYIsTime;

signals:

public slots:

};

#endif // BASIC_QWT_LINE_PLOT_POSITION_PICKER_H
