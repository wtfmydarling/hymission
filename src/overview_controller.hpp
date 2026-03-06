#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <hyprland/src/SharedDefs.hpp>
#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/devices/IKeyboard.hpp>
#include <hyprland/src/devices/IPointer.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/helpers/signal/Signal.hpp>
#include <hyprland/src/helpers/time/Time.hpp>
#include <hyprland/src/layout/algorithm/Algorithm.hpp>
#include <hyprland/src/layout/algorithm/TiledAlgorithm.hpp>
#include <hyprland/src/layout/space/Space.hpp>
#include <hyprland/src/layout/supplementary/WorkspaceAlgoMatcher.hpp>
#include <hyprland/src/plugins/HookSystem.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/render/pass/SurfacePassElement.hpp>

#include "mission_layout.hpp"
#include "overview_logic.hpp"

namespace hymission {

class OverviewController {
  public:
    enum class ScopeOverride {
        Default,
        OnlyCurrentWorkspace,
        ForceAll,
    };

    explicit OverviewController(HANDLE handle);
    ~OverviewController();

    OverviewController(const OverviewController&) = delete;
    OverviewController& operator=(const OverviewController&) = delete;

    bool initialize();

    [[nodiscard]] SDispatchResult open(const std::string& args = {});
    [[nodiscard]] SDispatchResult close();
    [[nodiscard]] SDispatchResult toggle(const std::string& args = {});
    [[nodiscard]] SDispatchResult debugCurrentLayout() const;

    void renderStage(eRenderStage stage);
    void handleMouseMove();
    void handleMouseButton(const IPointer::SButtonEvent& event, Event::SCallbackInfo& info);
    void handleKeyboard(const IKeyboard::SKeyEvent& event, Event::SCallbackInfo& info);
    void handleWindowSetChange(PHLWINDOW window);
    void handleWorkspaceChange();
    void handleMonitorChange(PHLMONITOR monitor);
    bool                shouldRenderWindowHook(const PHLWINDOW& window, const PHLMONITOR& monitor);
    void                borderDrawHook(void* borderDecorationThisptr, const PHLMONITOR& monitor, const float& alpha);
    void                shadowDrawHook(void* shadowDecorationThisptr, const PHLMONITOR& monitor, const float& alpha);
    void                calculateUVForSurfaceHook(const PHLWINDOW& window, SP<CWLSurfaceResource> surface, const PHLMONITOR& monitor, bool main, const Vector2D& projSize,
                                                  const Vector2D& projSizeUnscaled, bool fixMisalignedFSV1);
    [[nodiscard]] SDispatchResult fullscreenDispatcherHook(std::string args);
    [[nodiscard]] SDispatchResult fullscreenStateDispatcherHook(std::string args);
    bool                surfaceNeedsLiveBlurHook(void* surfacePassThisptr);
    bool                surfaceNeedsPrecomputeBlurHook(void* surfacePassThisptr);
    void                surfaceDrawHook(void* surfacePassThisptr, const CRegion& damage);
    CBox                surfaceTexBoxHook(void* surfacePassThisptr);
    std::optional<CBox> surfaceBoundingBoxHook(void* surfacePassThisptr);
    CRegion             surfaceOpaqueRegionHook(void* surfacePassThisptr);
    CRegion             surfaceVisibleRegionHook(void* surfacePassThisptr, bool& cancel);
  private:
    enum class Phase {
        Inactive,
        Opening,
        Active,
        ClosingSettle,
        Closing,
    };

    enum class CloseMode {
        Normal,
        ActivateSelection,
        Abort,
    };

    enum class PostCloseDispatcher {
        None,
        Fullscreen,
        FullscreenState,
    };

    struct CollectionPolicy {
        ScopeOverride requestedScope = ScopeOverride::Default;
        bool          onlyActiveWorkspace = false;
        bool          onlyActiveMonitor = true;
        bool          includeSpecial = false;
    };

    struct FullscreenWorkspaceBackup {
        PHLWORKSPACE    workspace;
        PHLWINDOW       originalFullscreenWindow;
        bool            hadFullscreenWindow = false;
        eFullscreenMode fullscreenMode = FSMODE_NONE;
        eFullscreenMode originalFullscreenMode = FSMODE_NONE;
    };

    struct ManagedWindow {
        PHLWINDOW    window;
        PHLMONITOR   targetMonitor;
        std::string  title;
        Rect         naturalGlobal;
        Rect         exitGlobal;
        Rect         relayoutFromGlobal;
        Rect         targetGlobal;
        WindowSlot   slot;
        float        previewAlpha = 1.0F;
        bool         isFloating = false;
        bool         isPinned = false;
    };

    struct State {
        Phase                                  phase = Phase::Inactive;
        PHLMONITOR                             ownerMonitor;
        PHLWORKSPACE                           ownerWorkspace;
        CollectionPolicy                       collectionPolicy;
        std::vector<PHLMONITOR>                participatingMonitors;
        std::vector<PHLWORKSPACE>              managedWorkspaces;
        std::vector<FullscreenWorkspaceBackup> fullscreenBackups;
        PHLWINDOW                              focusBeforeOpen;
        PHLWINDOW                              focusDuringOverview;
        PHLWINDOW                              pendingExitFocus;
        CloseMode                              closeMode = CloseMode::Normal;
        bool                                   fullscreenOverrideActive = false;
        std::vector<ManagedWindow>             windows;
        std::vector<ManagedWindow>             transientClosingWindows;
        std::vector<WindowSlot>                slots;
        std::optional<std::size_t>             hoveredIndex;
        std::optional<std::size_t>             selectedIndex;
        double                                 animationProgress = 0.0;
        double                                 animationFromVisual = 0.0;
        double                                 animationToVisual = 0.0;
        double                                 relayoutProgress = 1.0;
        bool                                   relayoutActive = false;
        std::size_t                            settleStableFrames = 0;
        bool                                   settleHasSample = false;
        bool                                   exitFullscreenReapplied = false;
        std::chrono::steady_clock::time_point  animationStart = {};
        std::chrono::steady_clock::time_point  relayoutStart = {};
        std::chrono::steady_clock::time_point  settleStart = {};
    };

    struct SurfaceRenderDataSnapshot {
        Vector2D pos;
        Vector2D localPos;
        double   w = 0.0;
        double   h = 0.0;
        float    alpha = 1.0F;
        float    fadeAlpha = 1.0F;
        bool     blur = false;
        bool     blockBlurOptimization = false;
        CBox     clipBox = {};
    };

    struct WindowTransform {
        Rect   actualGlobal;
        Rect   targetGlobal;
        double scaleX = 1.0;
        double scaleY = 1.0;
    };

    using SurfaceGetTexBoxFn = CBox (*)(void*);
    using SurfaceBoundingBoxFn = std::optional<CBox> (*)(void*);
    using SurfaceOpaqueRegionFn = CRegion (*)(void*);
    using SurfaceVisibleRegionFn = CRegion (*)(void*, bool&);
    using SurfaceDrawFn = void (*)(void*, const CRegion&);
    using SurfaceBlurNeedsFn = bool (*)(void*);
    using ShouldRenderWindowFn = bool (*)(void*, PHLWINDOW, PHLMONITOR);
    using BorderDrawFn = void (*)(void*, PHLMONITOR, const float&);
    using CalculateUVForSurfaceFn = void (*)(void*, PHLWINDOW, SP<CWLSurfaceResource>, PHLMONITOR, bool, const Vector2D&, const Vector2D&, bool);
    using DispatcherFn = SDispatchResult (*)(std::string);
    [[nodiscard]] LayoutConfig loadLayoutConfig() const;
    [[nodiscard]] CollectionPolicy loadCollectionPolicy(ScopeOverride requestedScope) const;
    [[nodiscard]] std::optional<ScopeOverride> parseScopeOverride(const std::string& args, std::string& error) const;
    [[nodiscard]] bool         focusFollowsMouseEnabled() const;
    [[nodiscard]] bool         debugLogsEnabled() const;
    [[nodiscard]] bool         debugSurfaceLogsEnabled() const;
    [[nodiscard]] bool         isScrollingWorkspace(const PHLWORKSPACE& workspace) const;
    [[nodiscard]] bool         hasScrollingWorkspace() const;
    [[nodiscard]] bool         shouldSyncRealFocusDuringOverview() const;
    void                       setInputFollowMouseOverride(bool disable);
    void                       setScrollingFollowFocusOverride(bool disable);
    [[nodiscard]] bool         installHooks();
    [[nodiscard]] bool         activateHooks();
    void                       deactivateHooks();
    [[nodiscard]] bool         hookFunction(const std::string& symbolName, const std::string& demangledNeedle, CFunctionHook*& hook, void* destination);
    [[nodiscard]] void*        findFunction(const std::string& symbolName, const std::string& demangledNeedle) const;

    [[nodiscard]] bool         isAnimating() const;
    [[nodiscard]] bool         isVisible() const;
    [[nodiscard]] bool         shouldHandleInput() const;
    [[nodiscard]] bool         ownsMonitor(const PHLMONITOR& monitor) const;
    [[nodiscard]] bool         ownsWorkspace(const PHLWORKSPACE& workspace) const;
    [[nodiscard]] bool         hasManagedWindow(const PHLWINDOW& window) const;
    [[nodiscard]] bool         shouldAutoCloseFor(const PHLWINDOW& window) const;
    [[nodiscard]] bool         shouldManageWindow(const PHLWINDOW& window, const State& state) const;
    [[nodiscard]] std::string  collectionSummary(const PHLMONITOR& monitor) const;
    [[nodiscard]] std::vector<Rect> targetRects() const;
    [[nodiscard]] const ManagedWindow* managedWindowFor(const PHLWINDOW& window) const;
    [[nodiscard]] PHLWINDOW     selectedWindow() const;
    [[nodiscard]] float        managedPreviewAlphaFor(const PHLWINDOW& window, float fallback = 1.0F) const;
    [[nodiscard]] PHLMONITOR   preferredMonitorForWindow(const PHLWINDOW& window, const State& state) const;
    [[nodiscard]] PHLMONITOR   previewMonitorForWindow(const PHLWINDOW& window) const;
    [[nodiscard]] const FullscreenWorkspaceBackup* fullscreenBackupForWorkspace(const PHLWORKSPACE& workspace) const;
    [[nodiscard]] const FullscreenWorkspaceBackup* fullscreenBackupForWindow(const PHLWINDOW& window) const;
    [[nodiscard]] Rect         liveGlobalRectForWindow(const PHLWINDOW& window) const;
    [[nodiscard]] Rect         goalGlobalRectForWindow(const PHLWINDOW& window) const;
    [[nodiscard]] std::optional<WindowTransform> windowTransformFor(const PHLWINDOW& window, const PHLMONITOR& monitor) const;
    [[nodiscard]] bool                          transformSurfaceRenderDataForWindow(const PHLWINDOW& window, const PHLMONITOR& monitor,
                                                                                   CSurfacePassElement::SRenderData& renderData) const;
    [[nodiscard]] bool                          shouldSuppressSurfaceBlur(void* surfacePassThisptr) const;
    [[nodiscard]] bool                          prepareSurfaceRenderData(void* surfacePassThisptr, const char* context, CSurfacePassElement::SRenderData*& renderData,
                                                                         PHLMONITOR& monitor, SurfaceRenderDataSnapshot& snapshot) const;
    void                                        restoreSurfaceRenderData(CSurfacePassElement::SRenderData* renderData, const SurfaceRenderDataSnapshot& snapshot) const;
    [[nodiscard]] std::optional<std::size_t> hitTestTarget(double x, double y) const;
    [[nodiscard]] Rect         currentPreviewRect(const ManagedWindow& window) const;
    [[nodiscard]] double       visualProgress() const;
    [[nodiscard]] double       relayoutVisualProgress() const;
    [[nodiscard]] bool         transformBoxForWindow(const PHLWINDOW& window, const PHLMONITOR& monitor, CBox& box, bool scaled) const;
    [[nodiscard]] CRegion      transformRegionForWindow(const PHLWINDOW& window, const PHLMONITOR& monitor, const CRegion& region, bool scaled) const;
    [[nodiscard]] PHLWINDOW    resolveExitFocus(CloseMode mode) const;
    [[nodiscard]] std::optional<Vector2D> visiblePointForWindowOnMonitor(const PHLWINDOW& window, const PHLMONITOR& monitor, bool preferGoal = false) const;
    [[nodiscard]] bool         clearWorkspaceFullscreenForExitTarget(const PHLWINDOW& window);
    void                       commitOverviewExitFocus(const PHLWINDOW& window);
    void                       refreshExitLayoutForFocus(const PHLWINDOW& window) const;
    void                       syncRealFocusDuringOverview(const PHLWINDOW& window);
    void                       syncFocusDuringOverviewFromSelection();
    void                       clearPostCloseForcedFocus();
    void                       clearPostCloseDispatcher();
    void                       queuePostCloseDispatcher(PostCloseDispatcher dispatcher, std::string args);
    [[nodiscard]] SDispatchResult runHookedDispatcher(PostCloseDispatcher dispatcher, std::string args);
    void                       setFullscreenRenderOverride(bool suppress);

    void beginOpen(const PHLMONITOR& monitor, ScopeOverride requestedScope);
    void beginClose(CloseMode mode = CloseMode::Normal);
    void deactivate();
    void refreshScene(const PHLMONITOR& monitor) const;
    void refreshOwnedMonitors() const;
    void damageOwnedMonitor() const;
    void updateAnimation();
    void updateHoveredFromPointer();
    void rebuildVisibleState();
    void moveSelection(Direction direction);
    void activateSelection();
    void notify(const std::string& message, const CHyprColor& color, float durationMs) const;
    void debugLog(const std::string& message) const;
    void debugSurfaceLog(const std::string& message) const;
    [[nodiscard]] std::string debugWindowLabel(const PHLWINDOW& window) const;
    void renderBackdrop() const;
    void renderSelectionChrome() const;
    void renderOutline(const Rect& rect, const CHyprColor& color, double thickness) const;
    State  buildState(const PHLMONITOR& monitor, ScopeOverride requestedScope) const;
    State  m_state;
    HANDLE m_handle = nullptr;

    CFunctionHook*            m_surfaceTexBoxHook = nullptr;
    CFunctionHook*            m_surfaceBoundingBoxHook = nullptr;
    CFunctionHook*            m_surfaceOpaqueRegionHook = nullptr;
    CFunctionHook*            m_surfaceVisibleRegionHook = nullptr;
    CFunctionHook*            m_surfaceDrawHook = nullptr;
    CFunctionHook*            m_surfaceNeedsLiveBlurHook = nullptr;
    CFunctionHook*            m_surfaceNeedsPrecomputeBlurHook = nullptr;
    CFunctionHook*            m_shouldRenderWindowHook = nullptr;
    CFunctionHook*            m_borderDrawHook = nullptr;
    CFunctionHook*            m_shadowDrawHook = nullptr;
    CFunctionHook*            m_calculateUVForSurfaceHook = nullptr;
    CFunctionHook*            m_fullscreenActiveHook = nullptr;
    CFunctionHook*            m_fullscreenStateActiveHook = nullptr;
    SurfaceGetTexBoxFn        m_surfaceTexBoxOriginal = nullptr;
    SurfaceBoundingBoxFn      m_surfaceBoundingBoxOriginal = nullptr;
    SurfaceOpaqueRegionFn     m_surfaceOpaqueRegionOriginal = nullptr;
    SurfaceVisibleRegionFn    m_surfaceVisibleRegionOriginal = nullptr;
    SurfaceDrawFn             m_surfaceDrawOriginal = nullptr;
    SurfaceBlurNeedsFn        m_surfaceNeedsLiveBlurOriginal = nullptr;
    SurfaceBlurNeedsFn        m_surfaceNeedsPrecomputeBlurOriginal = nullptr;
    ShouldRenderWindowFn      m_shouldRenderWindowOriginal = nullptr;
    BorderDrawFn              m_borderDrawOriginal = nullptr;
    BorderDrawFn              m_shadowDrawOriginal = nullptr;
    CalculateUVForSurfaceFn   m_calculateUVForSurfaceOriginal = nullptr;
    DispatcherFn              m_fullscreenActiveOriginal = nullptr;
    DispatcherFn              m_fullscreenStateActiveOriginal = nullptr;
    bool                      m_hooksActive = false;
    bool                      m_inputFollowMouseOverridden = false;
    long                      m_inputFollowMouseBackup = 1;
    bool                      m_restoreInputFollowMouseAfterPostClose = false;
    bool                      m_scrollingFollowFocusOverridden = false;
    long                      m_scrollingFollowFocusBackup = 1;
    bool                      m_deactivatePending = false;
    std::size_t               m_surfaceRenderDataTransformDepth = 0;
    PHLWINDOWREF              m_postCloseForcedFocus;
    bool                      m_postCloseForcedFocusLatched = false;
    std::size_t               m_ignorePostCloseMouseMoveCount = 0;
    PostCloseDispatcher       m_postCloseDispatcher = PostCloseDispatcher::None;
    std::string               m_postCloseDispatcherArgs;

    CHyprSignalListener       m_renderStageListener;
    CHyprSignalListener       m_mouseMoveListener;
    CHyprSignalListener       m_mouseButtonListener;
    CHyprSignalListener       m_keyboardListener;
    CHyprSignalListener       m_windowOpenListener;
    CHyprSignalListener       m_windowDestroyListener;
    CHyprSignalListener       m_windowCloseListener;
    CHyprSignalListener       m_windowMoveWorkspaceListener;
    CHyprSignalListener       m_workspaceActiveListener;
    CHyprSignalListener       m_monitorRemovedListener;
    CHyprSignalListener       m_monitorFocusedListener;
};

} // namespace hymission
