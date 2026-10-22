// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "ui/scroll_view.h"

#include <string>
#include <vector>

class HistoryRows;

namespace cutefish::ui {
class Label;
}

// The calculations so far, one a row, right-aligned and dimmed, kept
// scrolled to the latest, whose text line ends at the bottom.
class HistoryView : public cutefish::ui::ScrollView
{
public:
    HistoryView();

    void setEntries(const std::vector<std::string>& entries);
    size_t count() const;
    // How far a row's text line, and the ink of its figures, reach below its baseline.
    double descentBelowBaseline() const;
    double figureInkBelowBaseline() const;

protected:
    void layout() override;

private:
    HistoryRows* m_rows;
    // Measures the rows' figures; never shown.
    cutefish::ui::Label* m_measure;
    bool m_scrollToEnd = false;
};
