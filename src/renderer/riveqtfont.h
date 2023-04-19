/*
 * MIT License
 *
 * Copyright (C) 2023 by Jeremias Bosch
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
