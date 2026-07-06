dnl This Source Code Form is subject to the terms of the Mozilla Public
dnl License, v. 2.0. If a copy of the MPL was not distributed with this
dnl file, You can obtain one at http://mozilla.org/MPL/2.0/.

dnl Identify which version of the SDK we're building with
dnl Windows Server 2008 and newer SDKs have WinSDKVer.h, get the version
dnl from there
AC_DEFUN([MOZ_FIND_WINSDK_VERSION], [
  MOZ_CHECK_HEADERS([winsdkver.h])
  MOZ_CHECK_HEADERS([sdkddkver.h])
  if test "$ac_cv_header_winsdkver_h" = "yes"; then
      dnl Get the highest _WIN32_WINNT and NTDDI versions supported
      dnl Take the higher of the two
      dnl This is done because the Windows 7 beta SDK reports its
      dnl NTDDI_MAXVER to be 0x06000100 instead of 0x06010000, as it should
      AC_CACHE_CHECK(for highest Windows version supported by this SDK,
                     ac_cv_winsdk_maxver,
                     [cat > conftest.h <<EOF
#include <winsdkver.h>
#include <sdkddkver.h>

#if (NTDDI_VERSION_FROM_WIN32_WINNT(_WIN32_WINNT_MAXVER) > NTDDI_MAXVER)
#define WINSDK_MAXVER NTDDI_VERSION_FROM_WIN32_WINNT(_WIN32_WINNT_MAXVER)
#else
#define WINSDK_MAXVER NTDDI_MAXVER
#endif

WINSDK_MAXVER
EOF
                      ac_cv_winsdk_maxver=`$CPP conftest.h 2>/dev/null | tail -n1`
                      rm -f conftest.h
                     ])
      MOZ_WINSDK_MAXVER=${ac_cv_winsdk_maxver}
  else
      if test "$ac_cv_header_sdkddkver_h" = "yes"; then
			#include <sdkddkver.h>
                MOZ_WINSDK_MAXVER=0x06000000
            else
                # Assume the Server 2003 Platform SDK
                MOZ_WINSDK_MAXVER=0x05020000
            fi
  fi
])
