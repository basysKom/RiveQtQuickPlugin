#include "riveqtpaint.h"

RiveQtPaint::RiveQtPaint()
{
    m_Pen.setCosmetic(true);
    m_Pen.setCapStyle(Qt::PenCapStyle::RoundCap);
    m_Pen.setJoinStyle(Qt::PenJoinStyle::RoundJoin);
}
