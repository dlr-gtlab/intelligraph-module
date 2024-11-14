/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_FINDDIRECTCHILDNODE_H
#define GT_INTELLI_FINDDIRECTCHILDNODE_H

#include <intelli/node.h>

#include <gt_stringproperty.h>

namespace intelli
{

class ObjectData;

/**
 * @brief The FindDirectChildNode class
 * Node to find a child of an object based on the name of the child
 * and its class. If of it this is not defined the first child of the
 * specified name/class will be selected
 *
 * The edit widget for the class name is only available in the dev mode
 * as basic users would not know the class names
 */
class FindDirectChildNode : public Node
{
    Q_OBJECT

public:

    Q_INVOKABLE FindDirectChildNode();

protected:

    void eval() override;

private slots:
    /**
     * @brief updateClass - updates the value of the string property for the
     * class name to the given value
     * @param newClass
     */
    void updateClass(QString const& newClass);

    /**
     * @brief updateObjName - updates the value of the string property for the
     * object name to the given value
     * @param newObjName
     */
    void updateObjName(QString const& newObjName);

    /**
     * @brief onInputDataRecevied - emits a signal based on the current
     * selected object
     */
    void onInputDataRecevied();

private:

    /// target class name
    GtStringProperty m_childClassName;

    GtStringProperty m_objectName;

    /// ports for parent objet input and child object output
    PortId m_in, m_out;
signals:
    void emitCompleterUpdate(const ObjectData*);
};

} // namespace intelli

#endif // GT_INTELLI_FINDDIRECTCHILDNODE_H
