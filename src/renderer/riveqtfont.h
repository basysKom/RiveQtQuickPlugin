
// SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
// SPDX-FileCopyrightText: 2023 basysKom GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "private/qfontengine_p.h"
#include "rive/text_engine.hpp"
#include <rive/core.hpp>

#include <QFont>
#include <QFontDatabase>
#include <QFontInfo>
#include <QFontMetrics>
#include <QRawFont>
#include <QGlyphRun>
#include <QTextLayout>
#include <QTextOption>

#include "riveqtopenglrenderer.h"

class RiveQtFont : public rive::Font, public std::enable_shared_from_this<RiveQtFont>
{

public:
  RiveQtFont(const QFont &font, const QFontMetricsF &fontMetrics);

  RiveQtFont(const QFont &font, const std::vector<rive::Font::Coord> &coords);

  const QFont &font() const { return m_font; }

  uint32_t tagFromName(const char *name) const;

  rive::Font::Axis getAxis(uint16_t index) const override;

  uint16_t getAxisCount() const override;
  std::vector<rive::Font::Coord> getCoords() const override;

  rive::rcp<rive::Font> makeAtCoords(rive::Span<const rive::Font::Coord> coords) const override;

  rive::RawPath getPath(rive::GlyphID glyphId) const override;

  rive::SimpleArray<rive::Paragraph> onShapeText(rive::Span<const rive::Unichar> text, rive::Span<const rive::TextRun> runs) const override;

private:
  rive::Font::LineMetrics calculateLineMetrics(const QFont &font);

  QFont m_font;
  std::vector<rive::Font::Coord> m_coords;
};
