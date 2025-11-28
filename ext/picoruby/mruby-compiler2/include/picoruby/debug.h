/**
 * @file picoruby/debug.h - PicoRuby debug macros for Hako
 *
 * Simple debug output macros for PicoRuby components
 */

#ifndef PICORUBY_DEBUG_H
#define PICORUBY_DEBUG_H

#include <stdio.h>

#ifdef PICORUBY_DEBUG
  #define D(fmt, ...) printf("[DEBUG] " fmt, ##__VA_ARGS__)
#else
  #define D(fmt, ...) do {} while (0)
#endif

#endif /* PICORUBY_DEBUG_H */
