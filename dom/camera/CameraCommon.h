/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=40: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DOM_CAMERA_CAMERACOMMON_H
#define DOM_CAMERA_CAMERACOMMON_H

#ifndef __func__
#ifdef __FUNCTION__
#define __func__ __FUNCTION__
#else
#define __func__ __FILE__
#endif
#endif

#ifndef NAN
#define NAN std::numeric_limits<double>::quiet_NaN()
#endif

#include "prlog.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetCameraLog();
#define DOM_CAMERA_LOG( type, a ) PR_LOG(GetCameraLog(), (PRLogModuleLevel)type, ( a ))
#else
#define DOM_CAMERA_LOG( type, a )
#endif

#define DOM_CAMERA_LOGA( a )      DOM_CAMERA_LOG( 0, a )

/**
 * From the least to the most output.
 */
enum {
  DOM_CAMERA_LOG_NOTHING,
  DOM_CAMERA_LOG_ERROR,
  DOM_CAMERA_LOG_WARNING,
  DOM_CAMERA_LOG_INFO,
  DOM_CAMERA_LOG_TRACE,
  DOM_CAMERA_LOG_REFERENCES
};

/**
 * DOM_CAMERA_LOGR() can be called before 'gCameraLog' is set, so
 * we need to handle this one a little differently.
 */
#ifdef PR_LOGGING
#define DOM_CAMERA_LOGR( a )                                  \
  do {                                                          \
    if (GetCameraLog()) {                                       \
      DOM_CAMERA_LOG( DOM_CAMERA_LOG_REFERENCES, a ); \
    }                                                           \
  } while (0)
#else
#define DOM_CAMERA_LOGR( a )
#endif
#define DOM_CAMERA_LOGT( a )      DOM_CAMERA_LOG( DOM_CAMERA_LOG_TRACE, a )
#define DOM_CAMERA_LOGI( a )      DOM_CAMERA_LOG( DOM_CAMERA_LOG_INFO, a )
#define DOM_CAMERA_LOGW( a )      DOM_CAMERA_LOG( DOM_CAMERA_LOG_WARNING, a )
#define DOM_CAMERA_LOGE( a )      DOM_CAMERA_LOG( DOM_CAMERA_LOG_ERROR, a )

#endif // DOM_CAMERA_CAMERACOMMON_H
