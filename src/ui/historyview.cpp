// SPDX-FileCopyrightText: 2026 Reion Wong <reionwong@gmail.com>
// SPDX-License-Identifier: BSD-3-Clause

#include "historyview.h"

#include "ui/label.h"
#include "ui/theme.h"

#include <algorithm>

using namespace cutefish::ui;

// The rows: their text the large spacing twice apart, under the large
// spacing at the top; the last one's text ends at the bottom.
class HistoryRows : public View
{
public:
    void setEntries(const std::vector<std::string>& entries)
    {
        while (m_labels.size() > entries.size()) {
            takeChild(m_labels.back());
            m_labels.pop_back();
        }
        for (size_t i = 0; i < entries.size(); ++i) {
            if (i < m_labels.size()) {
                m_labels[i]->setText(entries[i]);
                continue;
            }
            Label* label = addChild<Label>(entries[i]);
            label->setName("history entry");
            label->setTextAlignment(Label::TextAlignment::Right);
            label->setElide(Label::Elide::Middle);
            label->setColor(ColorId::DisabledText);
            m_labels.push_back(label);
        }
        requestLayout();
    }

    size_t count() const { return m_labels.size(); }

    double heightForWidth(double) const override
    {
        if (m_labels.empty())
            return 0;
        const double spacing = Theme::current().largeSpacing;
        double height = spacing + 2 * spacing * double(m_labels.size() - 1);
        for (const Label* label : m_labels)
            height += label->sizeHint().height;
        return height;
    }

protected:
    void layout() override
    {
        const double padding = Theme::current().largeSpacing;
        const double width = std::max(0.0, geometry().width - 2 * padding);
        double y = std::max(0.0, geometry().height - heightForWidth(geometry().width)) + padding;
        for (Label* label : m_labels) {
            const double height = label->sizeHint().height;
            label->setGeometry({padding, y, width, height});
            y += height + 2 * padding;
        }
    }

private:
    std::vector<Label*> m_labels;
};

HistoryView::HistoryView()
    : m_rows(setContent<HistoryRows>())
    , m_measure(addChild<Label>("1457"))
{
    setName("history");
    m_rows->setLayoutHints({.fillHeight = true});
    m_measure->setVisible(false);
}

double HistoryView::descentBelowBaseline() const
{
    return m_measure->sizeHint().height - m_measure->baselineOffset();
}

double HistoryView::figureInkBelowBaseline() const
{
    return m_measure->inkBoundingRect().bottom();
}

void HistoryView::setEntries(const std::vector<std::string>& entries)
{
    m_scrollToEnd = entries.size() > m_rows->count();
    m_rows->setEntries(entries);
    requestLayout();
}

size_t HistoryView::count() const
{
    return m_rows->count();
}

void HistoryView::layout()
{
    ScrollView::layout();
    if (m_scrollToEnd) {
        m_scrollToEnd = false;
        setScrollPosition({0, contentSize().height - geometry().height});
    }
}
