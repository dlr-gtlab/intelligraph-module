/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/data/object.h"

using namespace intelli;

ObjectData::ObjectData(GtObject const* obj) :
    NodeData(QStringLiteral("object")),
    m_obj(unique_qptr<GtObject, DeferredDeleter>(obj ? obj->clone() : nullptr))
{

}

ObjectData::~ObjectData() = default;
