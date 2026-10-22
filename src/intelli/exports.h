/*
 * GTlab IntelliGraph
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  SPDX-FileCopyrightText: 2024 German Aerospace Center
 *
 *  Author: Marius Bröcker <marius.broecker@dlr.de>
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

#ifdef _MSVC_LANG
  #define GT_INTELLIGRAPH_CPP_VERSION _MSVC_LANG
#else
  #define GT_INTELLIGRAPH_CPP_VERSION __cplusplus
#endif

#if GT_INTELLIGRAPH_CPP_VERSION < 201703L
  #error "IntelliGraph requires C++17 or higher"
#endif

#endif // GT_INTELLI__EXPORTS_H
