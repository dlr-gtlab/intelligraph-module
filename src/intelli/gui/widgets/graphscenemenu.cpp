/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/gui/widgets/graphscenemenu.h"
#include "intelli/nodefactory.h"

#include "gt_icons.h"
#include "gt_utilities.h"
#include "gt_application.h"

#include <QKeyEvent>
#include <QTreeWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QTimer>
#include <QWidgetAction>

#include <algorithm>

using namespace intelli;

namespace
{

// local helper struct
struct NodeDetail
{
    QString className;
    QString displayName;
};

class MenuTreeWidget : public QTreeWidget
{
public:

    explicit MenuTreeWidget(QWidget* parent = nullptr) :
        QTreeWidget(parent)
    {
        assert(header());
        header()->close();
        setFrameShape(QFrame::NoFrame);
    }

protected:

    void keyPressEvent(QKeyEvent* event) override
    {
        switch (event->key())
        {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            // fixes: pressing enter on category item triggers action -> closes Menu
            if (currentItem() && currentItem()->childCount() > 0)
            {
                expand(currentIndex());
                event->accept();
                return;
            }
            break;
        default:
            break;
        }
        return QTreeWidget::keyPressEvent(event);
    }
};

} // namespace

GraphSceneMenu::GraphSceneMenu(QWidget* parent) :
    QMenu(parent),
    m_guard(false) // guard to trigger node creation only once (see issue #350 and #365)
{
    assert(gtApp);

    // Add filterbox to the context menu
    auto* filterEdit = new QLineEdit(this);
    filterEdit->setPlaceholderText(
        QStringLiteral(" Filter (%1)").arg(
            gtApp->getShortCutSequence(QStringLiteral("search")).toString()));
    filterEdit->setClearButtonEnabled(true);
    filterEdit->setMinimumHeight(filterEdit->sizeHint().height());
    m_filterEdit = filterEdit;

    // set the focus to allow text inputs
    QTimer::singleShot(0, filterEdit, SIGNAL(setFocus()));

    auto* filterEditAction = new QWidgetAction(this);
    filterEditAction->setDefaultWidget(filterEdit);
    filterEditAction->setShortcut(gtApp->getShortCutSequence("search"));
    this->addAction(filterEditAction);

    // Add result treeview to the context menu
    auto* treeView = new MenuTreeWidget(this);
    treeView->setMinimumHeight(treeView->sizeHint().height() * 1.5);
    treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView = treeView;

    auto* treeViewAction = new QWidgetAction(this);
    treeViewAction->setDefaultWidget(treeView);
    this->addAction(treeViewAction);

    auto commentItem = new QTreeWidgetItem(treeView);
    commentItem->setText(0, QObject::tr("Add Comment"));
    commentItem->setIcon(0, gt::gui::icon::comment());

    auto const& factory = NodeFactory::instance();

    QStringList const cats = factory.sortedCategories();

    for (QString const& cat : cats)
    {
        if (cat.isEmpty()) continue;

        auto item = new QTreeWidgetItem(treeView);
        item->setText(0, cat);
    }

    QVector<NodeDetail> nodeModelNames;
    {
        QStringList const nodes = factory.registeredNodes();
        std::transform(nodes.begin(), nodes.end(),
                       std::back_inserter(nodeModelNames),
                       [&factory, &nodeModelNames](QString const& className){
            return NodeDetail{className, factory.nodeModelName(className)};
        });
    }

    // sort by display name
    std::sort(nodeModelNames.begin(), nodeModelNames.end(), [](NodeDetail const& a,
                                                               NodeDetail const& b){
        return std::tie(a.displayName, a.className) <
               std::tie(b.displayName, b.className);
    });

    for (NodeDetail const& entry : nodeModelNames)
    {
        for (QString const& cat : factory.nodeCategories(entry.className))
        {
            auto parents = treeView->findItems(cat, Qt::MatchExactly);
            if (parents.empty()) continue;

            auto item = new QTreeWidgetItem(parents.first());
            item->setText(0, entry.displayName);
            item->setWhatsThis(0, entry.className); // here: store class name of node
        }
    }

    treeView->expandAll();

    auto onClicked = [this, commentItem](QTreeWidgetItem* item, int = 0){
        assert(item);
        if (item->childCount() > 0)
        {
            item->setExpanded(true);
            return;
        }

        if (m_guard) return;
        m_guard = true;

        (item == commentItem) ?
            emit createCommentRequested() :
            emit createNodeRequested(item->whatsThis(0));

        this->close();
    };

    // does not react to on key pressed
    connect(treeView, &QTreeWidget::itemClicked, this, onClicked);
    // may trigger multiple times on certain platforms
    connect(treeView, &QTreeWidget::itemActivated, this, onClicked);
    // trigger creation when comitting text
    connect(filterEdit, &QLineEdit::returnPressed, this, [onClicked, treeView](){
        auto const selection = treeView->selectedItems();
        if (!selection.empty()) onClicked(selection.front());
    });

    // setup filtering
    connect(filterEdit, &QLineEdit::textChanged,
            treeView, [treeView](QString const& filterText) {

        auto const filterItem =
            [&filterText](auto filter, QTreeWidgetItem* item, bool force) -> bool {
            assert(item);

            bool const selfMatch
                = force || item->text(0).contains(filterText, Qt::CaseInsensitive);

            // filter children
            bool childMatch = false;
            for (int i : gt::range(0, item->childCount()))
            {
                // apply filter first
                childMatch = filter(filter, item->child(i), selfMatch) || childMatch;
            }

            // apply filter
            bool const visible = selfMatch || childMatch;
            item->setHidden(!visible);
            item->setExpanded(visible);

            return visible;
        };

        // filter over all items
        for (int i : gt::range(0, treeView->topLevelItemCount()))
        {
            filterItem(filterItem, treeView->topLevelItem(i), filterText.isEmpty());
        }

        treeView->clearSelection();
        if (filterText.isEmpty()) return;

        // select first visible entry
        for (int p : gt::range(0, treeView->topLevelItemCount()))
        {
            QTreeWidgetItem* parent = treeView->topLevelItem(p);
            if (parent->isHidden()) continue;

            // highlight top level item
            if (parent->childCount() == 0)
            {
                parent->setSelected(true);
                return;
            }
            // else highlight first visible child
            for (int c : gt::range(0, parent->childCount()))
            {
                QTreeWidgetItem* child = parent->child(c);
                if (child->isHidden()) continue;

                child->setSelected(true);
                return;
            }
        }
    });
}

void
GraphSceneMenu::keyPressEvent(QKeyEvent* event)
{
    assert(m_filterEdit && m_treeView);
    assert(gtApp);

    // switch to filter
    if (gtApp->compareKeyEvent(event, gtApp->getShortCutSequence(QStringLiteral("search"))))
    {
        m_filterEdit->setFocus();
        return;
    }

    switch (event->key())
    {
    case Qt::Key_Up:
        if (m_filterEdit->hasFocus())
        {
            event->accept();
            return;
        }
        if (m_treeView->hasFocus())
        {
            event->accept();
            m_treeView->clearSelection();
            m_filterEdit->setFocus();
            return;
        }
        break;
    case Qt::Key_Down:
        if (m_filterEdit->hasFocus())
        {
            event->accept();
            m_treeView->setFocus();
            auto const selection = m_treeView->selectedItems();
            QTreeWidgetItem* item = !selection.empty() ?
                                        selection.front() :
                                        m_treeView->topLevelItem(0);
            m_treeView->clearSelection();
            m_treeView->setCurrentItem(item);
            item->setSelected(true);
            return;
        }
        if (m_treeView->hasFocus())
        {
            event->accept();
            return;
        }
        break;
    default:
        break;
    }
    QMenu::keyPressEvent(event);
}
