#pragma once
#ifndef LOG_HXX
#define LOG_HXX

#define error_log(fmt, ...) fprintf(stderr,  fmt  "\n", __VA_ARGS__)
#define error_log0(str) fprintf(stderr,  str  "\n")
/////////////////////////////////////////////////////////////////////
#endif // LOG_HXX
// vim: et ts=2 sts=2 sw=2
