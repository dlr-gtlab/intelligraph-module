/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/node/binarydisplay.h"
#include "intelli/data/bool.h"
#include "intelli/graphconnectionmodel.h"

#include <gt_application.h>

#include <QLCDNumber>
#include <QLayout>

#include <cmath>

using namespace intelli;

BinaryDisplayNode::BinaryDisplayNode() :
    DynamicNode(tr("Binary Display"),
                QStringList{typeId<BoolData>()},
                QStringList{},
                DynamicNode::DynamicInputOnly)
{
    setNodeEvalMode(NodeEvalMode::Blocking);
    setNodeFlag(Resizable);

    addStaticInPort(makePort(typeId<BoolData>()).setCaption("in_0"));

    registerWidgetFactory([this](){
        auto b = makeBaseWidget();
        auto* lay = b->layout();

        auto* wid = new QLCDNumber();
        lay->addWidget(wid);

        b->setMinimumSize(60, 80);

        auto const updateStyle = [wid](bool isDark){
            wid->setStyleSheet(isDark ?
                // dark mode
                R"(QLCDNumber{
                    background-color: rgb(0, 0, 0);
                    border: 2px solid rgb(113, 113, 113);
                    border-width: 2px;
                    border-radius: 10px;
                    color: rgb(255, 255, 255);
                }
                )" :
                // bright mode
                R"(QLCDNumber{
                    background-color: rgb(255, 255, 255);
                    border: 2px solid rgb(113, 113, 113);
                    border-width: 2px;
                    border-radius: 10px;
                    color: rgb(0, 0, 0);
                  }
                )");
        };

        auto updateDisplay = [wid, this](){
            int value = inputValue();
            wid->display(value);
        };
        auto updateDigitCount = [wid, this](){
            size_t maxValue = std::pow(2u, ports(PortType::In).size());
            int digits = std::ceil(std::log10(maxValue));
            wid->setDigitCount(digits);
            wid->setMinimumSize(20 * digits, 20);
        };

        connect(this, &Node::evaluated, wid, updateDisplay);
        connect(this, &Node::portInserted, wid, updateDigitCount);
        connect(this, &Node::portDeleted, wid, updateDigitCount);
        connect(gtApp, &GtApplication::themeChanged, wid, updateStyle);

        updateDisplay();
        updateDigitCount();
        updateStyle(gtApp->inDarkMode());

        return b;
    });
}

unsigned int
BinaryDisplayNode::inputValue()
{
    unsigned value = 0u;
    size_t idx  = 0;
    for (PortInfo const& info : ports(PortType::In))
    {
        if (auto data = nodeData<BoolData>(info.id()))
        {
            value |= (unsigned)data->value() << idx;
        }
        idx++;
    }
    return value;
}
