/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include "intelli/graphcategory.h"

using namespace intelli;

GraphCategory::GraphCategory()
{
    setObjectName(tr("Category"));

    setFlag(UserRenamable, true);
    setFlag(UserDeletable, true);
}
