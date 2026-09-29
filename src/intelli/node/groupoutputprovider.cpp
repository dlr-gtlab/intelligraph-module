/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
 */

#include <intelli/node/groupoutputprovider.h>

using namespace intelli;

GraphOutputProvider::GraphOutputProvider() :
    AbstractGraphProvider(PortType::Out, "Output Provider")
{ }
