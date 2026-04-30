#ifndef HEIST_BUILD_INFO_H
#define HEIST_BUILD_INFO_H

#ifndef HEIST_BUILD_TAG
#define HEIST_BUILD_TAG "dev-local"
#endif

#ifndef HEIST_BUILD_FLAVOR
#define HEIST_BUILD_FLAVOR "unknown"
#endif

inline const char* heistBuildTag() {
    return HEIST_BUILD_TAG;
}

inline const char* heistBuildFlavor() {
    return HEIST_BUILD_FLAVOR;
}

#endif