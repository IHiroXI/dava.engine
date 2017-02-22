#pragma once

#include "Engine/Private/Qt/RenderWidgetBackend.h"
#include "Base/BaseTypes.h"
#include <QWidget>

namespace DAVA
{
class RenderWidgetDX : public RenderWidgetBackendImpl<QWidget>
{
    using TBase = RenderWidgetBackendImpl<QWidget>;

public:
    RenderWidgetDX(IWindowDelegate* windowDelegate, uint32 width, uint32 height, QWidget* parent);

    void ActivateRendering() override;
    bool IsInitialized() const override;
    void Update() override;
    void InitCustomRenderParams(rhi::InitParam& params) override;
    void AcquireContext() override;
    void ReleaseContext() override;

protected:
    bool eventFilter(QObject* obj, QEvent* e);

    bool IsInFullScreen() const override;
    QWindow* GetQWindow() override;
    QPaintEngine* paintEngine() const override;
    void OnCreated() override;
    void OnFrame() override;
    void OnDestroyed() override;

    class RenderSurface;
    RenderSurface* surface = nullptr;
};
} // namespace DAVA