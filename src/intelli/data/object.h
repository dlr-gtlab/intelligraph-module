/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_OBJECTDATA_H
#define GT_INTELLI_OBJECTDATA_H

#include <intelli/nodedata.h>
#include <intelli/memory.h>

namespace intelli
{

/**
 * @brief The ObjectData class. Represents a GtObject as node data object.
 */
class GT_INTELLI_EXPORT ObjectData : public NodeData
{
    Q_OBJECT

public:

    /**
     * @brief constructor
     * @param obj Object to hold. Ownership is not transfered.
     * A copy will be made
     */
    Q_INVOKABLE ObjectData(GtObject const* obj = nullptr);
    ~ObjectData();

    /**
     * @brief getter
     * @return object
     */
    Q_INVOKABLE GtObject const* object() const { return m_obj.get(); }

private:

    volatile_ptr<GtObject, DeferredDeleter> m_obj;
};

} // namespace intelli

#endif // GT_INTELLI_OBJECTDATA_H
