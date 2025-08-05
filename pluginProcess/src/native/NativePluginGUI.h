#pragma once
#include <memory>
#include <clap/ext/gui.h>

class State;
struct NativeWindow;


class NativePluginGUI
{
  public:
    NativePluginGUI(int pluginId);
    ~NativePluginGUI();

    clap_window getClapWindow();
    void setSize(int width, int height);

private:
    std::unique_ptr<NativeWindow> m_native_window;
};
