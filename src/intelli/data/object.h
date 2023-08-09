/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 15.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */


#ifndef GT_INTELLI_OBJECTDATA_H
#define GT_INTELLI_OBJECTDATA_H

#include "intelli/nodedata.h"

namespace intelli
{

/**
 * @brief The GtIgObjectData class. Represents a GtObject as node data.
 */
class GT_INTELLI_EXPORT ObjectData : public NodeData
{
    Q_OBJECT

public:

    /**
     * @brief constructor
     * @param obj Object to hold. Ownership isno transfered. A copy will  be made
     */
    Q_INVOKABLE ObjectData(GtObject const* obj = nullptr);

    /**
     * @brief getter
     * @return object
     */
    GtObject const* object() const { return m_obj.get(); }

private:

    std::unique_ptr<GtObject const> m_obj;
};

} // namespace intelli

using GtIgObjectData [[deprecated]] = intelli::ObjectData;

#endif // GT_INTELLI_OBJECTDATA_H
