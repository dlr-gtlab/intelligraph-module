/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#ifndef GT_INTELLI_DUMMYNODE_H
#define GT_INTELLI_DUMMYNODE_H

#include <intelli/dynamicnode.h>
#include <intelli/nodedata.h>

#include <gt_objectlinkproperty.h>

namespace intelli
{

class DummyNode : public intelli::DynamicNode
{
    Q_OBJECT

public:

    Q_INVOKABLE DummyNode();

    bool setDummyObject(GtObject& object);

    QString const& linkedUuid() const;

    GtObject* linkedObject();
    GtObject const* linkedObject() const;

protected:

    void eval() override;

    void onObjectDataMerged() override;

private:

    GtObjectLinkProperty m_object;
};

} // namespace intelli

#endif // GT_INTELLI_DUMMYNODE_H
