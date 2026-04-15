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

using namespace intelli;

GenericCalculatorExecNodeUI::GenericCalculatorExecNodeUI() = default;

NodeUI::WidgetFactoryFunction
GenericCalculatorExecNodeUI::centralWidgetFactory(Node const& n) const
{
    if (!qobject_cast<GenericCalculatorExecNode const*>(&n)) return {};

    return [](Node& source, NodeGraphicsObject& object) -> QGraphicsWidgetPtr {
        auto* node = qobject_cast<GenericCalculatorExecNode*>(&source);
        if (!node) return nullptr;

        auto w = std::make_unique<QWidget>();
        auto* lay = new QVBoxLayout;
        w->setLayout(lay);
        lay->setContentsMargins(0, 0, 0, 0);

        auto* edit = new QComboBox;
        edit->addItems(GenericCalculatorExecNode::classIdents());
        edit->setStyleSheet(gt::gui::stylesheet::comboBox());
        lay->addWidget(edit);

        auto* model = exec::nodeDataInterface(*node);
        GtObject* scope = model ? model->scope() : static_cast<GtObject*>(gtApp->currentProject());
        assert(scope);

        auto* view = new GtPropertyTreeView(scope);
        view->setAnimated(false);
        lay->addWidget(view);

        auto updateView = [view, node](){
            view->setObject(nullptr);

            auto obj = node->currentObject();
            if (obj)
            {
                view->setObject(obj);
                // collapse first category
                view->collapse(view->model()->index(0, 0, view->rootIndex()));

                QObject::connect(obj, qOverload<GtObject*, GtAbstractProperty*>(&GtObject::dataChanged),
                                 node, &GenericCalculatorExecNode::onCurrentObjectDataChanged,
                                 Qt::UniqueConnection);
            }
        };

        auto const updateClass = [node, edit](){
            node->setClassName(GenericCalculatorExecNode::classNameFromIdent(edit->currentText()));
        };
        auto const updateClassText = [node, edit](){
            edit->setCurrentText(GenericCalculatorExecNode::identFromClassName(node->className()));
        };

        QObject::connect(edit, &QComboBox::currentTextChanged,
                         edit, updateClass);
        QObject::connect(node, &GenericCalculatorExecNode::classNameChanged,
                         edit, updateClassText);
        QObject::connect(node, &GenericCalculatorExecNode::currentObjectChanged,
                         view, updateView);

        node->syncConnectedPorts();

        node->className().isEmpty() ? updateClass() : updateClassText();
        updateView();

        return convertToGraphicsWidget(std::move(w), object);
    };
}
