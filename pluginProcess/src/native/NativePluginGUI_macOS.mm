#include "NativePluginGUI.h"
#import <Cocoa/Cocoa.h>
#include <choc/gui/choc_DesktopWindow.h>
//#include "State.h"

@interface PluginKeyView : NSView
@property (nonatomic, copy) void (^onEscapePressed)(void);
@end

@implementation PluginKeyView

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent *)event {
    if (event.keyCode == 53) { // 53 = Escape key
        if (self.onEscapePressed) {
            self.onEscapePressed();
        }
    } else {
        [super keyDown:event];
    }
}

@end


struct NativeWindow
{
    NativeWindow(int pluginId) : m_pluginId{pluginId}, m_window({ 0, 0, 200, 200 })
    {
        PluginKeyView* view = [[PluginKeyView alloc] initWithFrame:NSMakeRect(0, 0, 200, 200)];
        pluginView = view;

        view.onEscapePressed = ^{
//            m_state.plugins.nativeGuiVisible(m_pluginId);
        };

        m_window.setContent(pluginView);
        m_window.setResizable(false);

        m_window.windowClosed = [&]()
        {
//            m_state.plugins.nativeGuiVisible(m_pluginId);
        };

        m_window.toFront();
        [[pluginView window] makeFirstResponder:pluginView];
    }

    [[nodiscard]] clap_window getClapWindow() const
    {
        clap_window w = {};
        w.api = CLAP_WINDOW_API_COCOA;
        w.cocoa = reinterpret_cast<clap_nsview>(pluginView);

        return w;
    }

    void setSize(const int width, const int height)
    {
        // TODO: figure out how to keep track of x,y with choc so we can persist it
        m_window.centreWithSize(width, height);

        pluginView.frame = NSMakeRect(0, 0, width, height);
    }

//    State& m_state;
    int m_pluginId;
    choc::ui::DesktopWindow m_window;
    NSView* pluginView = nil;
};



NativePluginGUI::NativePluginGUI(int pluginId)
{
    m_native_window = std::make_unique<NativeWindow>(pluginId);
}

NativePluginGUI::~NativePluginGUI() = default;

clap_window NativePluginGUI::getClapWindow()
{
    return m_native_window->getClapWindow();
}

void NativePluginGUI::setSize(int width, int height)
{
    m_native_window->setSize(width, height);
}