#include "PluginQuickView.h"
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include "PluginHost.h"


PluginQuickView::PluginQuickView(QQmlApplicationEngine& engine, PluginHost& plugin) :
    QQuickView{&engine, nullptr},
    m_plugin{plugin}
{
    installEventFilter(this);
    setFlags(flags() | Qt::WindowStaysOnTopHint | Qt::Tool);
    setResizeMode(ResizeMode::SizeRootObjectToView);

    connect(this, &QQuickView::statusChanged, this, [this](const Status status)
    {
        if (status != Status::Ready)
            return;

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
            setMinimumWidth(rootItem->width());
            setMaximumWidth(rootItem->width());
        });
        connect(rootItem, &QQuickItem::heightChanged, [this, rootItem]()
        {
            setMinimumHeight(rootItem->height());
            setMaximumHeight(rootItem->height());
        });
    });

    loadFromModule("ClapWorkbench", "PluginGUI");
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
