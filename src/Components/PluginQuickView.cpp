#include "PluginQuickView.h"
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include "Nodes/PluginHost.h"


PluginQuickView::PluginQuickView(QQmlApplicationEngine& engine, PluginHost& plugin) :
    QQuickView{&engine, nullptr},
    m_plugin{plugin}
{
    installEventFilter(this);
    setColor(Qt::transparent);
    setFlags(flags() | Qt::FramelessWindowHint | Qt::Window | Qt::WindowStaysOnTopHint);
    setResizeMode(ResizeMode::SizeRootObjectToView);

    loadFromModule("ClapWorkbench", "PluginGUI");

    QQuickItem* rootItem = rootObject();
    if (!rootItem)
        return;

    QVariant variant = QVariant::fromValue(static_cast<QObject*>(&m_plugin));
    rootItem->setProperty("plugin", std::move(variant));
    rootItem->setProperty("isFloating", true);

    setMinimumWidth(static_cast<int>(rootItem->width()));
    setMaximumWidth(static_cast<int>(rootItem->width()));
    setMinimumHeight(static_cast<int>(rootItem->height()));
    setMaximumHeight(static_cast<int>(rootItem->height()));

    connect(rootItem, &QQuickItem::widthChanged, [this, rootItem]()
    {
        setMinimumWidth(static_cast<int>(rootItem->width()));
        setMaximumWidth(static_cast<int>(rootItem->width()));
    });
    connect(rootItem, &QQuickItem::heightChanged, [this, rootItem]()
    {
        setMinimumHeight(static_cast<int>(rootItem->height()));
        setMaximumHeight(static_cast<int>(rootItem->height()));
    });

    show();
}

PluginQuickView::~PluginQuickView() = default;

void PluginQuickView::addKeyPressListener(QQuickWindow* windowToListenTo)
{
    windowToListenTo->installEventFilter(this);
}

void PluginQuickView::closeEvent(QCloseEvent* close_event)
{
    QQuickView::closeEvent(close_event);
}

bool PluginQuickView::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() != QEvent::Type::KeyPress)
        return QObject::eventFilter(obj, event);

    if (const QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);
        keyEvent->key() != Qt::Key::Key_Escape)
    {
        return QObject::eventFilter(obj, event);
    }

    close();
    return true;
}
