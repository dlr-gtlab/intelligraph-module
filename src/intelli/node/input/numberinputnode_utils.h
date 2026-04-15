/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2026 German Aerospace Center
 */

#ifndef GT_INTELLI_NUMBERINPUTNODE_UTILS_H
#define GT_INTELLI_NUMBERINPUTNODE_UTILS_H

#include <QObject>
#include <QString>

#include <gt_abstractproperty.h>

namespace intelli
{
namespace detail
{

template <typename ModePropT>
inline int inputModeValue(ModePropT const& mode)
{
    if (!mode.isInitialized()) return 0;
    return mode.getMetaEnum().keyToValue(mode.getVal().toUtf8());
}

template <typename ModePropT>
inline void setInputModeValue(ModePropT& mode, int value)
{
    if (!mode.isInitialized()) return;
    const char* key = mode.getMetaEnum().valueToKey(value);
    if (!key) return;
    if (mode.getVal() == QLatin1String(key)) return;
    bool success = true;
    mode.setVal(key, &success);
}

template <typename NodeT,
          typename ValuePropT,
          typename BoundPropT,
          typename ModePropT,
          typename InputModeEnum,
          typename SetResizableFn>
inline void setupNumberInputNode(NodeT* node,
                                 ValuePropT& value,
                                 BoundPropT& min,
                                 BoundPropT& max,
                                 ModePropT& mode,
                                 SetResizableFn&& setResizableHOnly)
{
    bool success = mode.template registerEnum<InputModeEnum>();
    assert(success);

    auto updateResizability = [&mode, setResizableHOnly]() {
        switch (static_cast<InputModeEnum>(inputModeValue(mode)))
        {
        case InputModeEnum::SliderH:
        case InputModeEnum::LineEditBound:
        case InputModeEnum::LineEditUnbound:
            setResizableHOnly(true);
            break;
        default:
            setResizableHOnly(false);
            break;
        }
    };

    QObject::connect(&value, &ValuePropT::changed,
                     node, [node]() { emit node->rangeChanged(); });
    QObject::connect(&min, &BoundPropT::changed,
                     node, [node]() {
                        emit node->rangeChanged();
                        emit node->nodeChanged();
                        emit node->triggerNodeEvaluation();
                     });
    QObject::connect(&max, &BoundPropT::changed,
                     node, [node]() {
                        emit node->rangeChanged();
                        emit node->nodeChanged();
                        emit node->triggerNodeEvaluation();
                     });
    QObject::connect(&mode, &GtAbstractProperty::changed,
                     node, [node, updateResizability]() {
                        updateResizability();
                        emit node->inputModeChanged();
                        emit node->nodeChanged();
                     });

    updateResizability();
}

} // namespace detail
} // namespace intelli

#endif // GT_INTELLI_NUMBERINPUTNODE_UTILS_H
