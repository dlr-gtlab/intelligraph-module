/* GTlab - Gas Turbine laboratory
 * copyright 2009-2023 by DLR
 *
 *  Created on: 1.6.2023
 *  Author: Marius Bröcker (AT-TWK)
 *  E-Mail: marius.broecker@dlr.de
 */

#ifndef GT_INTELLI__EXPORTS_H
#define GT_INTELLI__EXPORTS_H

#if defined(WIN32)
  #if defined (GTlabIntelliGraph_EXPORTS)
    #define GT_INTELLI_EXPORT __declspec (dllexport)
    #define GT_INTELLI_TEST_EXPORT __declspec (dllexport)
  #else
    #define GT_INTELLI_EXPORT __declspec (dllimport)
    #if (defined (GTlabIntelliGraphTest))
      #define GT_INTELLI_TEST_EXPORT __declspec (dllimport)
    #else
      #define GT_INTELLI_TEST_EXPORT
    #endif
  #endif
#else
  #define GT_INTELLI_EXPORT
  #define GT_INTELLI_TEST_EXPORT
#endif


#endif // GT_INTELLI__EXPORTS_H
