/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 1.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLIGRAPH_EXPORTS_H
#define GT_INTELLIGRAPH_EXPORTS_H


#if defined(WIN32)
  #if defined (GTlabIntelliGraph_EXPORTS)
    #define GT_IG_EXPORT __declspec (dllexport)
  #else
    #define GT_IG_EXPORT __declspec (dllimport)
  #endif
#else
  #define GT_IG_EXPORT
#endif


#endif // GT_INTELLIGRAPH_EXPORTS_H
