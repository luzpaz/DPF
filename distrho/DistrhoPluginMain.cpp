/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "src/DistrhoPlugin.cpp"

#if defined(DISTRHO_PLUGIN_TARGET_CARLA)
# include "src/DistrhoPluginCarla.cpp"
#elif defined(DISTRHO_PLUGIN_TARGET_JACK)
# include "src/DistrhoPluginJACK.cpp"
#elif (defined(DISTRHO_PLUGIN_TARGET_LADSPA) || defined(DISTRHO_PLUGIN_TARGET_DSSI))
# include "src/DistrhoPluginLADSPA+DSSI.cpp"
#elif defined(DISTRHO_PLUGIN_TARGET_LV2)
# include "src/DistrhoPluginLV2.cpp"
# include "src/DistrhoPluginLV2export.cpp"
#elif defined(DISTRHO_PLUGIN_TARGET_VST2)
# include "src/DistrhoPluginVST2.cpp"
#elif defined(DISTRHO_PLUGIN_TARGET_VST3)
# include "src/DistrhoPluginVST3.cpp"
#else
# error unsupported format
#endif
