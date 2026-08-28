/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/groupinputprovider.h>

using namespace intelli;

GraphInputProvider::GraphInputProvider() :
    AbstractGraphProvider(PortType::In, "Input Provider")
{ }
