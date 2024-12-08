#pragma once

#define VERSION_MAJOR 3
#define VERSION_MINOR 0
#define VERSION_PATCH 0
#define VERSION_BUILD 0

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define VERSION_STRING TOSTRING(VERSION_MAJOR) "." TOSTRING(VERSION_MINOR) "." TOSTRING(VERSION_PATCH) "." TOSTRING(VERSION_BUILD)