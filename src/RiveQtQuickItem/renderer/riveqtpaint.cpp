// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "riveqtpaint.h"

RiveQtPaint::RiveQtPaint()
{
    m_Pen.setCosmetic(true);
    m_Pen.setCapStyle(Qt::PenCapStyle::RoundCap);
    m_Pen.setJoinStyle(Qt::PenJoinStyle::RoundJoin);
}
