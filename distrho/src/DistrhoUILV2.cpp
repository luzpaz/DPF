/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
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

#include "DistrhoUIInternal.hpp"

#include "../extra/String.hpp"

#include "lv2/atom.h"
#include "lv2/atom-util.h"
#include "lv2/data-access.h"
#include "lv2/instance-access.h"
#include "lv2/midi.h"
#include "lv2/options.h"
#include "lv2/parameters.h"
#include "lv2/patch.h"
#include "lv2/ui.h"
#include "lv2/urid.h"
#include "lv2/lv2_kxstudio_properties.h"
#include "lv2/lv2_programs.h"

#ifndef DISTRHO_PLUGIN_LV2_STATE_PREFIX
# define DISTRHO_PLUGIN_LV2_STATE_PREFIX "urn:distrho:"
#endif

START_NAMESPACE_DISTRHO

typedef struct _LV2_Atom_MidiEvent {
    LV2_Atom atom;    /**< Atom header. */
    uint8_t  data[3]; /**< MIDI data (body). */
} LV2_Atom_MidiEvent;

#if ! DISTRHO_PLUGIN_WANT_MIDI_INPUT
static const sendNoteFunc sendNoteCallback = nullptr;
#endif

// -----------------------------------------------------------------------

template <class LV2F>
static const LV2F* getLv2Feature(const LV2_Feature* const* features, const char* const uri)
{
    for (int i=0; features[i] != nullptr; ++i)
    {
        if (std::strcmp(features[i]->URI, uri) == 0)
            return (const LV2F*)features[i]->data;
    }

    return nullptr;
}

class UiLv2
{
public:
    UiLv2(const char* const bundlePath,
          const intptr_t winId,
          const LV2_Options_Option* options,
          const LV2_URID_Map* const uridMap,
          const LV2_Feature* const* const features,
          const LV2UI_Controller controller,
          const LV2UI_Write_Function writeFunc,
          LV2UI_Widget* const widget,
          void* const dspPtr,
          const float sampleRate,
          const float scaleFactor,
          const uint32_t bgColor,
          const uint32_t fgColor)
        : fUI(this, winId, sampleRate,
              editParameterCallback,
              setParameterCallback,
              setStateCallback,
              sendNoteCallback,
              nullptr, // resize is very messy, hosts can do it without extensions
              fileRequestCallback,
              bundlePath,
              dspPtr,
              scaleFactor,
              bgColor,
              fgColor),
          fUridMap(uridMap),
          fUiPortMap(getLv2Feature<LV2UI_Port_Map>(features, LV2_UI__portMap)),
          fUiRequestValue(getLv2Feature<LV2UI_Request_Value>(features, LV2_UI__requestValue)),
          fUiTouch(getLv2Feature<LV2UI_Touch>(features, LV2_UI__touch)),
          fController(controller),
          fWriteFunction(writeFunc),
          fURIDs(uridMap),
          fBypassParameterIndex(fUiPortMap != nullptr ? fUiPortMap->port_index(fUiPortMap->handle, "lv2_enabled")
                                                      : LV2UI_INVALID_PORT_INDEX),
          fWinIdWasNull(winId == 0)
    {
        if (widget != nullptr)
            *widget = (LV2UI_Widget)fUI.getNativeWindowHandle();

#if DISTRHO_PLUGIN_WANT_STATE
        // tell the DSP we're ready to receive msgs
        setState("__dpf_ui_data__", "");
#endif

        if (winId != 0)
            return;

        // if winId == 0 then options must not be null
        DISTRHO_SAFE_ASSERT_RETURN(options != nullptr,);

        const LV2_URID uridWindowTitle    = uridMap->map(uridMap->handle, LV2_UI__windowTitle);
        const LV2_URID uridTransientWinId = uridMap->map(uridMap->handle, LV2_KXSTUDIO_PROPERTIES__TransientWindowId);

        bool hasTitle = false;

        for (int i=0; options[i].key != 0; ++i)
        {
            if (options[i].key == uridTransientWinId)
            {
                if (options[i].type == fURIDs.atomLong)
                {
                    if (const int64_t transientWinId = *(const int64_t*)options[i].value)
                        fUI.setWindowTransientWinId(static_cast<intptr_t>(transientWinId));
                }
                else
                    d_stderr("Host provides transientWinId but has wrong value type");
            }
            else if (options[i].key == uridWindowTitle)
            {
                if (options[i].type == fURIDs.atomString)
                {
                    if (const char* const windowTitle = (const char*)options[i].value)
                    {
                        hasTitle = true;
                        fUI.setWindowTitle(windowTitle);
                    }
                }
                else
                    d_stderr("Host provides windowTitle but has wrong value type");
            }
        }

        if (! hasTitle)
            fUI.setWindowTitle(DISTRHO_PLUGIN_NAME);
    }

    // -------------------------------------------------------------------

    void lv2ui_port_event(const uint32_t rindex, const uint32_t bufferSize, const uint32_t format, const void* const buffer)
    {
        if (format == 0)
        {
            const uint32_t parameterOffset = fUI.getParameterOffset();

            if (rindex < parameterOffset)
                return;

            DISTRHO_SAFE_ASSERT_RETURN(bufferSize == sizeof(float),)

            float value = *(const float*)buffer;

            if (rindex == fBypassParameterIndex)
                value = 1.0f - value;

            fUI.parameterChanged(rindex-parameterOffset, value);
        }
#if DISTRHO_PLUGIN_WANT_STATE
        else if (format == fURIDs.atomEventTransfer)
        {
            const LV2_Atom* const atom = (const LV2_Atom*)buffer;

            if (atom->type == fURIDs.dpfKeyValue)
            {
                const char* const key   = (const char*)LV2_ATOM_BODY_CONST(atom);
                const char* const value = key+(std::strlen(key)+1);

                fUI.stateChanged(key, value);
            }
            else
            {
                d_stdout("received atom not dpfKeyValue");
            }
        }
#endif
    }

    // -------------------------------------------------------------------

    int lv2ui_idle()
    {
        if (fWinIdWasNull)
            return (fUI.plugin_idle() && fUI.isVisible()) ? 0 : 1;

        return fUI.plugin_idle() ? 0 : 1;
    }

    int lv2ui_show()
    {
        return fUI.setWindowVisible(true) ? 0 : 1;
    }

    int lv2ui_hide()
    {
        return fUI.setWindowVisible(false) ? 0 : 1;
    }

    // -------------------------------------------------------------------

    uint32_t lv2_get_options(LV2_Options_Option* const /*options*/)
    {
        // currently unused
        return LV2_OPTIONS_ERR_UNKNOWN;
    }

    uint32_t lv2_set_options(const LV2_Options_Option* const options)
    {
        for (int i=0; options[i].key != 0; ++i)
        {
            if (options[i].key == fURIDs.paramSampleRate)
            {
                if (options[i].type == fURIDs.atomFloat)
                {
                    const float sampleRate = *(const float*)options[i].value;
                    fUI.setSampleRate(sampleRate);
                    continue;
                }
                else
                {
                    d_stderr("Host changed UI sample-rate but with wrong value type");
                    continue;
                }
            }
        }

        return LV2_OPTIONS_SUCCESS;
    }

    // -------------------------------------------------------------------

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void lv2ui_select_program(const uint32_t bank, const uint32_t program)
    {
        const uint32_t realProgram = bank * 128 + program;

        fUI.programLoaded(realProgram);
    }
#endif

    // -------------------------------------------------------------------

protected:
    void editParameterValue(const uint32_t rindex, const bool started)
    {
        if (fUiTouch != nullptr && fUiTouch->touch != nullptr)
            fUiTouch->touch(fUiTouch->handle, rindex, started);
    }

    void setParameterValue(const uint32_t rindex, float value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fWriteFunction != nullptr,);

        if (rindex == fBypassParameterIndex)
            value = 1.0f - value;

        fWriteFunction(fController, rindex, sizeof(float), 0, &value);
    }

    void setState(const char* const key, const char* const value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fWriteFunction != nullptr,);

        const uint32_t eventInPortIndex = DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS;

        // join key and value
        String tmpStr;
        tmpStr += key;
        tmpStr += "\xff";
        tmpStr += value;

        tmpStr[std::strlen(key)] = '\0';

        // set msg size (key + separator + value + null terminator)
        const size_t msgSize = tmpStr.length() + 1U;

        // reserve atom space
        const size_t atomSize = sizeof(LV2_Atom) + msgSize;
        char* const  atomBuf = (char*)malloc(atomSize);
        std::memset(atomBuf, 0, atomSize);

        // set atom info
        LV2_Atom* const atom = (LV2_Atom*)atomBuf;
        atom->size = msgSize;
        atom->type = fURIDs.dpfKeyValue;

        // set atom data
        std::memcpy(atomBuf + sizeof(LV2_Atom), tmpStr.buffer(), msgSize);

        // send to DSP side
        fWriteFunction(fController, eventInPortIndex, atomSize, fURIDs.atomEventTransfer, atom);

        // free atom space
        free(atomBuf);
    }

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void sendNote(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fWriteFunction != nullptr,);

        if (channel > 0xF)
            return;

        const uint32_t eventInPortIndex = DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS;

        LV2_Atom_MidiEvent atomMidiEvent;
        atomMidiEvent.atom.size = 3;
        atomMidiEvent.atom.type = fURIDs.midiEvent;

        atomMidiEvent.data[0] = channel + (velocity != 0 ? 0x90 : 0x80);
        atomMidiEvent.data[1] = note;
        atomMidiEvent.data[2] = velocity;

        // send to DSP side
        fWriteFunction(fController, eventInPortIndex, lv2_atom_total_size(&atomMidiEvent.atom),
                       fURIDs.atomEventTransfer, &atomMidiEvent);
    }
#endif

    bool fileRequest(const char* const key)
    {
        d_stdout("UI file request %s %p", key, fUiRequestValue);

        if (fUiRequestValue == nullptr)
            return false;

        String dpf_lv2_key(DISTRHO_PLUGIN_URI "#");
        dpf_lv2_key += key;

        const int r = fUiRequestValue->request(fUiRequestValue->handle,
                                        fUridMap->map(fUridMap->handle, dpf_lv2_key.buffer()),
                                        fURIDs.atomPath,
                                        nullptr);

        d_stdout("UI file request %s %p => %s %i", key, fUiRequestValue, dpf_lv2_key.buffer(), r);
        return r == LV2UI_REQUEST_VALUE_SUCCESS;
    }

private:
    UIExporter fUI;

    // LV2 features
    const LV2_URID_Map*        const fUridMap;
    const LV2UI_Port_Map*      const fUiPortMap;
    const LV2UI_Request_Value* const fUiRequestValue;
    const LV2UI_Touch*         const fUiTouch;

    // LV2 UI stuff
    const LV2UI_Controller     fController;
    const LV2UI_Write_Function fWriteFunction;

    // LV2 URIDs
    const struct URIDs {
        const LV2_URID_Map* _uridMap;
        LV2_URID dpfKeyValue;
        LV2_URID atomEventTransfer;
        LV2_URID atomFloat;
        LV2_URID atomLong;
        LV2_URID atomPath;
        LV2_URID atomString;
        LV2_URID midiEvent;
        LV2_URID paramSampleRate;
        LV2_URID patchSet;

        URIDs(const LV2_URID_Map* const uridMap)
            : _uridMap(uridMap),
              dpfKeyValue(map(DISTRHO_PLUGIN_LV2_STATE_PREFIX "KeyValueState")),
              atomEventTransfer(map(LV2_ATOM__eventTransfer)),
              atomFloat(map(LV2_ATOM__Float)),
              atomLong(map(LV2_ATOM__Long)),
              atomPath(map(LV2_ATOM__Path)),
              atomString(map(LV2_ATOM__String)),
              midiEvent(map(LV2_MIDI__MidiEvent)),
              paramSampleRate(map(LV2_PARAMETERS__sampleRate)),
              patchSet(map(LV2_PATCH__Set)) {}

        inline LV2_URID map(const char* const uri) const
        {
            return _uridMap->map(_uridMap->handle, uri);
        }
    } fURIDs;

    // index of bypass parameter, if present
    const uint32_t fBypassParameterIndex;

    // using ui:showInterface if true
    const bool fWinIdWasNull;

    // -------------------------------------------------------------------
    // Callbacks

    #define uiPtr ((UiLv2*)ptr)

    static void editParameterCallback(void* ptr, uint32_t rindex, bool started)
    {
        uiPtr->editParameterValue(rindex, started);
    }

    static void setParameterCallback(void* ptr, uint32_t rindex, float value)
    {
        uiPtr->setParameterValue(rindex, value);
    }

    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
        uiPtr->setState(key, value);
    }

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    static void sendNoteCallback(void* ptr, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        uiPtr->sendNote(channel, note, velocity);
    }
#endif

    static bool fileRequestCallback(void* ptr, const char* key)
    {
        return uiPtr->fileRequest(key);
    }

    #undef uiPtr
};

// -----------------------------------------------------------------------

static LV2UI_Handle lv2ui_instantiate(const LV2UI_Descriptor*,
                                      const char* const uri,
                                      const char* const bundlePath,
                                      const LV2UI_Write_Function writeFunction,
                                      const LV2UI_Controller controller,
                                      LV2UI_Widget* const widget,
                                      const LV2_Feature* const* const features)
{
    if (uri == nullptr || std::strcmp(uri, DISTRHO_PLUGIN_URI) != 0)
    {
        d_stderr("Invalid plugin URI");
        return nullptr;
    }

    const LV2_Options_Option* options   = nullptr;
    const LV2_URID_Map*       uridMap   = nullptr;
    void*                     parentId  = nullptr;
    void*                     instance  = nullptr;

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
    struct LV2_DirectAccess_Interface {
        void* (*get_instance_pointer)(LV2_Handle handle);
    };
    const LV2_Extension_Data_Feature* extData = nullptr;
#endif

    for (int i=0; features[i] != nullptr; ++i)
    {
        /**/ if (std::strcmp(features[i]->URI, LV2_OPTIONS__options) == 0)
            options = (const LV2_Options_Option*)features[i]->data;
        else if (std::strcmp(features[i]->URI, LV2_URID__map) == 0)
            uridMap = (const LV2_URID_Map*)features[i]->data;
        else if (std::strcmp(features[i]->URI, LV2_UI__parent) == 0)
            parentId = features[i]->data;
#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
        else if (std::strcmp(features[i]->URI, LV2_DATA_ACCESS_URI) == 0)
            extData = (const LV2_Extension_Data_Feature*)features[i]->data;
        else if (std::strcmp(features[i]->URI, LV2_INSTANCE_ACCESS_URI) == 0)
            instance = features[i]->data;
#endif
    }

    if (options == nullptr && parentId == nullptr)
    {
        d_stderr("Options feature missing (needed for show-interface), cannot continue!");
        return nullptr;
    }

    if (uridMap == nullptr)
    {
        d_stderr("URID Map feature missing, cannot continue!");
        return nullptr;
    }

    if (parentId == nullptr)
    {
        d_stdout("Parent Window Id missing, host should be using ui:showInterface...");
    }

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
    if (extData == nullptr || instance == nullptr)
    {
        d_stderr("Data or instance access missing, cannot continue!");
        return nullptr;
    }

    if (const LV2_DirectAccess_Interface* const directAccess = (const LV2_DirectAccess_Interface*)extData->data_access(DISTRHO_PLUGIN_LV2_STATE_PREFIX "direct-access"))
        instance = directAccess->get_instance_pointer(instance);
    else
        instance = nullptr;

    if (instance == nullptr)
    {
        d_stderr("Failed to get direct access, cannot continue!");
        return nullptr;
    }
#endif

    const intptr_t winId = (intptr_t)parentId;
    float sampleRate = 0.0f;
    float scaleFactor = 1.0f;
    uint32_t bgColor = 0;
    uint32_t fgColor = 0xffffffff;

    if (options != nullptr)
    {
        const LV2_URID uridAtomInt     = uridMap->map(uridMap->handle, LV2_ATOM__Int);
        const LV2_URID uridAtomFloat   = uridMap->map(uridMap->handle, LV2_ATOM__Float);
        const LV2_URID uridSampleRate  = uridMap->map(uridMap->handle, LV2_PARAMETERS__sampleRate);
        const LV2_URID uridBgColor     = uridMap->map(uridMap->handle, LV2_UI__backgroundColor);
        const LV2_URID uridFgColor     = uridMap->map(uridMap->handle, LV2_UI__foregroundColor);
        const LV2_URID uridScaleFactor = uridMap->map(uridMap->handle, LV2_UI__scaleFactor);

        for (int i=0; options[i].key != 0; ++i)
        {
            /**/ if (options[i].key == uridSampleRate)
            {
                if (options[i].type == uridAtomFloat)
                    sampleRate = *(const float*)options[i].value;
                else
                    d_stderr("Host provides UI sample-rate but has wrong value type");
            }
            else if (options[i].key == uridScaleFactor)
            {
                if (options[i].type == uridAtomFloat)
                    scaleFactor = *(const float*)options[i].value;
                else
                    d_stderr("Host provides UI scale factor but has wrong value type");
            }
            else if (options[i].key == uridBgColor)
            {
                if (options[i].type == uridAtomInt)
                    bgColor = (uint32_t)*(const int32_t*)options[i].value;
                else
                    d_stderr("Host provides UI background color but has wrong value type");
            }
            else if (options[i].key == uridFgColor)
            {
                if (options[i].type == uridAtomInt)
                    fgColor = (uint32_t)*(const int32_t*)options[i].value;
                else
                    d_stderr("Host provides UI foreground color but has wrong value type");
            }
        }
    }

    if (sampleRate < 1.0)
    {
        d_stdout("WARNING: this host does not send sample-rate information for LV2 UIs, using 44100 as fallback (this could be wrong)");
        sampleRate = 44100.0;
    }

    return new UiLv2(bundlePath, winId, options, uridMap, features,
                     controller, writeFunction, widget, instance,
                     sampleRate, scaleFactor, bgColor, fgColor);
}

#define uiPtr ((UiLv2*)ui)

static void lv2ui_cleanup(LV2UI_Handle ui)
{
    delete uiPtr;
}

static void lv2ui_port_event(LV2UI_Handle ui, uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer)
{
    uiPtr->lv2ui_port_event(portIndex, bufferSize, format, buffer);
}

// -----------------------------------------------------------------------

static int lv2ui_idle(LV2UI_Handle ui)
{
    return uiPtr->lv2ui_idle();
}

static int lv2ui_show(LV2UI_Handle ui)
{
    return uiPtr->lv2ui_show();
}

static int lv2ui_hide(LV2UI_Handle ui)
{
    return uiPtr->lv2ui_hide();
}

// -----------------------------------------------------------------------

static uint32_t lv2_get_options(LV2UI_Handle ui, LV2_Options_Option* options)
{
    return uiPtr->lv2_get_options(options);
}

static uint32_t lv2_set_options(LV2UI_Handle ui, const LV2_Options_Option* options)
{
    return uiPtr->lv2_set_options(options);
}

// -----------------------------------------------------------------------

#if DISTRHO_PLUGIN_WANT_PROGRAMS
static void lv2ui_select_program(LV2UI_Handle ui, uint32_t bank, uint32_t program)
{
    uiPtr->lv2ui_select_program(bank, program);
}
#endif

// -----------------------------------------------------------------------

static const void* lv2ui_extension_data(const char* uri)
{
    static const LV2_Options_Interface options = { lv2_get_options, lv2_set_options };
    static const LV2UI_Idle_Interface  uiIdle  = { lv2ui_idle };
    static const LV2UI_Show_Interface  uiShow  = { lv2ui_show, lv2ui_hide };

    if (std::strcmp(uri, LV2_OPTIONS__interface) == 0)
        return &options;
    if (std::strcmp(uri, LV2_UI__idleInterface) == 0)
        return &uiIdle;
    if (std::strcmp(uri, LV2_UI__showInterface) == 0)
        return &uiShow;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    static const LV2_Programs_UI_Interface uiPrograms = { lv2ui_select_program };

    if (std::strcmp(uri, LV2_PROGRAMS__UIInterface) == 0)
        return &uiPrograms;
#endif

    return nullptr;
}

#undef instancePtr

// -----------------------------------------------------------------------

static const LV2UI_Descriptor sLv2UiDescriptor = {
    DISTRHO_UI_URI,
    lv2ui_instantiate,
    lv2ui_cleanup,
    lv2ui_port_event,
    lv2ui_extension_data
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

DISTRHO_PLUGIN_EXPORT
const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
    USE_NAMESPACE_DISTRHO
    return (index == 0) ? &sLv2UiDescriptor : nullptr;
}

// -----------------------------------------------------------------------
