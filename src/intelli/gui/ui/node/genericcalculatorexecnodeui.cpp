/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#include <intelli/gui/ui/node/genericcalculatorexecnodeui.h>

#include <intelli/gui/graphics/nodeobject.h>
#include <intelli/node/genericcalculatorexec.h>
#include <intelli/nodedatainterface.h>

#include <gt_coreapplication.h>
#include <gt_project.h>
#include <gt_propertytreeview.h>
#include <gt_stylesheets.h>

#include <QComboBox>
#include <QGraphicsWidget>
#include <QVBoxLayout>

#include <cassert>

using namespace intelli;

GenericCalculatorExecNodeWidget::GenericCalculatorExecNodeWidget(GenericCalculatorExecNode& node,
                                                                 QWidget* parent) :
    QWidget(parent)
{
    m_node = &node;

    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    m_classEdit = new QComboBox(this);
    m_classEdit->addItems(GenericCalculatorExecNode::classIdents());
    m_classEdit->setStyleSheet(gt::gui::stylesheet::comboBox());
    lay->addWidget(m_classEdit);

    auto* model = exec::nodeDataInterface(*m_node);
    GtObject* scope = model ? model->scope() : static_cast<GtObject*>(gtApp->currentProject());
    assert(scope);

    m_view = new GtPropertyTreeView(scope, this);
    m_view->setAnimated(false);
    lay->addWidget(m_view);

    QObject::connect(m_classEdit, &QComboBox::currentTextChanged,
                     this, &GenericCalculatorExecNodeWidget::updateClassFromEditor);
    QObject::connect(m_node, &GenericCalculatorExecNode::classNameChanged,
                     this, &GenericCalculatorExecNodeWidget::updateClassEditorFromNode);
    QObject::connect(m_node, &GenericCalculatorExecNode::currentObjectChanged,
                     this, &GenericCalculatorExecNodeWidget::updateCurrentObjectView);

    m_node->syncConnectedPorts();

    m_node->className().isEmpty() ? updateClassFromEditor(m_classEdit->currentText())
                                  : updateClassEditorFromNode(m_node->className());
    updateCurrentObjectView();
}

NodeUI::QGraphicsWidgetPtr
GenericCalculatorExecNodeWidget::create(Node& source, NodeGraphicsObject& object)
{
    auto* node = qobject_cast<GenericCalculatorExecNode*>(&source);
    if (!node) return nullptr;

    auto w = std::make_unique<GenericCalculatorExecNodeWidget>(*node);
    return NodeUI::convertToGraphicsWidget(std::move(w), object);
}

void
GenericCalculatorExecNodeWidget::updateCurrentObjectView()
{
    m_view->setObject(nullptr);

    auto obj = m_node->currentObject();
    if (!obj) return;

    m_view->setObject(obj);
    // collapse first category
    m_view->collapse(m_view->model()->index(0, 0, m_view->rootIndex()));

    QObject::connect(obj, qOverload<GtObject*, GtAbstractProperty*>(&GtObject::dataChanged),
                     m_node, &GenericCalculatorExecNode::onCurrentObjectDataChanged,
                     Qt::UniqueConnection);
}

void
GenericCalculatorExecNodeWidget::updateClassFromEditor(QString const& classText)
{
    m_node->setClassName(GenericCalculatorExecNode::classNameFromIdent(classText));
}

void
GenericCalculatorExecNodeWidget::updateClassEditorFromNode(QString const& className)
{
    m_classEdit->setCurrentText(GenericCalculatorExecNode::identFromClassName(className));
}

GenericCalculatorExecNodeUI::GenericCalculatorExecNodeUI() = default;

NodeUI::WidgetFactoryFunction
GenericCalculatorExecNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<GenericCalculatorExecNode const*>(&n)) return {};

    return &GenericCalculatorExecNodeWidget::create;
}
