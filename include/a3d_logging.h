#pragma once

#include <stdio.h>
#include <time.h>

#ifndef A3D_LOG_USE_COLOR
#define A3D_LOG_USE_COLOR 1
#endif

#ifndef A3D_LOG_SHOW_SRC
#define A3D_LOG_SHOW_SRC 1
#endif

#if A3D_LOG_USE_COLOR
#define A3D_ANSI_RESET      "\x1b[0m"
#define A3D_ANSI_DIM        "\x1b[2m"
#define A3D_ANSI_BOLD       "\x1b[1m"

#define A3D_ANSI_FG_RED     "\x1b[31m"
#define A3D_ANSI_FG_GREEN   "\x1b[32m"
#define A3D_ANSI_FG_YELLOW  "\x1b[33m"
#define A3D_ANSI_FG_BLUE    "\x1b[34m"
#define A3D_ANSI_FG_MAGENTA "\x1b[35m"
#define A3D_ANSI_FG_CYAN    "\x1b[36m"
#define A3D_ANSI_FG_WHITE   "\x1b[37m"
#define A3D_ANSI_FG_GRAY    "\x1b[90m"
#else
#define A3D_ANSI_RESET      ""
#define A3D_ANSI_DIM        ""
#define A3D_ANSI_BOLD       ""

#define A3D_ANSI_FG_RED     ""
#define A3D_ANSI_FG_GREEN   ""
#define A3D_ANSI_FG_YELLOW  ""
#define A3D_ANSI_FG_BLUE    ""
#define A3D_ANSI_FG_MAGENTA ""
#define A3D_ANSI_FG_CYAN    ""
#define A3D_ANSI_FG_WHITE   ""
#define A3D_ANSI_FG_GRAY    ""
#endif

static inline const char* a3d_log_time_hhmmss(char out[9])
{
	time_t t = time(NULL);
	struct tm tmv;
#if defined(_WIN32)
	localtime_s(&tmv, &t);
#else
	localtime_r(&t, &tmv);
#endif
	/* "HH:MM:SS" */
	snprintf(out, 9, "%02d:%02d:%02d", tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
	return out;
}

#define A3D_LOG__LEVEL_INFO  "INFO"
#define A3D_LOG__LEVEL_WARN  "WARN"
#define A3D_LOG__LEVEL_ERROR "ERROR"
#define A3D_LOG__LEVEL_DEBUG "DEBUG"

#define A3D_LOG__C_INFO      A3D_ANSI_FG_CYAN
#define A3D_LOG__C_WARN      A3D_ANSI_FG_YELLOW
#define A3D_LOG__C_ERROR     A3D_ANSI_FG_RED A3D_ANSI_BOLD
#define A3D_LOG__C_DEBUG     A3D_ANSI_FG_MAGENTA
#define A3D_LOG__C_TIME      A3D_ANSI_FG_GRAY
#define A3D_LOG__C_SRC       A3D_ANSI_FG_GRAY

#if A3D_LOG_SHOW_SRC
#define A3D_LOG__SRC_FMT " " A3D_LOG__C_SRC "%s : %d : %s()" A3D_ANSI_RESET
#define A3D_LOG__SRC_ARGS , __FILE__, __LINE__, __func__
#else
#define A3D_LOG__SRC_FMT ""
#define A3D_LOG__SRC_ARGS
#endif

#define A3D_LOG__TAG_FMT " " A3D_ANSI_FG_BLUE "[%s]" A3D_ANSI_RESET
#define A3D_LOG__TAG_ARG(tag) ((tag) ? (tag) : "-")

#define A3D_LOG__PRINT(stream, LEVELSTR, LEVELCOL, TAG, fmt, ...)       \
do {                                                                    \
	char _t[9];                                                         \
	fprintf((stream),                                                   \
		A3D_LOG__C_TIME "%s" A3D_ANSI_RESET                             \
		" " LEVELCOL "%-5s" A3D_ANSI_RESET                              \
		A3D_LOG__TAG_FMT                                                \
		A3D_LOG__SRC_FMT                                                \
		" " fmt "\n",                                                   \
		a3d_log_time_hhmmss(_t),                                        \
		(LEVELSTR),                                                     \
		A3D_LOG__TAG_ARG(TAG)                                           \
		A3D_LOG__SRC_ARGS,                                              \
		##__VA_ARGS__);                                                 \
} while (0)

#ifndef A3D_LOG_TAG
#define A3D_LOG_TAG NULL
#endif

/* api */
#define A3D_LOG(fmt, ...) \
	fprintf(stdout, fmt "\n", ##__VA_ARGS__)

#define A3D_LOG_INFO(fmt, ...)  A3D_LOG__PRINT(stdout, A3D_LOG__LEVEL_INFO,  A3D_LOG__C_INFO,  A3D_LOG_TAG, fmt, ##__VA_ARGS__)
#define A3D_LOG_WARN(fmt, ...)  A3D_LOG__PRINT(stderr, A3D_LOG__LEVEL_WARN,  A3D_LOG__C_WARN,  A3D_LOG_TAG, fmt, ##__VA_ARGS__)
#define A3D_LOG_ERROR(fmt, ...) A3D_LOG__PRINT(stderr, A3D_LOG__LEVEL_ERROR, A3D_LOG__C_ERROR, A3D_LOG_TAG, fmt, ##__VA_ARGS__)

#if !defined(NDEBUG)
#define A3D_LOG_DEBUG(fmt, ...) A3D_LOG__PRINT(stdout, A3D_LOG__LEVEL_DEBUG, A3D_LOG__C_DEBUG, A3D_LOG_TAG, fmt, ##__VA_ARGS__)
#else
#define A3D_LOG_DEBUG(fmt, ...) ((void)0)
#endif
