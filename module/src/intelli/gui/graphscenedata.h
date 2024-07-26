/* GTlab - Gas Turbine laboratory
 *
 * SPDX-License-Identifier: MPL-2.0+
 * SPDX-FileCopyrightText: 2024 German Aerospace Center (DLR)
 *
 * Created on: 13.5.2024
 * Author: Marius Bröcker (AT-TWK)
 * E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI_GRAPHSCENEDATA_H
#define GT_INTELLI_GRAPHSCENEDATA_H

namespace intelli
{

/**
 * @brief The GraphSceneData class.
 * Data object that is shared by all node graphics objects. This avoids redundant
 * data in nodes and allows nodes to reference scene specific settings without
 * the need to know scene specific implementation details
 */
struct GraphSceneData
{
    /// step size to snap nodes to (assuming that grid's origin is (0,0) )
    int gridSize = 0;
    /// whether nodes should be snaped to the grid when moving
    bool snapToGrid = false;
};

} // namespace intelli

#endif // GT_INTELLI_GRAPHSCENEDATA_H
