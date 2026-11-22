# UWP XAML in Raven — plan

Status: stage 0/1 started; desktop CoreWindow, mouse/keyboard events, async actions, thread teardown and single-view startup verified (`native/upstream/raven-xaml`, `native_core_window_x64` / `_x32`). Stage 2's composition object model and CPU presentation are in place (`native_composition_x64`/`_x32`). Stage 3's interface layer is in place: application-defined XAML types resolve through the app's `IXamlMetadataProvider`, and `Grid`/`TextBlock`/`Button` build a real element tree (see the 2026-10-28 sections).

Goal: run Windows Runtime/XAML applications such as Shapr3D without Microsoft's
XAML binaries, by implementing the Windows Runtime surface they bind to and
mapping its rendering onto Raven's existing desktop, Direct2D, DirectWrite and
Direct3D (DXVK) stack.

Non-goals: running `Windows.UI.Xaml.dll` from a Windows installation, emulating
DWM, or translating shipped .NET Native binaries into another framework. Those
are blocked (or made fragile) by the platform contracts below, not by missing
APIs.

## Why the engine is reimplemented

`Windows.UI.Xaml.Application` is not a small service: an application subclasses
it, implements its override interfaces and ships its UI as compiled XBF. For
Shapr3D specifically the measured surface is:

- 397 distinct `Windows.UI.Xaml.*` symbols in `Shapr3D.dll` (99 interfaces, 179
  override members), 50 `Microsoft.UI.Xaml.*` symbols, Win2D
  (`CanvasControl`, `CanvasSwapChain`), `Windows.UI.Composition`,
  `Windows.UI.Text`, `Windows.UI.Input`, `Windows.Storage`,
  `Windows.ApplicationModel.Package`.
- 304 compiled XAML pages (`.xbf`) in `resources.pri`.
- A package manifest declaring a UWP XAML application
  (`EntryPoint="Shapr3D.App"`, `uap:VisualElements`, `Microsoft.UI.Xaml.2.8`).

`Windows.UI.Xaml.dll` is an inbox OS component (20 MB on Windows 11), not a
redistributable package; it calls `dcomp`, DWM, `CoreMessaging`,
`twinapi.appcore`, `UIAutomationCore`, `WindowManagementAPI`, `NInput` and
private `ext-ms-win-*` contracts. WinUI 2.8 is only a control layer on top of
it, and WinUI 3/Uno use different ABIs or need source.

## Architecture

```
application (.NET Native / guest PE)
        │  WinRT activation, HSTRING, async
        ▼
┌──────────────────────────────────────────────────────────────┐
│ raven-xaml.dll  (guest component, mingw, beside the runtime) │
│   Windows.UI.Xaml.*      XBF, types, DP, styles, layout      │
│   Windows.UI.Core.*      CoreWindow, CoreDispatcher          │
│   Windows.UI.Composition Composition visual tree             │
│   Windows.System.*       DispatcherQueue, MemoryManager      │
└──────────────────────────────────────────────────────────────┘
        │  host calls (small, explicit ABI)
        ▼
┌──────────────────────────────────────────────────────────────┐
│ Raven host                                                   │
│   desktop/windows.cpp   window, input, message loop          │
│   desktop/surface       CPU pixels (Skia) / GPU host surface │
│   graphics              D2D1, DWrite, D3D11 → DXVK → Vulkan  │
└──────────────────────────────────────────────────────────────┘
```

Decisions:

1. **No DWM.** XAML visuals become Raven desktop surfaces. A GPU-backed
   compositor is the target; `wl_shm` CPU presentation is only the fallback.
2. **GPU-first frame path.** The 3D viewport must composite on the GPU (or as a
   host child surface). No GPU→CPU readback of a `SwapChainPanel`.
3. **Guest components, like `raven-deployment.dll`.** Classes are implemented in
   `native/upstream/raven-*/`, built by CMake, listed in `runtime_components`
   (`native/nt/system_exports.h`) and registered in the WinRT catalog
   (`NativeRegistry::register_runtime_classes`, `native/nt/registry.cpp`).
4. **ABI comes from headers/winmd, never from guesses.** mingw-w64 already
   provides C WinRT headers for Core, Composition, Foundation and the
   dependency-object part of XAML. XAML proper (`Application`, controls, media)
   is not covered and will be generated from
   `System32/WinMetadata/Windows.UI.Xaml.winmd`.
5. **Package identity is implemented from the app's manifest**, not from the
   Windows package database. MRT Core reads `resources.pri` directly.
6. **The shell component must be preloaded for WinRT activation.** Raven serves
   `combase.dll` from `raven-shell.dll` only when that component is in the
   module set, because the overlay is applied at provider construction
   (`native/nt/providers.cpp`). A program that imports `combase.dll`,
   `runtimeobject.dll` or a WinRT API set therefore loads it up front
   (`native/main.cpp`); without that, `RoGetActivationFactory` falls through to
   a host stub that answers class-not-registered.

## Stages

Each stage ends with a test that runs under `raven --run-process
--execution=native` in the normal suite, for x64 and x86 where the component is
built for both.

### Stage 0 — activation scaffolding (started)

- `raven-xaml.dll`, `DllGetActivationFactory`, catalog entry.
- Acceptance: a client activates a class by name and QIs its interface.
- State: `Windows.UI.Core.CoreWindow` activates for x64 and x86; see stage 1.

### Stage 1 — CoreWindow, CoreDispatcher, DispatcherQueue (started)

- Per-thread `CoreWindow`, dispatcher queue integrated with Raven's message
  loop, `RunAsync`/`ProcessEvents`, task continuation.
- Acceptance (`native_core_window_x64/x32`): same window per thread; a worker
  posts to a waiting UI dispatcher; callbacks execute on the owning thread;
  completion registration racing with execution delivers exactly once.
- State: cross-thread `RunAsync` wakes a queue event. `ProcessOneIfPresent`,
  `ProcessAllIfPresent`, blocking `ProcessOneAndAllPending` and `ProcessUntilQuit`
  are implemented; pumping from another thread returns `RPC_E_WRONG_THREAD`.
  Priorities run high to idle, FIFO within each priority. `CoreWindow.Close`
  stops the pump and completes pending/new dispatch actions without invoking
  their callbacks, following the documented `RunAsync` shutdown behavior.
- Actions expose `IAsyncInfo` (identity, ID, status, error, cancellation, close).
  Handler failures propagate to `Error`/`GetResults`; cancellation skips queued
  callbacks. Completion assignment and execution share a lock, and handlers
  remain available through `get_Completed` until the action is closed/released.
- Desktop integration: the pump retrieves and dispatches desktop messages before
  queued callbacks, and waits with `MsgWaitForMultipleObjectsEx` on both input
  and the queue event. Existing desktop timers wake the same pump. `WM_QUIT`
  closes the CoreWindow and terminates `ProcessUntilQuit`.
- Thread teardown: `DLL_THREAD_DETACH` closes the queue, releases its cached
  dispatcher and drops the TLS window reference. External COM references may
  outlive the UI thread, but its window no longer hands out dispatchers.
  Raven now sends detach notifications on explicit `ExitThread` as well as
  normal thread return; forced thread termination still skips them.
- Application startup: `Windows.ApplicationModel.Core.CoreApplication` is in the
  WinRT catalog. `Run(IFrameworkViewSource)` calls `CreateView`, `Initialize`,
  `SetWindow`, `Load`, `Run` and `Uninitialize`, propagating errors and releasing
  source/provider references even if the UI thread calls `ExitThread`.
  `GetCurrentView` returns the running thread's view; other threads fail.
  Nested or concurrent `Run` calls are rejected. The view exposes its window,
  `IsMain=true`, `IsHosted=false` and revocable `Activated` subscriptions.
- Activation is posted at high priority to the window dispatcher after `Load`;
  handlers run when the view pumps events, after its `Run` begins. The launch
  arguments implement `IActivatedEventArgs` and `ILaunchActivatedEventArgs`.
  Event dispatch holds a snapshot, so handlers can revoke themselves or add
  subscriptions during dispatch. `CoreApplication.Exit` posts `WM_QUIT` to the
  UI thread, including from another thread; `Exiting` runs after `Uninitialize`.
- Application limits: one view; no suspend/resume event delivery
  (`E_NOTIMPL` — no UI thread exists until `Run` owns one, and the host
  delivers no lifecycle events). `RunWithActivationFactories` validates
  its callback (`E_POINTER` on NULL, `E_ILLEGAL_METHOD_CALL` while `Run`
  owns the UI thread, never retaining a refused callback) but the
  packaged/COM-launch run stays `E_NOTIMPL` — no activation host exists
  to invoke the callback. `Id` answers
  the manifest's `Applications/Application@Id` (the PRAID) when the process
  has package identity, and keeps forwarding the host's no-package error
  otherwise; `Properties` is a process-wide property set with real
  `IMap<HSTRING, IInspectable>` semantics (content-compared string keys,
  live view), not yet claiming the `PropertySet` runtime class name since
  its activation is not wired (see the PropertySet entry — claimed
  2026-10-31). Direct view-source
  startup has an empty `Load` entry point unless the process carries a
  manifest `EntryPoint`, which then travels to `Load` as its argument
  (2026-10-31); launch arguments and tile ID stay empty, and there is no
  splash screen. `RunWithActivationFactories` remains for stage 5; the
  manifest `EntryPoint` class name itself is not activated by the
  component — it names the guest's own Application subclass, resolved
  through the guest's `IXamlMetadataProvider`, so the factory chain keeps
  answering `CLASS_E_CLASSNOTAVAILABLE` for it.
- Desktop CoreWindow: each UI thread owns an initially hidden 800×600 client
  HWND. `Activate` shows and focuses it; `Close`, host `WM_CLOSE` and thread
  teardown destroy it. `ICoreWindowInterop` provides the HWND and one-byte
  `MessageHandled` ABI. Bounds report client origin relative to the outer window
  and client dimensions in DIPs using the desktop DPI.
- Desktop messages feed `Activated`, `Closed`, `SizeChanged`, `VisibilityChanged`,
  `InputEnabled`, `KeyDown`, `KeyUp` and `CharacterReceived`. Event arguments
  support their typed interfaces plus `ICoreWindowEventArgs.Handled`, preserve
  key repeat/scan/extended/menu/previous/release bits, and can outlive dispatch.
  UTF-16 character pairs become Unicode code points; malformed pairs use U+FFFD.
  Disabling input suppresses keyboard delivery. Handler failures propagate through
  `ProcessEvents` or the synchronous property/close call.
- Mouse input: `PointerEntered`, `PointerExited`, `PointerMoved`, `PointerPressed`,
  `PointerReleased`, `PointerWheelChanged` and `PointerCaptureLost` route desktop
  mouse messages. Their arguments expose immutable `PointerPoint` and
  `PointerPointProperties` snapshots, DIP client/raw position, all five buttons,
  update kind, signed wheel delta/axis, contact, key modifiers, frame ID and
  microseconds since boot. The single desktop mouse uses pointer ID 1 and a stable
  `PointerDevice` identity. Mouse pressure uses the public contract's 0.5 default;
  pen-only values are zero/false. No touch/pen input is synthesized as such.
- `SetPointerCapture` / `ReleasePointerCapture` use the native HWND; release does
  not disturb capture held by another window. `PointerPosition` reads the native
  cursor in client DIPs. Disabled windows suppress mouse delivery and reset
  enter tracking so re-enabled input produces a fresh enter event. Retained
  arguments, points and properties remain usable after the window closes.
- Input timestamps now retain each thread's last retrieved Win32 message time,
  including a non-removing peek. They are no longer replaced with the time when
  `GetMessageTime` happens to be called; pointer timestamps extend the 32-bit
  message time into the boot-relative 64-bit clock.
- Limits: touch/pen, cursor objects, key-state queries, flow direction, custom
  properties and automation are not implemented. Mouse sample history /
  `GetIntermediatePoints`, pointer-class static factories and device HID usage
  collections remain unsupported. Remaining CoreWindow vtable slots fail with
  `E_NOTIMPL` instead of calling null pointers. CoreWindow still does not render
  XAML. Cancellation cannot interrupt a callback already executing;
  `RunIdleAsync` remains to be implemented.
- `Windows.System.DispatcherQueue` now exposes the current CoreWindow thread's
  queue, with stable identity, thread-access queries and cross-thread priority
  posting on CoreDispatcher's existing pump. Threads without a CoreWindow return
  a null queue. `CreateTimer` supplies agile timers with Tick subscriptions,
  one-shot/repeating operation, interval changes, restart and stop. Defaults are
  zero interval and repeating; negative intervals fail. Native timers wake the
  pump, while Tick callbacks run below idle priority and never before the stored
  deadline. Long intervals are rearmed across Win32's maximum timer period;
  precision remains limited by the desktop timer and queue scheduling.
- Timer generations invalidate pending ticks after stop, restart, interval or
  repeat changes. Close and thread teardown cancel timers and pending work,
  notify ShutdownStarting/ShutdownCompleted, and release subscriptions even when
  a shutdown handler fails. This is immediate CoreWindow-owned shutdown:
  it finishes synchronously on the window thread, so `GetDeferral` there
  still returns `E_NOTIMPL` — deferrals are honored on the controller pump
  (see the deferred-shutdown entry). Standalone DispatcherQueueController
  creation and shutdown draining are supported.
- DispatcherQueue verification: `dispatcher_queue_fixture64` and
  `dispatcher_queue_fixture32`, with `ctest --test-dir build -R
  native_dispatcher_queue_ --output-on-failure`, pass all four normal/parallel
  x64/x86 cases. Coverage includes priority order, cross-thread posting/start,
  deadline and restart behavior, pending-tick cancellation, callback errors,
  shutdown failure cleanup, active-timer thread exit and `DllCanUnloadNow`.
  The combined CoreWindow/application/desktop/presentation/DispatcherQueue,
  native queue, window hook and paint regression selection passes 27 tests.
  Contracts follow Microsoft's public
  [DispatcherQueue lookup](https://learn.microsoft.com/en-us/uwp/api/windows.system.dispatcherqueue.getforcurrentthread),
  [timer](https://learn.microsoft.com/en-us/uwp/api/windows.system.dispatcherqueuetimer),
  [interval](https://learn.microsoft.com/en-us/uwp/api/windows.system.dispatcherqueuetimer.interval)
  and [repeat](https://learn.microsoft.com/en-us/uwp/api/windows.system.dispatcherqueuetimer.isrepeating)
  documentation.
- Public contracts: [event-processing options](https://learn.microsoft.com/en-us/uwp/api/windows.ui.core.coreprocesseventsoption),
  [`RunAsync`](https://learn.microsoft.com/en-us/uwp/api/windows.ui.core.coredispatcher.runasync),
  [`IAsyncInfo`](https://learn.microsoft.com/en-us/uwp/api/windows.foundation.iasyncinfo),
  [`MsgWaitForMultipleObjectsEx`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-msgwaitformultipleobjectsex),
  [`IFrameworkView`](https://learn.microsoft.com/en-us/uwp/api/windows.applicationmodel.core.iframeworkview).
  The launch-interface IID and layout missing from mingw come from Microsoft's
  [published Windows metadata projection](https://github.com/microsoft/windows-rs/blob/master/crates/libs/windows/src/Windows/ApplicationModel/Activation/mod.rs)
  (`ILaunchActivatedEventArgs_Vtbl`: inspectable base, Arguments, TileId).
  The native interop declaration follows the public SDK
  [`CoreWindow.h`](https://github.com/microsoft/win32metadata/blob/main/generation/WinSDK/RecompiledIdlHeaders/winrt/CoreWindow.h),
  including its `boolean` setter parameter; bounds follow the documented
  [client-area DIP contract](https://learn.microsoft.com/en-us/uwp/api/windows.ui.core.corewindow.bounds).
  Pointer snapshots follow the public
  [`PointerPoint`](https://learn.microsoft.com/en-us/uwp/api/windows.ui.input.pointerpoint),
  [timestamp](https://learn.microsoft.com/en-us/uwp/api/windows.ui.input.pointerpoint.timestamp),
  [raw-position](https://learn.microsoft.com/en-us/uwp/api/windows.ui.input.pointerpoint.rawposition)
  and [pressure](https://learn.microsoft.com/en-us/uwp/api/windows.ui.input.pointerpointproperties.pressure)
  contracts; message time follows
  [`GetMessageTime`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmessagetime).
- Verified on 2026-10-28: build `raven`, `core_window_fixture64` and
  `core_window_fixture32`, then run
  `ctest --test-dir build -R native_core_window_ --output-on-failure`.
  All four x64/x86 normal/parallel cases pass. Each covers 256 completion
  races, desktop-message and timer wakeups, callback-close and `WM_QUIT`
  shutdown, and surviving COM references after return/`ExitThread`. The
  lifetime check requires `DllCanUnloadNow` to return `S_OK` after the last
  external window/dispatcher reference is released.
- Application verification: build `core_application_fixture64` and
  `core_application_fixture32`; run
  `ctest --test-dir build -R 'native_core_(application|window)_' --output-on-failure`.
  All eight x64/x86 normal/parallel cases pass. The application fixture checks
  eleven lifecycle scenarios: success; CreateView/Initialize/SetWindow/Load/
  Activated/Run/Uninitialize failures; cross-thread exit; explicit `ExitThread`;
  and a view source returning success with no view. It also checks canonical
  COM identity, activation timing, current-view isolation, registration removal
  and addition during callbacks, and `DllCanUnloadNow` after releasing retained
  view/window/dispatcher/argument references.
- Desktop verification: build `core_window_desktop_test`; run
  `ctest --test-dir build -R 'native_core_(application|window|desktop|presentation)_' --output-on-failure`.
  Sixteen x64/x86 normal/parallel cases cover the above plus HWND identity,
  cross-thread rejection, native resize, disabled input, character decoding,
  callback revocation, keyboard/close error propagation and retained event args.
  Mouse checks include negative client coordinates, five button transitions,
  vertical/horizontal wheel data, mouse-device identity, pressure/contact,
  enter/exit, capture loss/ownership, pointer-handler errors, and retained
  point/property lifetimes after close. A delayed queued event verifies that its
  timestamp survives dispatch latency; a second thread checks message-clock
  isolation. The targeted regression run also includes native queue, hook and
  paint cases: 23 checks pass in total.
  The presentation fixture activates the window, draws through the interop HWND,
  verifies green client pixels in Raven's native presentation callback, injects
  a host close, and requires exactly one host-surface removal. This checks the
  desktop/WinRT connection rather than XAML rendering. A real compositor run
  remains unverified here because `labwc` is not installed.
  Native tests require ptrace/process permissions outside the execution sandbox.

### Stage 2 — composition

- `Windows.UI.Composition`: Compositor, Visual/ContainerVisual/SpriteVisual,
  brushes, CompositionTarget, simple animations. Backed by a Raven window
  surface; GPU compositing where available.
- Acceptance: a composition client draws coloured visuals into a Raven window
  and receives size changes.
- State: the object model and the CPU presentation are in place (2026-10-28).
  `xaml_composition.c` implements `Windows.UI.Composition.Compositor` (a
  registered runtime class) with the ABI mingw's `windows.ui.composition.h`
  declares: `CreateColorBrush`, `CreateColorBrushWithColor`,
  `CreateContainerVisual`, `CreateSpriteVisual` and
  `CreateTargetForCurrentView` are real; the animation, effect, surface,
  clip and batch factories answer `E_NOTIMPL`, and every stub keeps its slot's
  exact signature. Every object answers `ICompositionObject`
  (`get_Compositor` real). A `ContainerVisual` holds a `VisualCollection`
  (`IVisualCollection`: `Count`, `InsertAbove`, `InsertAtBottom`,
  `InsertAtTop`, `InsertBelow`, `Remove`, `RemoveAll`; re-adding a visual
  moves it, as a real compositor reparents). A `SpriteVisual` carries a
  `CompositionColorBrush`. `CompositionTarget.CreateTargetForCurrentView`
  binds the calling thread's CoreWindow, the way the UWP XAML framework
  attaches its own tree; `Root` accepts any visual.
- Presentation is the CPU fallback: the target's tree draws through GDI into
  the CoreWindow's desktop surface — the background cleared, then the tree
  walked bottom to top, sprite brushes filling their offset/size rectangles in
  pixels scaled from the window's DPI. Every mutation and every window size
  change presents synchronously, so a caller always sees the tree it just
  described. Damage tracking and the GPU path replace this later; alpha
  blending does not apply yet (a brush paints opaque, zero opacity paints
  nothing), and visual animations are not wired.
- The element tree renders through this stage (2026-10-28):
  `xaml_render.c` projects a hosted tree onto the composition visuals —
  `Windows.UI.Xaml.Window.put_Content` registers the hosted element with the
  window's composition target, every grid becomes a background sprite sized to
  its area (the window's client for the root, its `Width`/`Height` for a
  child), children render above it in document order, and a frame renders the
  page it navigated to through the page's content. The pass rebuilds the flat
  sprite list on every change — hosting, `Panel.Background`, the compiled
  `Background` colour, `Width`/`Height`, the child list and navigation — and
  the composition present makes it visible; window resizes re-present through
  the existing size hook. Text and control chrome do not render yet, a grid
  without a colour fills nothing, and measure/arrange remain the layout
  stage's work.
- The loader applies the element properties this needs: the stable properties
  `Panel.Background` (647), `FrameworkElement.Width` (409) and
  `FrameworkElement.Height` (393); a `Background` constant is one of the XAML
  colour names until the media brushes arrive, and `Panel.Background` with a
  real brush reads the colour from the composition colour brush the app
  passes.
- Grid layout (2026-10-28): `RowDefinition` and `ColumnDefinition` are runtime
  classes (catalog-registered, activated through the elements factory) with
  the `IRowDefinition`/`IColumnDefinition` ABI generated from the winmd — the
  `GridLength` Height/Width slots are hand-adjusted because the 16-byte struct
  is passed by value, which the generator's word-slot rule cannot express on
  x86. `IGrid.get_RowDefinitions`/`get_ColumnDefinitions` return collections
  built on the generalised children vector, with pinterface GUIDs derived by
  the validated signature rule (`IVector`1<RowDefinition>` =
  `2a17524a-48ef-579b-9e63-e1ce52adcae2`, `IVector`1<ColumnDefinition>` =
  `caf7c66c-1ccb-5ada-afce-fc62d2db2c66`). The loader applies the attached
  properties `Grid.Row` (860) and `Grid.Column` (857) onto children. The
  render pass lays children into cells with the measure stage behind it:
  fixed Pixel tracks take their length, Auto tracks take the largest content
  that wants them (measured recursively — a nested grid resolves its own
  tracks, a text block measures its run through the host font service, a
  content control measures its child; explicit Width/Height wins), and Star
  tracks share what remains after fixed and auto, proportionally to their
  values (equal shares when no star carries a weight). Children without a
  row/column go to cell (0,0), slots beyond the definitions are zero-size
  cells after the last track, an axis without definitions gives every child
  the grid's extent. Measuring uses the same
  DPI scale the paint does, so the layout and the pixels agree. `native_layout_x64`/`_x32` verify the cells
  by pixel and exercise the definition classes through the public API,
  including a `put_Width` re-render, a star round (columns 1*, 100, 1* share
  the window) and an auto round whose zero-measured first column collapses
  the layout. `native_text_x64`/`_x32` host a page whose Auto column takes
  the measured run of the text block it holds, so the following column starts
  at the text extent (bounded pixel assertions — the face the font service
  picks decides the exact width).
- Grid spans (2026-10-28, stage 3): the attached `Grid.RowSpan` (862) and
  `Grid.ColumnSpan` (859) merge the child's own track with the ones after it
  into one cell running from its track's origin to the end of the last
  spanned track. The spanning child fills the merged region without growing
  any Auto track — the WPF distribution of a spanning want across the spanned
  Auto tracks is not implemented — so spanning wants are skipped when the
  Auto tracks take their content, and the child fills whatever the merged
  region resolved to. A span reaching past the last track clamps to the
  tracks that exist, a span below one is one track, and a slot beyond the
  definitions stays the zero-size cell after the end whatever the span says.
  Slot storage moved to one field-selector path (`store_slot`) shared by the
  row, column and span attached properties. The span round in
  `native_layout_x64`/`_x32` hosts a grid whose children cover the
  no-slot (cell (0,0)), column-span-only (one row tall) and two-by-two-span
  cases, verified by pixel.
- TextBlock text rendering (2026-10-28, stage 4): a hosted TextBlock paints
  its text in the CPU-fallback presentation. `ITextBlock` FontSize/Foreground
  are real (the compiled `Foreground` colour-name path reuses the Background
  parser; a composition colour brush works through the same slot), the render
  pass hands the sprite a text payload (string, DIP font size, colour), and
  the composition paint draws the glyphs through the host font service
  (FreeType-backed, resolved from ravenhost.dll) composited onto the window
  DC. The host GDI text providers only exist in Skia configurations — and an
  API the surface does not serve stops the guest — so the fallback does not
  use `TextOutW`. Segoe UI, else Liberation Sans, else DejaVu Sans; 1 DIP = 1
  pixel at 96 DPI, run top at the sprite origin. No measuring (the block does
  not yet influence its own size), no shaping, no per-glyph clipping beyond
  the sprite bound; the stage-4 DirectWrite component path remains the target.
  `native_text_x64`/`_x32` verify glyph pixels over the background, the
  textless area staying background, size/colour round-trips, re-renders on
  `put_Text`/`put_Foreground`, and the empty-text case.
- `CoreDispatcher.RunIdleAsync` (2026-10-28, the last noted stage-1 gap): the
  idle callback queues as an ordinary idle-priority item, so ordering,
  cancellation, completion and failure handling are exactly `RunAsync`'s, and
  a self-re-queueing callback keeps the pump busy like any re-queueing
  handler. `IdleDispatchedHandlerArgs.IsDispatcherIdle` reports, live under
  the queue lock, whether any work above idle priority has been posted since
  the idle item was queued (the queue counts such posts; idle-priority
  siblings and below-idle timer tasks do not count) — so normal work arriving
  while the item waits, or while it runs, turns it false.
  `native_dispatcher_idle_x64`/`_x32` cover ordering against normal and
  idle-priority work, cancellation, handler failure, the pump waiting for
  cross-thread idle work instead of spinning, bounded self-re-queueing, and
  close completing pending and new idle actions without invoking callbacks.
- Styles (2026-10-28, stage 4): `Windows.UI.Xaml.Style` is a runtime class
  with real `TargetType` (the TypeName interop struct), `IsSealed`/`Seal`, a
  real `Setters` collection (`IVector`1<SetterBase>`, GUID derived by the
  validated signature rule and confirmed in the real Windows.UI.Xaml.dll), and
  Setter objects carrying property-name/value pairs (`ISetter.Value` real;
  `Property` waits for the DependencyProperty stage). The element classes
  store an owned Style (engine-internal `put_style`/`get_style`), and the
  render readback applies the setters OVER the element's own state — a
  "Background" string setter repaints a hosted grid, `put_Style(NULL)`
  restores the unset state exactly. Sealed styles reject mutation.
  `native_style_x64`/`_x32` verify TargetType/seal round-trips, the styled
  render, the dictionary round-trip, and the unstyled element keeping its own
  background. The increment also fixed a pre-existing refcount bug: re-hosting
  a window over-released the replaced element.
- Automation peers (2026-10-28, stage 4): `FrameworkElementAutomationPeer`
  answers `IAutomationPeer` (GetName real through the engine's name readback —
  element Name, a text block's Text as fallback, the runtime class name last;
  IsContentElement/IsControlElement TRUE; GetPattern E_NOTIMPL for every
  pattern; the rest of the 39 slots at exact signatures) and
  `IFrameworkElementAutomationPeer` (get_Owner). The statics' CreatePeerForElement
  and FromElement give one peer per element per process (a registry keyed by
  the element pointer, holding its own reference; the creation's refcount
  lifecycle was fixed — the peer started at zero and died on the first
  release). The button's peer serves the Invoke pattern (2026-10-31):
  `GetPattern` hands an `IInvokeProvider` (the interface generated from the
  winmd into `windows_ui_xaml_automation_provider.h`) whose `Invoke` raises
  the button's `Click` through the button's own raise, independent of the
  click mode the way the real `ButtonAutomationPeer.Invoke` calls `OnClick`,
  synchronously inside the call; peers of other element kinds keep the empty
  pattern answer, and the x:Name loader path stores names on buttons so the
  name readback answers. `native_automation_x64`/`_x32` verify peer identity,
  Owner, the name precedence, GetPattern (the text peers' empty set, the
  button's Invoke and nothing else), the Invoke-raises-Click round twice,
  content/control flags, the composable factory, and the ownerless peer
  answering no name.
- Pointer input routing (2026-10-28, stage 4's routing begun): the element
  classes answer `IUIElement` (ABI generated from the winmd — the seven
  pointer event pairs with token revocation are real, plus
  `IsHitTestVisible`; the rest of the 96 slots answer `E_NOTIMPL` at their
  generated signatures) and Button answers `IButtonBase` (`Click` storage,
  `ClickMode`, `IsPressed`). The render pass records every element's DIP rect
  in paint order; the routing (`xaml_input.c`) subscribes to the hosting
  CoreWindow's pointer events, hit-tests topmost-first over the recorded
  geometry, tracks hover as a per-element set (entering a button also enters
  the grid behind it), delivers release to the pressed element regardless of
  position, and raises `Click` on an in-bounds release under
  `ClickMode.Release`. A failing handler never stops delivery, revocation
  works mid-flight, and teardown follows re-hosting and window close.
  `native_xaml_input_x64`/`_x32` inject desktop mouse messages on the interop
  HWND and assert enter/exit, press transitions, exactly one Click on-button
  and none off-button, failure isolation and revocation.
- Routed-event bubbling (2026-10-31, stage 4): after the target's own
  delivery, `PointerPressed`, `PointerReleased` and `PointerWheelChanged`
  bubble — each ancestor's instance handlers run immediate-parent-first up
  to the root, while the element behavior stays target-only, so a button
  under an ancestor's pressed handler still clicks exactly once. The press,
  wheel and fresh-target releases bubble through the recorded containment
  chain (reverse paint order — the flat geometry standing in for ancestry),
  a release of a pressed element that still hovers bubbles through its
  hover-set chain, and one that no longer hovers is delivered alone. An
  application handler cannot stop the pass: the routed arguments are the
  CoreWindow's snapshot and carry no `Handled`. Entered, exited, moved and
  capture-lost keep their single-element delivery. The input fixture's
  order log asserts child-parent-grandparent per event kind, the
  click-once round, the not-hovered release and the non-bubbling hover
  events.
- CoreCursor (2026-10-31, stage 4): `Windows.UI.Core.CoreCursor` is a
  runtime class of the component — activation answers the default arrow,
  and the factory's `CreateCursor` builds any built-in type (an
  out-of-range type `E_INVALIDARG`, `Custom` `E_NOTIMPL`: a custom cursor
  loads a bitmap the emulated desktop has no source for; mingw's
  `ICoreCursor` surfaces `get_Id`/`get_Type`, and the 32x32 standard size
  of the built-ins is documented where the class is defined). The catalog
  routes the class name to the component. The CoreWindow's
  `get_PointerCursor`/`put_PointerCursor` are real at exact signatures: the
  put stores the application's object (AddRef, swapped under the window
  lock), a NULL put restores the platform default, and both answer
  `RO_E_CLOSED` after close. The pointer snapshot's `KeyModifiers` and the
  `IPointerPointProperties` set were verified already honest from the
  sample; pointer history landed the same day (see the pointer-history
  entry below).
  `native_core_desktop_x64`/`_x32` (and the parallel variants) drive the
  cursor round through `RoGetActivationFactory`: activation types, factory
  construction and rejection, the window default, put/identity/NULL-put
  restore.
- Styled setters on the tree (2026-10-31, stage 4): `put_Style` applies on
  the element classes with real settable state — each setter routes through
  the same store the loader and the direct property puts use, so the pixel
  follows a styled value the way it follows a `put_Background`, and the
  re-render comes from the same hook; the render-readback override that
  re-applied setters at every paint is gone. `BasedOn` walks the chain
  (base setters first, the style's own after, derived wins; a depth cap of
  32 guards cycles), and the whole chain validates in a check-only pass
  before anything mutates, so a wrong-kind value fails the put with
  `E_INVALIDARG` and leaves the element untouched; a property the engine
  has no storage for is skipped, since without DependencyProperty it cannot
  tell a name it does not store from one it will. Replacement re-applies
  and does not revert — a property the old style set and the new one skips
  stays where the old one left it (documented). Templates,
  DependencyProperty, TargetType checking and unmerge stay unimplemented.
  `native_style_x64`/`_x32` drive the styled grid and text block through
  pixels and re-measured runs, the BasedOn precedence round, the
  wrong-kind failure rounds and the `get_Style` round-trip.
- Pointer history (2026-10-31, stage 4): every desktop window retains a
  small ring (8 entries, overwritten oldest-first, no allocation) of the
  mouse samples it built, pushed before the event fires, and each
  `PointerEventArgs` snapshot freezes its own history vector at
  construction — the entries strictly older than the current point's
  frame, oldest first — so a retained args object answers the history it
  had at press time, not the ring's later contents.
  `GetIntermediatePoints` AddRefs that snapshot (an empty history is a
  valid empty vector, never `NULL`; the `E_NOTIMPL` shortcut is gone).
  The vector itself is a real `IVector<PointerPoint>` (the mingw header
  precomputes the instantiation and its IID) with honest mutation slots,
  `E_BOUNDS` out of range, pointer-identity `IndexOf` and an owning
  `GetView`; the exported engine-internal `raven_pointer_history_create`
  builds one from raw samples the same way the event path does.
  `native_core_desktop_x64`/`_x32` check the press-time ordering
  (strictly decreasing frames below the current point), `E_BOUNDS`, the
  snapshot surviving hundreds of later samples overwriting the ring, and
  the builder round with full refcount balance.
- Authored default styles (2026-10-31, stage 4): the engine authors one
  sealed `Style` per supported control class from the same
  `raven_style_add_setter` path applications use — the text block's
  `FontSize` 14 / `Foreground` black (exactly the values its constructor
  hardcodes) and the button's `Background` transparent — and attaches it
  at activation, so a control that never sees an explicit `put_Style`
  still carries the documented defaults and answers `get_Style`. The
  defaults are shared per class and sealed at authoring (every mutation
  slot of a sealed style already rejects), so an application holding the
  object cannot mutate what every instance points at; an explicit
  `put_Style` replaces the default the usual way (re-applies, no revert),
  and `put_Style(NULL)` clears. Without DependencyProperty the authored
  set stays small: properties the applier has no storage for cannot be
  authored honestly, and templates remain unimplemented.
  `native_style_x64`/`_x32` check the constructor pickup, the setter
  pairs read back through `raven_style_setter_count`/`_at`, the defaults
  rendering, attach re-putting the same engine object, the explicit
  override and the clear.
- DependencyProperty (2026-10-31, stage 4): `Windows.UI.Xaml.DependencyProperty`
  is a runtime class of the component — a statics-only class like the real
  one, activation faults `ActivateInstance` and the factory answers
  `IDependencyPropertyStatics` (`get_UnsetValue` hands out one
  identity-stable per-process sentinel; `Register` with a hand-rolled
  `IPropertyMetadata` default creates the property, a duplicate
  (owner, name) `E_INVALIDARG`, the same name under another owner
  registers, `RegisterAttached` `E_NOTIMPL` since no attached-property
  store exists). The real `IDependencyProperty` ABI is exactly one member,
  `GetMetadata` (the projection's Name/TypeName/OwnerType getters are not
  in the surface), so the metadata reads go through the engine-internal
  interop part instead of pretending. A Setter now carries a
  DependencyProperty (`put_Property`/`get_Property`, engine DPs only) and
  at application time the DP's registered name flows into the very same
  (name-string, value) pair store `put_Style` walks — so a DP setter
  renders exactly like a name setter and the applier is untouched; the
  no-revert and unknown-name conventions carry over. The catalog routes
  the class name to the component.
  `native_style_x64`/`_x32` drive registration, the metadata round-trips,
  the duplicate/attached contracts, UnsetValue stability, the Setter
  property round-trip, and the rendered pixel through a DP setter.
- PointerDevice statics (2026-10-31, stage 4): `Windows.Devices.Input.
  PointerDevice` answers `IPointerDeviceStatics` through the activation
  chain (the factory part faults `ActivateInstance` — the real class has
  no default activation): `GetPointerDevices` returns an owning
  `IVectorView<PointerDevice>` snapshot over a once-built catalog from
  the host's own enumeration (`GetRawInputDeviceList` for mice, user32
  `GetPointerDevices` with a raw-input HID digitizer fallback for
  touch/pen), and the engine mouse is appended even when the host lists
  nothing, because the engine synthesizes mouse input and
  `GetPointerDevice(1)` must always resolve. `GetPointerDevice` answers
  only id 1 — the id every event reports — and `E_INVALIDARG` for an
  unknown id. The instance object grew its honest per-device state (a
  mouse is never integrated, `MaxContacts` 1, the desktop rects), and
  `SupportedUsages` stays `E_NOTIMPL`: the emulated mouse is a Win32
  cursor device with no HID report descriptor, so there is no usage
  collection to answer.
  `native_core_desktop_x64`/`_x32` check the catalog through both the
  engine-internal export and `RoGetActivationFactory`, the mouse's COM
  identity against the event-verified device, the documented values,
  view bounds/GetMany/IndexOf and a full refcount ledger.
- Touch delivery (2026-10-31, stage 4): the desktop window decodes
  `WM_POINTER` messages alongside the mouse — pointer id from the
  message, flags from the high word, position to DIPs, samples pushed to
  the same history ring before the event fires — so touch contacts reach
  Pressed/Released/Moved/Entered/Exited as distinct pointers with stable
  ids across a contact (a documented 8-contact bound; the first is
  primary), touch presses the left button the way the real statics do,
  and the PointerPoint hands out the matching device object from the
  statics catalog. The host probe settled what is deliverable: no
  digitizer in the enumeration, `SendInput` injects nothing,
  `GetPointerInfo` is an unimplemented import, and the host pointer
  table is empty — so synthetic `WM_POINTER` posts (which arrive
  verbatim) are the real test seam, ids other than the engine mouse's
  decode as touch by documented convention, and pen stays unreachable
  until a host populates the table (the comment at the decoder states
  the mapping a real host must add). `WM_TOUCH` is not decoded: nothing
  here can produce a decodable one. The catalog appends a documented
  touch stand-in when the host lists no digitizer, whose unknown
  digitizer facts answer `E_NOTIMPL` rather than invented values.
  `native_core_desktop_x64`/`_x32` drive a hover→contact→press round on
  two simultaneous contacts (distinct ids, only one primary), the touch
  history round-tripping through `GetIntermediatePoints` with the touch
  device's identity, the stand-in's facts, and a closing mouse move
  still answering id 1/Mouse.
- CoreApplication identity and properties (2026-10-31, stage 5):
  `CoreApplication.Id` answers the manifest's `Applications/Application@Id`
  (the PRAID — `EntryPoint` is a class name, not an id) through the same
  hand-rolled scanner `package.c` already runs, and keeps forwarding the
  host's no-package error without identity; the scanner's capture is
  covered through an exported seam that parses in-memory manifests with
  the disk path's own parser, since the engine reads identity once per
  process and the fixture cannot run both branches in one.
  `CoreApplication.Properties` is a process-wide property set with real
  `IMap<HSTRING, IInspectable>` semantics — content-compared string keys
  (a NULL HSTRING is the empty key), Insert reports replacement and owns
  values, `E_BOUNDS` on missing Lookup/Remove, HasKey/Size/Clear and a
  live `GetView` — handed out as the same object per get and never
  freed. The singleton now claims the `PropertySet` runtime class; see the
  PropertySet entry below. `RunWithActivationFactories` validates its
  callback and refuses the packaged run (see the Application limits
  paragraph); suspend/resume stays `E_NOTIMPL` with the missing pieces
  named in comments.
  `native_core_application_x64`/`_x32` and `native_package_x64`/`_x32`
  cover the no-package `Id` error, the scanner seam, and the full
  property-set round.
- PropertySet runtime class (2026-10-31, stage 5): the process property set
  was refactored from a static singleton into per-instance objects — each
  carries its own `CRITICAL_SECTION`, entry array and reference count, and
  releasing the last reference disposes the entries, deletes the lock and
  balances `raven_xaml_objects` for tracked instances. With real instances
  the engine now claims the class: `Windows.Foundation.Collections.PropertySet`
  is in the activation catalog, the factory's `ActivateInstance` builds a
  tracked instance (the class is activatable, it has no statics), and every
  part — IPropertySet, IMap, IMapView, factory — answers
  `GetRuntimeClassName` with `Windows.Foundation.Collections.PropertySet`
  instead of the parameterized IMap name. `CoreApplication.Properties`
  remains a process-wide instance created once at attach that holds the
  engine's permanent reference, so `get_Properties` still answers the same
  pointer every call. The fixtures prove two activated instances are
  distinct COM identities with isolated maps, and that their refcounts
  balance to zero while the singleton survives.
- Control templates (2026-10-31, stage 4): `Windows.UI.Xaml.Controls.
  ControlTemplate` is a runtime class with the real `IControlTemplate` ABI
  (TargetType get/put — the real interface carries no content member; the
  engine-internal root-attach export fills that role), activatable through
  the catalog. `Control.Template` exists on the button over the real
  41-member `IControl` vtable (IIDs sourced from the WDK 8.1 IDL, since no
  mingw/generated header carries them): `put_Template` accepts only engine
  templates (`E_INVALIDARG` otherwise), re-put replaces, NULL clears, and
  the Style setter path routes the `"Template"` pair through the same
  applier with the check-only validation pass kept — a wrong-kind template
  fails the whole `put_Style` atomically. While a template without a
  ContentPresenter is applied the template root **is** the rendered visual
  tree and the stored Content is not rendered (`get_Content` still answers
  the logical content); with a ContentPresenter in the tree, the templated
  control's logical Content shows inside the template (see the
  ContentPresenter entry below). No render-path change was needed: the
  existing `raven_xaml_frame_render_child` hook answers the template root
  before the content branch, so both measure and paint pick the template
  tree up through the same call. `native_style_x64`/`_x32` (template applied
  through a Style, pixel-proved apply/replace/clear) and
  `native_xaml_controls_x64`/`_x32` (activation, TargetType round-trip,
  real-vtable assignment, precedence, clear/re-put) cover the increment.
- XBF-declared templates (2026-10-31, stage 3): the loader's node walker
  already hands nested element values to properties (that is how
  `Grid.Children` and `Content` ride), so templates needed only a small
  hook: the loader recognizes engine ControlTemplate objects, opens a
  template scope that defers the pending property application until the
  scope pops (a template is created before its root element, but the
  control must store it only after the root is attached — a failed
  application fails the document, an unwind never half-applies), and the
  next object created under that scope becomes its root via the
  engine-internal `put_root`. `create_known` resolves the document-named
  `ControlTemplate` and `ContentPresenter` classes through the engine
  factories, since the stable type table does not yet carry them — real
  app XBFs referencing them by stable index still fail until the tables
  grow the entries. The wiring fixed a real bug: the element set-object
  path short-circuited `S_FALSE` for any name other than Children/Content,
  so "Template" never reached the elements dispatch. Covered in
  `native_xaml_controls_x64`/`_x32` (hand-built XBF: Button with content,
  a template root grid holding a presenter, template identity, presenter
  answering the button's content).
- ContentPresenter (2026-10-31, stage 4): a deliberately narrow element —
  class name and content resolution only, no `IContentPresenter` surface,
  no framework-element properties, not in the runtime catalog (activation
  is engine-internal). It binds a non-owning back-pointer to its templated
  parent when a button stores a template; the old root is unbound first,
  so replace/clear can never leave a dangling binding, and a retained
  presenter after a clear answers no content. At measure/render it
  resolves the templated parent's Content through the same
  `raven_xaml_frame_render_child` hook chain. Pixel-proved in
  `native_style_x64`/`_x32`: a red template root with a presenter shows
  the button's green content inside it, and clearing falls back to
  rendering the content directly.
- Stable-index templates (2026-10-31, stage 3): the metadata tables carry
  `Control.Template` (property 560), `ControlTemplate` (type 191) and
  `ContentPresenter` (type 296), indices verified against the numbering
  convention of `scripts/xbf-stable-indices.json` (array index == stable
  index, cross-checked against every already-registered entry). The
  loader's stable branch gained a fallback: when `RoActivateInstance`
  fails for a stable type, the class name resolves through
  `raven_xaml_class_by_stable` and the engine-internal factory path —
  load-bearing for ContentPresenter, which has no catalog entry;
  successful activations keep the normal path. A real-shaped XBF whose
  metadata section is empty — every node resolved by stable index
  (Button 540 → Content 999 → TextBlock 396 → Template 560 →
  ControlTemplate 191 → Grid 442 → Children 648 → ContentPresenter 296) —
  now loads and applies its template: `native_xaml_application_x64`/`_x32`
  verify the applied template identity, the root grid, the presenter
  resolving the button's content, and the logical `get_Content`.
  Template-adjacent members beyond these three (e.g.
  `ContentPresenter.ContentTemplate` 525) remain unresolved by stable
  index.
- DispatcherQueue deferred shutdown (2026-10-31, stage 1): a
  `ShutdownStarting` handler on the controller pump can now
  `GetDeferral` — a real `IDeferral` (mingw's precomputed
  `IID___x_ABI_...CIDeferral`, no guessed GUID). While a deferral is
  outstanding the close work — queued below every client priority —
  records it and returns without closing; when the last deferral's
  `Complete` runs (idempotent), the close is re-posted at idle-minus-one,
  so work posted during the deferral window still drains before
  `ShutdownCompleted`. The deferral owns a queue reference, so a deferral
  held past the drain cannot free the queue, and a cross-thread
  `Complete` after the queue already closed is a safe no-op. On the
  immediate CoreWindow-owned close `GetDeferral` stays `E_NOTIMPL`: that
  close finishes synchronously on the window thread and cannot wait —
  fixture-covered refusal. `native_dispatcher_controller_x64`/`_x32` run
  two deterministic rounds (Complete-from-final-work-item with a
  double-Complete idempotency check; handler failing `E_ACCESSDENIED`
  with a second OS thread completing cross-thread on signal), and
  `native_dispatcher_queue_x64`/`_x32` cover the immediate-path refusal.
- RunWithActivationFactories (2026-10-31, stage 5): the entry validates
  its callback — `E_POINTER` on NULL (matching `Run`'s NULL source), and
  `E_ILLEGAL_METHOD_CALL` while `Run` owns the UI thread, via a
  non-mutating atomic read so a refused call never mutates startup state
  or retains the callback. The packaged/COM-launch run itself stays
  `E_NOTIMPL`: the real contract serves out-of-proc activation through
  the caller's `IGetActivationFactory` while a packaged activation host
  delivers the launch — nothing of that chain exists on the emulated
  desktop, and the only class name the engine could honestly ask for
  (the manifest `EntryPoint`) does not exist in a manifest-less process.
  `native_core_application_x64`/`_x32` carry a client-side
  `IGetActivationFactory` (mingw's published IID) probing NULL, idle
  refusal, refcount-neutrality, and the concurrent-during-`Run` error.
- Presenter generalization and ContentTemplate (2026-10-31, stage 4): the
  presenter bind no longer lives in button code — `element_store_template`
  is element-agnostic in both owner and storage slot (the replaced root's
  presenters unbind first, then the swap, then the new root binds), and
  every entry point (direct `put_Template`, the style applier, the
  loader's deferred template-scope pop) rides it; the button remains the
  only engine element with a slot today, and a future kind needs only a
  dispatch case. `ContentPresenter.ContentTemplate` (stable 525, verified
  by the established index convention) is stored on the presenter through
  the same routing and now applies — see the nested-template entry below.
  `native_xaml_application_x64`/`_x32`
  load a stable-index XBF declaring a template under property 525 and
  verify the stored template's identity and root, and
  `native_style_x64`/`_x32` re-cover the generalized bind pixel-exactly.
- Nested-template application (2026-10-31, stage 4): a stored content
  template now applies, riding the existing machinery with no second hook:
  the presenter's `template_child` hook — the same one the button's
  template root answers from — resolves the templated parent's Content
  through the existing `IContentControl` path and, when a content template
  is stored, answers the content template's root in both measure and
  paint. The content template's own presenters bind lazily at resolution
  time with the **resolved Content object** as the walk's owner (not the
  templated control) — the same walk, the same non-owning back-pointer,
  idempotent and self-correcting when Content changes. Content NULL
  renders nothing; content that is not a ContentControl or is a
  non-element value stops the walk and inner presenters answer nothing
  rather than something invented. Unbind-on-replace/clear is cascaded: a
  replaced or cleared template leaves no bound back-pointer behind,
  including the stored content template's inner presenters, and the store
  raises `raven_render_changed()`. Pixel-proved in
  `native_style_x64`/`_x32` (green-over-blue direct content becomes
  green-over-red flowing through the content template's tree; after
  `put_Template(NULL)` both presenters answer nothing and content renders
  directly) and covered in `native_xaml_application_x64`/`_x32` (an inner
  presenter resolves the content button's Content, proving binding to the
  content object).
- Stable-index coverage (2026-10-31, stage 3): a full inventory of what
  the loader/appliers resolve end-to-end registered three more properties
  — `ContentControl.ContentTemplate` (833), `TextElement.FontSize` (251),
  `TextElement.Foreground` (255) — all verified against the established
  index convention and exercised through fully stable-index XBFs
  (`native_xaml_controls_x64`/`_x32` codes 4000–4116): a deferred
  content-template application with a TextBlock root, and a stable-only
  text document proving `get_FontSize` reads back 21.5 against the 14
  default. All 33 pre-existing registrations were re-verified against
  `scripts/xbf-stable-indices.json`. Honest refusals (no end-to-end
  resolution): presenter Content/FontSize/Foreground (no such surfaces),
  Border/StackPanel/Canvas types (no factories), HorizontalAlignment/
  Margin/VerticalAlignment/Style (no appliers), enum types, and
  RowDefinition.Height/ColumnDefinition.Width (the vtables exist but the
  loader's float dispatch does not route Width/Height to definitions —
  the next candidate unlock).
- SwapChainPanel (2026-10-31, stage 6): the parked design question is
  resolved as **contract-only**, on probe evidence: D3D10 creation is
  confirmed unsupported by the existing fixtures, D3D9 objects work but
  do not present, and guest Vulkan submits and reads back with installed
  drivers (`lvp_icd.json` is the only thing missing in the harness) — but
  the desktop's GPU child surfaces are HWND-backed and offer no bridge
  importing a supplied composition swapchain into the XAML tree, so GPU
  capability alone does not establish panel interop. `Windows.UI.Xaml.
  Controls.SwapChainPanel` (stable type 536) is a runtime class in the
  catalog with the real ABI: `ISwapChainPanel` (no SwapChain or
  CompositeMode property — attachment belongs to
  `ISwapChainPanelNative::SetSwapChain`, verified against the SDK
  headers; the WinRT IID `c589644f-…` and native IID `f92f19d2-…` were
  re-verified against independent sources before integration) answering
  CompositionScaleX/Y and the CompositionScaleChanged event pair,
  `CreateCoreIndependentInputSource` `E_NOTIMPL` (no input routing into
  a GPU surface), and the native part storing an internal swapchain
  reference — SetSwapChain with a non-NULL chain returns `E_NOTIMPL`
  without retaining the refused chain, NULL detaches, thread checks and
  COM identity/lifetime are real, and no pixel is ever read back or
  invented. The panel deliberately exposes no IUIElement/IPanel until
  layout can reject GPU content, and `xaml_composition.c` documents why
  GPU surface brushes stay unsupported.
  `native_composition_x64`/`_x32` cover identity, ownership, replacement,
  destruction, threading and refusal (codes 150–220) and the
  surface-brush refusal (271–275).
- Package identity (2026-10-28, stage 5 begun): `Windows.ApplicationModel.Package`
  is implemented in `package.c` from the app's `AppxManifest.xml` — the
  package root resolves from the executable's directory (`<exe>.AppxManifest`
  or `<exe-name>/AppxManifest.xml`), a tolerant hand-rolled XML scanner reads
  Identity and Properties, `IPackageId` reports Name/Publisher/Version/
  Architecture and the documented PublisherId (SHA-256 over the UTF-16
  publisher, Crockford base32 — verified against Microsoft's known
  `8wekyb3d8bbwe`), FullName/FamilyName follow the documented construction,
  and `IPackageStatus` reports OK. A process without a manifest gets an empty
  identity rather than an error, the documented identity-less answer.
  `native_package_x64`/`_x32` verify the identity fields, singleton
  `get_Current`, and the publisher-id derivation.
- Resource dictionaries (2026-10-28, stage 3): `Windows.UI.Xaml.ResourceDictionary`
  is a runtime class with real `IMap<Object, Object>` (Insert replaces and
  reports, Lookup fails E_BOUNDS on a missing key, HasKey, Size, Clear, a live
  GetView) and `IResourceDictionary.MergedDictionaries` (a live
  `IVector<ResourceDictionary>`); string keys compare by content through the
  boxed `IReference<HSTRING>`, everything else by COM identity. The
  parameterized GUIDs (`IMap<Object,Object>` = `f5f69427-…`,
  `IVector<ResourceDictionary>` = `fe820a0c-…`) were validated against the
  real Windows.UI.Xaml.dll binary before use. `Application.Resources` stores
  and returns the dictionary. `native_resource_dictionary_x64`/`_x32` cover
  insert/lookup/replace/clear, MergedDictionaries, Application round-trip and
  unload. The PE resource fixtures keep their own client
  (`resource_pe_client.c`), which the dictionary fixture had overwritten.
- MRT Core (2026-10-28, stage 5): `Windows.ApplicationModel.Resources.Core.ResourceManager`
  reads strings from a real `resources.pri` beside the executable —
  `scripts/make_test_pri.py` generates one (verified round-trip through
  `pri-extract.py`), and the C reader in `mrt_manager.c` parses the header,
  section directory, descriptor, hierarchical schema, decisions and candidates
  fail-closed (a malformed PRI answers lookups with E_BOUNDS, never a crash).
  `get_Current` is a per-process singleton; `MainResourceMap.GetValue`
  returns a NamedResource whose resolved candidate carries the string;
  `GetValue(name, context)` accepts a ResourceContext (NULL = default;
  qualifier matching is not yet applied). `native_mrt_manager_x64`/`_x32`
  verify both strings, the resolution precedence between the two candidate
  layouts, missing-name behavior and `IsResourceReference`.
- Windows.Storage subset (2026-10-28, stage 5): `Windows.Storage.ApplicationData`
  and `Windows.Storage.PathIO` are runtime classes in `storage.c`.
  `ApplicationData.Current` is a per-process singleton whose `LocalFolder` is
  a `StorageFolder` bound to the `LocalState` directory beside the executable,
  created on demand through the emulated `CreateDirectoryW` on the first ask;
  the singleton owns the folder object and lives as long as the process, the
  way `Package.Current` does. `IStorageFolder` answers `CreateFileAsync`
  (option 1 replaces through `CREATE_ALWAYS`, option 0 fails through
  `CREATE_NEW` with `HRESULT_FROM_WIN32(ERROR_FILE_EXISTS)`, the other
  collision options E_NOTIMPL) and `GetFileAsync` (a missing name completes
  the operation with `HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)`, the answer
  Windows gives for a missing item). `IStorageItem` answers `Name`, `Path`
  and nothing else real on folders; on files `FileType` carries the extension
  with the dot and `DeleteAsync` deletes the file for both delete options
  (the emulated filesystem has no recycle bin). `PathIO` answers
  `WriteTextAsync`/`ReadTextAsync` over absolute paths with real
  `CreateFileW`/`WriteFile`/`ReadFile` I/O, UTF-8 bytes and a BOM the reader
  takes off; the line, append and buffer forms E_NOTIMPL at their exact
  signatures. The `IAsyncOperation`/`IAsyncAction` shapes are one macro
  family over an `async_core` (status, error, lock, handler): the work runs
  inside the call and the operation is already terminal when it escapes —
  there is no thread hop, the CPU-fallback style the dispatcher actions use —
  so a handler assigned after completion fires from `put_Completed`, and
  `GetResults` answers `E_ILLEGAL_METHOD_CALL` while Started. The
  parameterized GUID for `IAsyncOperation<StorageFile>`
  (`5e52f8ce-…`, signature `rc(Windows.Storage.StorageFile;{IStorageFile})`
  over the `IAsyncOperation`1` base `{9fc2b0bb-…}`) came from the
  parameterized-type rule and was validated against mingw's precomputed
  `IAsyncOperation<HSTRING>` before hardcoding. The
  `folder_item_get_name`/`file_item_get_name` class of bug — casting the
  second interface part (`IStorageItem`) to the object instead of going
  through `CONTAINING_RECORD` — reads 8 bytes past the owner and is the
  failure the fixtures caught first. `native_storage_x64`/`_x32` cover the
  singleton identity, the LocalState name and path (and that the directory
  exists on disk by the time the call returns), a created file verified with
  `GetFileAttributesW` from the client, the create-collision error, the
  lookup round trip and miss, the PathIO byte-for-byte round trip, a delete
  that really removes the file, a Completed handler per shape, and that the
  module unloads clean (`DllCanUnloadNow` S_OK) once the guest drops its
  last reference.
- DispatcherQueue controllers (2026-10-28):
  `Windows.System.DispatcherQueueController` is registered and implemented in
  `dispatcher_queue.c` — `CreateOnDedicatedThread` spawns a thread owning its
  CoreWindow and queue, `get_DispatcherQueue` reports it, and
  `ShutdownQueueAsync` returns an action that runs the documented shutdown on
  the dedicated thread (`ShutdownStarting`, drain, close driving the existing
  window/queue teardown and `ShutdownCompleted`), joins the thread, and
  completes; a failing `ShutdownStarting` handler never stalls the shutdown.
  Post-shutdown queues
  reject work the way closed queues do. `native_dispatcher_controller_x64`/
  `_x32` cover creation, cross-thread posting in priority order, the event
  ordering, the thread-exited-before-completed invariant, and
  `DllCanUnloadNow` after all references drop.
- Acceptance met: `native_composition_x64`/`_x32` activate the compositor,
  check the `ICompositionObject` identity, create the target for the current
  view, build a white background with red and blue squares above it, and
  verify the presented pixels through the same emulated GDI surface
  (`GetPixel`): background between the squares, each colour over its own
  rectangle, then offset, visibility and brush mutations re-present, and a
  native window resize re-presents into the enlarged surface (the client
  fills the window red first, so only a fresh presentation can restore the
  tree and clear the exposed area). `native_render_x64`/`_x32` host a compiled
  page — a blue grid with a red 100x100 square and an unrendered text block —
  through `Window.put_Content`, verify the presented pixels, re-render through
  the real `Panel.Background` brush slot, and re-present after a resize. The
  composition and render tests join the regression selection: 45 x64/x86 and
  desktop checks pass.

### Stage 3 — XAML core

- XBF reader (v2/v3) for the app's 304 pages; type system; dependency
  properties; resource dictionaries; styles and templates; measure/arrange;
  data binding; visual tree to composition.
- Acceptance: a fixture XAML app (Application, Page, Grid, TextBlock, Button)
  renders and responds to input.
- State: the Application ABI and activation are in place (see below); the engine
  behind the interface is not.

#### Application ABI and activation (started 2026-10-28)

`scripts/generate-xaml-header.py` reads a winmd and emits the C declarations an
interface needs: the IID from the `Guid` attribute, the vtable in metadata order
after the IInspectable methods, and one slot per method. The ABI slot count is
the declared parameter count plus one when the logical return type is not void
(property getters and factory-created objects return through an out parameter);
a repeated method name in one interface is suffixed in the C member name while
the order stays unchanged. It requires `dnfile`:

```sh
python3 -m pip install --target /tmp/winmd-lib dnfile
PYTHONPATH=/tmp/winmd-lib python3 scripts/generate-xaml-header.py \
  --winmd /path/to/WinMetadata/Windows.UI.Xaml.winmd --namespace Windows.UI.Xaml \
  --types IApplication,IApplication2,IApplication3,IApplicationStatics,IApplicationFactory \
  --out native/upstream/raven-xaml/windows_ui_xaml_application.h
```

The generated header records the source winmd's SHA-256
(`c55875b03b9829c2e6ed6ad8b2d1ba33bd80c31208ba3bcda1a12ac46eddc41d` for the
Windows 11 25H2 metadata). `xaml_application.c` provides the class: a factory
reachable by name that offers `IActivationFactory`, `IApplicationStatics` and
`IApplicationFactory`; `CreateInstance` and `ActivateInstance` create an object
with `IApplication`, `IApplication2` and `IApplication3`, each with its own
vtable; `Exit` records the request and every engine member answers `E_NOTIMPL`.
The class is registered in the WinRT catalog, and `native_xaml_application_x64`
/ `_x32` verify activation, the factory QI, `CreateInstance`, the three
interfaces, the runtime class name and the unknown-class answer.

Shapr3D now activates `Windows.UI.Xaml.Application` instead of failing with
`REGDB_E_CLASSNOTREG`. The runtime proceeds into the managed startup and the
next observed stops are the unregistered
`Windows.ApplicationModel.Resources.Core.ResourceContext` (eight lookups, the
one that ends in the fail-fast in `components/shapr3d/launch-nextclass.log`) and
`Windows.System.MemoryManager`.

#### Metadata provider, node loader and Frame/Page (2026-10-28)

The first part of the XAML type system is in place.

- `xaml_metadata.c` maps the framework types the engine implements to their
  WinRT classes, by stable XBF index (Application 31, Window 141, Frame 509,
  Page 525) and by full name, and creates them through `RoActivateInstance` so
  the runtime catalog is the single source of activation.
- `xaml_loader.c` implements `RavenXamlLoad`, an exported entry point that reads
  a compiled XBF document: header, substream table and node records. It resolves
  trusted type references through the metadata provider, instantiates the
  objects, tracks scope nesting and releases them correctly, and refuses a
  document whose nodes fall outside the implemented subset (property values,
  namespaces and the deferred custom-runtime-data blobs are not applied yet)
  instead of half-building it.
- `xaml_controls.c` implements `Windows.UI.Xaml.Controls.Frame` and `Page` with
  the ABI from the winmd: `IFrame`, `IFrame2`–`IFrame5`, the factory and statics;
  `IPage`, the factory and statics. `Frame.Navigate` resolves the `TypeName`
  through the metadata provider, creates the page, attaches the frame to it and
  records the source page type; `CurrentSourcePageType`, `CacheSize`,
  `CanGoBack/CanGoForward`, the navigation events and
  `IsNavigationStackEnabled` are answered; the navigation stack and its
  serialization answer `E_NOTIMPL`. `Page` carries its frame, navigation cache
  mode and app bars.
- `native_xaml_controls_x64`/`_x32` activate both classes, navigate a frame to a
  page by type name, check the source page type and the later interfaces, set
  the page cache mode, and hand `RavenXamlLoad` a minimal compiled document that
  creates a Frame and a Page, verifying the loader and metadata provider.

Shapr3D is unchanged at this point: it still fail-fasts during its own managed
XAML type initialization, before `OnLaunched`, so it does not yet exercise these
classes. The next gaps are applying node properties, resolving application
types through the app's metadata provider, and the element tree the pages host.

#### Window (2026-10-28)

`Windows.UI.Xaml.Window` is registered in the WinRT catalog and implemented in
`xaml_window.c`, with the ABI generated from `Windows.UI.Xaml.winmd`
(`windows_ui_xaml_window.h`, interfaces `IWindow`, `IWindow2`, `IWindow3`,
`IWindow4`, `IWindowStatics`).

- `Window.Current` is the window of the calling UI thread: the first call
  creates one over that thread's `CoreWindow`, later calls return the same
  object, and after `Close` the next call starts a new window. The thread
  reference lives in a TLS slot created and released by the component's
  `DllMain` (`raven_xaml_window_notify`).
- `Activate` makes the XAML window visible even where the host cannot show the
  CoreWindow (a headless run); `Close` closes the CoreWindow and drops the
  thread's window; `Content` is stored and returned with its reference;
  `CoreWindow` and `Dispatcher` come from the thread's CoreWindow; the
  activated/closed/size/visibility events are accepted with tokens and not
  raised yet; `SetTitleBar` is accepted; `Compositor` and `UIContext` answer
  `E_NOTIMPL` (composition is a later stage).
- `native_xaml_window_x64`/`_x32` verify activation, the statics, thread
  identity, the hidden-until-Activate visibility, bounds, CoreWindow and
  dispatcher, the three later interfaces, content, the class name, Close
  starting a new window, and the unknown-class answer.

Shapr3D does not reach `Window.Current` yet: its fail-fast still happens during
XAML type initialization, before `OnLaunched`; the window is part of the engine
groundwork rather than the current stop.

#### Resources, memory and XBF (2026-10-28)

`Windows.ApplicationModel.Resources.Core.ResourceContext` and
`Windows.System.MemoryManager` are registered in the WinRT catalog and have
tests (`native_mrt_x64`/`_x32`, `native_memory_manager_x64`/`_x32`).

- `xaml_application.c` now implements `Application.Start` and
  `Application.Current`: `Start` invokes the runtime's initialization delegate
  through its fourth vtable slot with an
  `IApplicationInitializationCallbackParams` object; `Current` returns the
  application the runtime last created. The delegate call reads the object's
  vtable pointer before indexing it, and the factory returns the created base as
  the instance (returning the outer object stopped .NET Native's startup).
- `resources.c` implements `IResourceContext` and its statics, with
  `GetForCurrentView`/`GetForViewIndependentUse` returning contexts and
  `QualifierValues` as an `IMap<HSTRING,HSTRING>`/`IMapView<HSTRING,HSTRING>`
  carrying `language`, `scale` and `contrast`.
- `memory_manager.c` reports usage and budget from the host's physical memory,
  implements `IMemoryManagerStatics` including the event tokens, and the report
  objects `IAppMemoryReport`/`IAppMemoryReport2`/`IProcessMemoryReport`.

Observed limit: the runtime asks the qualifier map to look up a managed type
name (`Windows.Foundation.Collections.MapChangedEventHandler`2[System.String,System.String]`),
not just the language/scale/contrast keys, and then fail-fasts. That is the XAML
type system talking, not a missing registration: the engine (metadata provider,
type maps, Window/Frame/Page) is the next wall.

XBF: the container is parsed end to end.

- `scripts/pri-extract.py` reads the PRI header and table of contents, the PRI
  descriptor, the hierarchical schema (6226 names in Shapr3D's file), the
  decision info, the resource map (item info and candidates) and the data item
  sections. `--name App.xbf` resolves the schema path `/Files/App/App.xbf` to
  candidate 6, data item section 16, blob 4809: a 3624-byte XBF file
  (SHA-256 `f9af147da45a0dafcae13e6758fa53a9dc2afd730d87680b43f69ce54b6fd61d`
  as extracted from the local package) that begins with `XBF\0`.
- `scripts/xbf-dump.py` reads the XBF header (metadata and node stream sizes,
  version 2.1), the metadata (six table offsets relative to the metadata start,
  the 64-byte hash and the string, assembly, type-namespace, type, property and
  xmlns tables, tolerating both null-terminated and plain string tables), the
  substream table and the node records. Stable type and property indices are
  resolved with `scripts/xbf-stable-indices.json`, generated from Microsoft's
  `StableXbfIndexMetadata.g.cs`.
- `App.xbf` decodes to: `Application`, `CheckPeerType 'Shapr3D.App.App'`, a
  `ResourceDictionary` with `MergedDictionaries` holding `XamlControlsResources`
  and theme dictionaries (`Colors.xaml`, `DefaultButtonStyles.xaml`,
  `GridViewItemStyle.xaml`, `ListViewItemStyle.xaml`, ...), which matches the
  application's App.xaml. The remaining substreams carry the deferred content.

The reader is still a tool, not the runtime: instantiating the decoded object
tree, `Window`/`Frame`/`Page`, controls and templates is the XAML engine stage,
and that is the wall the runtime now reaches.

#### Application types through `IXamlMetadataProvider`; Grid, TextBlock, Button (2026-10-28)

Application-defined XAML types now resolve the way a real application provides them.

- `windows_ui_xaml_markup.h` is generated from the same winmd
  (`scripts/generate-xaml-header.py`, types `IXamlMetadataProvider`,
  `IXamlType`, `IXamlType2`, `IXamlMember`). The metadata declares
  `IXamlMetadataProvider` with `GetXamlType` twice — the first overload takes
  the interop `TypeName`, the second (emitted as `GetXamlType2`) takes the full
  name — and `GetXmlnsDefinitions`; the vtable order follows the metadata. The
  Microsoft Learn page for the interface documents the same three members.
- `Windows.UI.Xaml.Application` now delegates `QueryInterface` for interfaces
  the base object does not carry to the composed outer object it was created
  with (`IApplicationFactory.CreateInstance`), which is how the engine reaches
  the metadata provider a XAML compiler generates on the application class.
- `raven_xaml_create_named` — the path behind both XBF node creation and
  `Frame.Navigate` — resolves a name the framework table does not know through
  `Application.Current`: QI for `IXamlMetadataProvider`, the by-full-name
  `GetXamlType` overload, then `IXamlType.ActivateInstance`. Without a running
  application it answers `CLASS_E_CLASSNOTAVAILABLE` as before.
- `Windows.UI.Xaml.Controls.Grid`, `TextBlock` and `Button` are registered in
  the WinRT catalog and implemented in `xaml_elements.c` with the ABI their
  winmd interfaces declare: `IGrid` + `IPanel` (children), `ITextBlock`
  (text), `IButton` + `IContentControl` (content). Each unimplemented slot
  answers `E_NOTIMPL`. `Page` gained its inherited `IContentControl`, so the
  loader's `Content` property reaches it. The loader applies the stable
  properties `Panel.Children` (648), `TextBlock.Text` (800) and
  `UserControl.Content` (999) and activates the three classes by their stable
  type indices (Grid 442, TextBlock 396, Button 540).
- `Grid.Children` is the parameterized interface
  `Windows.Foundation.Collections.IVector`1<Windows.UI.Xaml.IUIElement>`. A
  parameterized interface has no winmd of its own: its GUID is the SHA-1 of its
  signature (`pinterface({...};rc(Windows.UI.Xaml.IUIElement;{...}))` over the
  namespace `d57af411-737b-c042-abae-878b1e16adee`), giving
  `393d6210-fa21-573a-a238-f0e8733b3c94`, and its vtable carries the twelve
  `IVector`1` methods of the generic definition without the inherited
  `IIterable`1.First`. Both facts were validated against the precomputed
  instantiations in mingw's `windows.foundation.h` (`IVector<HSTRING>`,
  `IVectorView<StorageFile>`, and peers) before use.
- The loader's object attachment now fails a document whose property object
  cannot be attached, instead of silently dropping it from the tree.
- `native_xaml_controls_x64`/`_x32` extend their coverage: a Page Content
  round-trip, a compiled page that hosts `Grid` → `TextBlock` with
  `Text="Hello"` read back through `IPanel.Children` (size, `GetAt`, text), a
  Button content round-trip with COM identity, and the provider fixture — a
  client-side `IXamlMetadataProvider`/`IXamlType` composed onto the Application
  object, reached by the loader through `Application.Current`, returning the
  provider's object as the document root and satisfying `Frame.Navigate` for
  the application type. The full
  `native_core_(application|window|desktop|presentation)_`,
  `native_xaml_`, `native_mrt_`, `native_memory_manager_` and
  `native_dispatcher_queue_` selection passes 30 x64/x86 normal/parallel tests.

Stage 3's remaining wall is the engine behind the interfaces: applying node
properties to the element classes, the app's own `IXamlType`s feeding real
pages, layout, and rendering through the composition stage.

### Stage 4 — controls, text, input, automation

- Controls in the app's markup, authored default styles (Microsoft's
  `generic.xaml` is not redistributable), DirectWrite text via the existing
  component, pointer/keyboard/manipulation routing, automation peers.
- Acceptance: growing control fixtures; Shapr3D's pages parse and lay out.

### Stage 5 — app model

- Package identity from `AppxManifest.xml`; MRT Core (`ResourceManager`,
  `ResourceContext`); `Windows.Storage` subset; `Windows.System.MemoryManager`;
  `Windows.Management.Deployment` already exists.
- Acceptance: identity queries answer from the manifest; resource lookup
  returns strings from `resources.pri`.

### Stage 6 — graphics interop

- `SwapChainPanel`/`ICompositionSurface`, Win2D (`Microsoft.Graphics.Canvas`)
  on D2D1/D3D11, DirectX interop.
- ~~Design question: a SwapChainPanel is a GPU texture, not pixels copied
  into wl_shm.~~ Resolved 2026-10-31 as contract-only (see the
  SwapChainPanel entry): the emulated desktop's presentation path cannot
  accept a composition swapchain, so the real ABI stores and refuses
  honestly while no pixel is faked.
- Acceptance: a Direct3D texture composites in a XAML surface without readback.

### Stage 7 — Shapr3D

- Run the app against the implementation; fix gaps driven by its traces.
- Not a stage with an independent contract: it is the stress test for 0–6.

Run-through (2026-10-31): a fresh 21k-line traced run established the
first real wall. Shapr3D.dll asks the `ResourceContext.QualifierValues`
map for `IObservableMap<String,String>` (the parameterized IID verified
by computing the RFC 4122 v5 SHA-1 of the pinterface signature, the same
algorithm that reproduces mingw's precomputed collection GUIDs); the
engine answered `E_NOINTERFACE`, so .NET Native called the map's
`Lookup` slot with a GC object as its "key", the exception machinery
fail-fasted (1004) and the process died. Fixes, all in `resources.c`:

- Real `IObservableMap<String,String>` on the qualifier map —
  `add_MapChanged`/`remove_MapChanged` with tokens and handler
  ref-counting; the map never changes, so handlers are held but never
  invoked, which is Windows' own behavior for a constant map. The
  QualifierValues getter now returns the observable part (the property's
  declared type).
- A real bug underneath: `qualifier_lookup` read `*length` before the
  same expression's `WindowsGetStringRawBuffer(key, length)` wrote it,
  so every lookup compared length 0 and answered `E_BOUNDS` — even
  `Lookup("language")` never worked. In the app the fix makes
  `Lookup("language")` answer `en-US`.
- `IWeakReferenceSource`/`IWeakReference` with the real Windows ABI
  (IInspectable-derived, method at vtable slot 6): .NET Native takes a
  weak reference on every WinRT object it projects and treats absence
  as fatal.
- `ResourceContext.Languages` — a read-only `IVectorView<String>`
  built from the host's default UI language (`GetUserDefaultUILanguage`/
  `LCIDToLocaleName`), replacing a bare uncommented `E_NOTIMPL`.
- Canonical parameterized runtime class names (`IMap`2<String,String>`,
  `IObservableMap`2…`, full namespace, primitive `String`, no space —
  the format observed in real `Microsoft.UI.Xaml.dll` statics) and real
  `GetIids` for the context/map/view/observable.

The run now proceeds: activation → weak reference → QualifierValues →
MapChanged subscription → `Lookup("language")` = `en-US` → Languages —
then .NET Native initializes `CultureInfo` for the neutral culture
`"en"` and fail-fasts when `GetLocaleInfoEx` near-variant queries
(`LOCALE_SNAME`/`SPARENT`/…, flags like `0x20001010`) meet the host's
en-US-only NLS. That wall and the marshaling/weak-reference walls fell
the same day — see the two entries below. Evidence:
`launch-stage7-1.log` (pre-fix wall) and `launch-stage7-final.log`
(post-fix state). `native_mrt_x64`/`_x32` and `native_mrt_manager_x64`/
`_x32` pin the new surface (codes 60–85).

- Neutral-culture NLS (2026-10-31, stage 7): the trace corrected the
  brief — the old host's `SPARENT("en")` = `""` was already right (Wine's
  conformance suite asserts the empty parent; .NET's invariant walk
  depends on it). The load-bearing gaps were `LOCALE_INEUTRAL` (served 0
  for `en` where 1 is required, sending .NET down the specific-culture
  branch and into the near-variant queries) and the absent
  `ResolveLocaleName`. Fixes in `text_api.cpp`: neutral cultures differ
  from their specific form only in identity fields (SNAME canonical,
  SPARENT `""`, INEUTRAL 1, ILANGUAGE the language id — `en`→`0009`,
  number 9 with `RETURN_NUMBER`); the invariant answers only its empty
  SNAME/SPARENT and refuses other kinds; `ResolveLocaleName` maps
  neutral → Windows's documented default specific (`ar`→`ar-SA`,
  `zh-Hans`→`zh-CN`, …) and passes specifics through; unknown names and
  kinds now fail `ERROR_INVALID_PARAMETER` instead of receiving invented
  en-US data; and the prelude fast path — which answers by LCTYPE
  without seeing the name — now dispatches the four name-dependent
  types to the runtime handler. `RoOriginateLanguageException`
  (`platform_api.cpp`) is implemented with its real BOOL signature and
  per-thread originate record; honest limit stated: no
  `IRestrictedErrorInfo` object exists to hand back, so
  `GetRestrictedErrorInfo` still reports none. The Shapr3D culture walk
  now completes (en-US init → neutral `en` init → parent walk →
  invariant) with every visible NLS answer matching real Windows
  (`launch-stage7b-2.log`); the next throw is .NET Native's
  unhandled-exception drain with no failing API ahead of it. Pinned by
  `native_locale_cli` (checks 22–47).
- Weak references and apartment awareness (2026-10-31, stage 7): the
  weak-reference helper lifted into `xaml_internal.h` — and the lift
  exposed a real lifetime bug in the first implementation: it AddRef'd
  the owner (which could never die while referenced) and its own release
  decremented the owner without running the release path, leaking the
  owner and unbalancing `raven_xaml_objects`. The lifted design gives
  the reference a shared target block holding **no** owner reference;
  the owner disconnects in its release path and a later Resolve answers
  the documented null. `IAgileObject` is answered where thread-safety
  justifies it — ResourceContext, the qualifier map/view/observable
  (subscription list now lock-guarded), CoreApplication, PropertySet
  instances — and refused with inline reasons on thread-affine objects
  (Languages vector's unsynchronized buffer, CoreApplicationView, XAML
  Window per-TLS state); the weak-reference part forwards the marker ask
  to its owner. `IMarshal` stays unimplemented (no cross-apartment
  invocation infrastructure) with the refusal documented. Adoption also
  caught an x86 cdecl/stdcall mismatch in the helper's function-pointer
  type that segfaulted the 32-bit fixture. Fixtures: `mrt_client.c`
  codes 79–85 (agile markers, owner-death resolve) and
  `xaml_window_client.c` 25–28 (window weak source, resolve identity,
  agile refusal pinned).

- Exception diagnosis (2026-10-31, stage 7): `RAVEN_TRACE_EXCEPTIONS`
  now includes the flags, address and bounded parameter list of a
  `RaiseFailFastException` record. In the Shapr3D run the record contains
  code `0x1004`, flags 1, address 0 and no parameters, so it does not
  reveal an HRESULT. No `RoOriginateLanguageException` call was observed.
  The earlier exception trace reveals a null read at
  `sharedlibrary.dll+0x82c8ef`. Unwinding identifies the actual caller as
  `shapr3d.dll+0x1a755a2`; the earlier stack-scan entry
  `shapr3d.dll+0x1374d30` is data, not a return address. The caller uses
  `Windows.UI.Xaml.Application` and queries its composed inner object.
- Application composition (2026-10-31, stage 7): `Application.Start` and
  the application's `CreateInstance` were reached, but the latter returned
  a null inner object for a non-null outer. It now returns a nondelegating
  inner; public interfaces delegate identity and lifetime to the outer, and
  the inner borrows the outer to avoid a reference cycle. The controls fixture
  checks both identities and release on x64 and x86. With the updated
  component, Shapr3D passes the former null read and originates `0x80131508`
  (`Arg_IndexOutOfRangeException`).
- Native digits (2026-10-31, stage 7): instruction tracing found that .NET
  reads ten digits from `LOCALE_SNATIVEDIGITS`; the en-US table returned `1`
  rather than `0123456789`. The corrected shared table supplies both the
  guest fast path and the host handler; `native_locale_cli` covers both,
  length queries and insufficient buffers. Shapr3D proceeds to
  `Application.add_Suspending`, which previously returned `E_NOTIMPL`.
- Application lifecycle subscriptions (2026-10-31, stage 7): Suspending
  and Resuming now retain handlers under distinct event tokens, support
  removal and release remaining subscriptions with the application. The
  desktop does not deliver suspend or resume lifecycle events yet.
- The subsequent Shapr3D run passes lifecycle registration and reaches
  `ApplicationData.LocalSettings`. Root and nested containers now expose
  persistent values maps for boxed Boolean, Int32, UInt32, Int64, UInt64,
  Single, Double and String values. Nested containers use flat hashed
  directories with checked identity manifests, parent generations and a
  32-level depth limit. Recursive deletion preflights the whole subtree and
  refuses corrupt manifests, unknown files and reparse points; stale container
  handles stop addressing a deleted and recreated container. The storage
  fixture checks hierarchy, `Containers` views, persistence, weak references,
  notifications, corruption refusal, deletion safety and ABA behavior across
  separate processes on x64 and x86. Unsupported boxed types still return
  errors; concurrent updates across processes are not coordinated.
  Shapr3D originally failed with `COR_E_KEYNOTFOUND` when indexing absent
  preferences. Microsoft documents that a missing LocalSettings value is null
  in C# ([settings guide](https://learn.microsoft.com/en-us/windows/uwp/get-started/settings-learning-track));
  the values map now returns a successful null for an absent key. With that fix
  the app gets through the fresh-prefix preference lookups without seeded values.
  `RoOriginateLanguageException` diagnostics can now include the HRESULT and
  message when `RAVEN_TRACE_EXCEPTIONS` is set.
- LocalSettings values clear (2026-10-31): `IMap.Clear` validates every
  matching record before deleting values in that container, rejects corrupt or
  linked records, and sends a map reset event. It leaves child containers alone.
  The x64 and x86 fixtures verify persistence, reset notification and refusal
  without modifying records when one is corrupt. Concurrent cross-process
  mutations remain uncoordinated, including during the two-pass clear.
- Disposable preference probe (2026-10-31): placing empty boxed strings in an
  isolated prefix for `inputDevice.usedPointerDevices` and then `device.id`
  advanced execution to `settings.requestedTheme`. Supplying a third empty
  string advanced it to `Application.RequestedTheme`, whose getter and setter
  were unimplemented. The getter and setter now store the application's selected
  light/dark value, defaulting to light. Theme-specific resource selection,
  system preference and rejecting changes after startup remain unimplemented.
  These experiments establish neither the correct types nor defaults and do not justify seeding real settings. The temporary
  records were removed. The language view's `GetMany` now propagates failure
  from string creation instead of reporting a successful empty result.
- Fresh-prefix startup (2026-10-31): `Application.Start` now invokes the
  composed application's `OnWindowCreated` and `OnLaunched` overrides and runs
  the CoreWindow dispatcher until exit. The base override interface supports
  calls from the composed app, and launch arguments expose a false prelaunch
  flag. X64 and x86 fixtures check callback order, arguments and shutdown.
  Shapr3D now reaches `OnLaunched` on a fresh prefix. The direct XAML startup
  path does not yet provide a `CoreApplicationView` session or other activation
  kinds.
- URI activation (2026-10-31): `Windows.Foundation.Uri` now exposes its
  factory, URI, stringable and weak-reference interfaces. Simple absolute
  hierarchical ASCII URIs retain their raw text and expose scheme, path, query
  and fragment. Normalization, escaped and non-ASCII forms, complex authorities,
  relative resolution and several component getters still return `E_NOTIMPL`;
  this is a limited parser, not a complete WinRT URI implementation. The x64 and
  x86 fixtures cover the supported form, identity and unsupported inputs.
  Shapr3D creates `ms-appx:///App/App.xaml` and hands it to
  `Application.LoadComponent` (see the next entry).
- Application.LoadComponent (2026-10-31, stage 3): the three-argument
  `LoadComponent` resolves an `ms-appx:///…xaml` locator through the PRI
  (`/Files/…xbf`, the compiled form of the same path), fetches the blob,
  and loads it into the supplied application instance; the two-argument
  form goes through the same path and the location argument accepts only
  "from URI" (`E_NOTIMPL` for XAML-island form, `E_INVALIDARG` otherwise).
  Escaped, qualified, backslash or query/fragment locators are refused
  with `E_NOTIMPL` rather than guessed. `scripts/make_test_pri.py` grew
  an `--extended` mode to embed real XBF blobs, and
  `native_load_component_x64`/`_x32` cover the resolved load, the
  supplied instance receiving the tree, and the refusals;
  `native_mrt_extended_x64`/`_x32` cover the extended PRI. Shapr3D reaches
  this call, but loading its application resources still fails.
- Application resource dictionaries (2026-11-01): the XBF loader handles
  namespace scopes, `MergedDictionaries` and collection append operations,
  and attaches a completed resource dictionary to the supplied application.
  The x64 and x86 `LoadComponent` fixtures cover two merged dictionaries and
  verify that a failed load does not attach an incomplete dictionary. Unknown
  classes and unsupported operations fail instead of silently producing an
  incomplete application tree. A fresh-prefix trace reaches the creation of
  `XamlControlsResources` and fails there with `E_INVALIDARG`. That resource
  class, dictionary sources, theme dictionaries and custom runtime data still
  need implementation.
- Compiler xmlns type records (2026-11-01, stage 3): the loader assumed a
  type record's first field indexed the type-namespace table. Real compiler
  output uses field 0 == 2, indexing the XML namespace table, whose entry is
  a `using:<CLR namespace>` directive — verified across all 235 type records
  in the package's 84 loadable XBF documents. `metadata_type_name` now
  selects the xmlns table for that marker, strips `using:`, and refuses empty
  directives or `using:` entries carrying a `;` assembly qualifier rather
  than silently dropping it; non-`using:` XML namespaces still fail
  `E_INVALIDARG`. `native_xaml_controls_x64`/`_x32` cover the positive form
  and the refusal (checks 1639–1660).
- WinUI 2.x activation ceiling (2026-11-01, stage 7): after the resource
  dictionary work a fresh Shapr3D trace's first wall is
  `Microsoft.UI.Xaml.Controls.XamlControlsResources` activation —
  `CLASS_E_CLASSNOTAVAILABLE` from the catalog lookup
  (`launch-stage7c-4.log`, lines 1557–1574). A standalone guest probe loaded
  the prefix's `Microsoft.UI.Xaml.dll` and called its
  `DllGetActivationFactory` directly: WinUI itself answers
  `CLASS_E_CLASSNOTAVAILABLE`, because internally it asks for
  `Windows.UI.Xaml.PropertyMetadata` (IID `c1b81cc0-…`) and then for
  `Windows.UI.Xaml.dll`/`windows.ui.dll`, none of which exist here. WinUI 2.x
  is a control library layered on the inbox platform runtime; it cannot
  serve its factory on a WinRT-only raven-xaml.dll, and cataloging
  `Microsoft.UI.*` names would only move the failure. The policy is
  therefore recorded rather than faked: WinUI 2.x apps are outside the
  current platform-XAML surface until framework-package registration and the
  platform classes WinUI asks for (starting with activatable
  `Windows.UI.Xaml.PropertyMetadata`) exist. Evidence:
  `launch-stage7c-1`…`-4.log`; the app still opens no window.
- XBF input bounds (2026-10-31, stage 3): the loader validates metadata
  and declared node-stream extents, variable-length values, truncated
  operands and scope depth before returning a tree. Fixtures cover malformed
  documents and declare node sizes including their substream tables, as in
  the real `App.xbf`.
- Theme dictionaries and `ResourceDictionary.Source` (2026-11-01, stage 3):
  `ThemeDictionaries` is now a real map with its own COM identity, stored
  beside the entries exactly as `MergedDictionaries` owns a separate vector,
  so a theme map handed out by `get_ThemeDictionaries` stays live and is not
  the entry map. Keys compare by content for boxed strings and by identity
  for everything else. `put_Source` resolves an `ms-appx:///…xaml` locator to
  `/Files/…xbf` through the PRI, decodes that document into a temporary
  dictionary and moves both of its maps in only after the load succeeds, so a
  failed load leaves the dictionary unchanged; a dispatchable locator naming
  no resource is `E_BOUNDS`, one dispatchable load that fails is reported
  with its own error, a non-`ms-appx` scheme is `E_NOTIMPL`, and
  self-referential `Source` documents stop at a depth bound instead of
  recursing. `get_Source` returns the stored `Uri`. A null assignment is
  refused because Windows' clearing semantics could not be verified. The
  loader grew dictionary documents: `PushConstant` before
  `AddToDictionaryWithKey` inserts a string-keyed entry, dictionary scopes
  keep the last object across the pop, and `PushScopeGetValue`
  `ThemeDictionaries` (length 17) resolves to the theme map while
  `MergedDictionaries` still resolves to the vector. `SetCustomRuntimeData`
  and `ProvideStaticResourceValue`/`ProvideThemeResourceValue` still refuse
  rather than guess. `scripts/make_test_pri.py` emits `Source.xbf`,
  `Theme.xbf` and a self-referential `Loop.xbf`, and the x64/x86 fixtures
  check theme-map identity, isolation, live views and removal, a `Source`
  load and lookup, missing/looping/`http` refusals, unchanged state after a
  failure, and a theme dictionary reached through `Source`. Resource lookup
  and theme precedence stay unimplemented; nothing yet consumes these maps.
- Compiled type references (2026-11-01, stage 3): `SetValueTypeConvertedResolvedType`
  (node 29) no longer refuses when a target is present. Its property reference
  resolves through the stable property table (the new `Style_TargetType`
  entry) and its type reference through either the stable class table or the
  document's own type table; the type resolves to the authored short name
  (`Button`, not the namespace-qualified runtime class), which is what a
  XAML type property carries. The value reaches `Style.TargetType` through
  the style setter, and a property whose value is not a type name still
  refuses rather than storing the name in the wrong slot. `Style` itself is
  now named by stable index in the metadata table (it was already in the
  runtime catalog) so a document can declare one inline. The x64/x86
  `native_xaml_controls` fixtures build a Style document with node 29 and
  check the stored `TypeName` name and kind (checks 708–711). Shapr3D's
  `App.xbf` uses this form for its `ToggleSwitch` styles; the document is
  still not reachable because the app stops at WinUI activation.
- Element resources (2026-11-01, stage 3): `FrameworkElement.Resources` (stable
  property 404) is a real slot on the Grid, TextBlock and Button elements and
  `raven_xaml_elements_set_object` dispatches the name to it, so a compiled
  page's `SetValue FrameworkElement_Resources` followed by the
  ResourceDictionary scope that carries the page's entries now stores that
  dictionary on the element instead of being skipped. The slot holds an owned
  reference, replaces on reassignment, and is released with the element; a
  null assignment clears it, and `raven_xaml_element_get_resources` creates an
  empty dictionary on first read so an element that never uses resources
  carries none until asked. `raven_xaml_element_put_resources` is the setter
  the loader calls. The x64/x86 `native_xaml_controls` fixtures load a
  document whose Grid resources hold one entry and check the dictionary
  identity, the entry count and the empty-dictionary case after clearing
  (checks 712–718). Resource lookup does not consume these dictionaries yet.
  The earlier x86-only fault was a fixture bug, not the component: a probe
  that typed the exported function as `WINAPI` (`__stdcall`) while the export
  is `__cdecl` popped the arguments twice and corrupted the 32-bit stack; the
  engine-internal exports are all `__cdecl`, and guest declarations of them
  must omit `WINAPI`.
- Resource lookup and resource-valued properties (2026-11-01, stage 3):
  `SetValueFromStaticResource` (node 30) and `SetValueFromThemeResource` (node 36) now carry a resource key — a
  StringReference constant, the form `welcome.xbf` uses for `FrameworkElement.Style`,
  `TextElement.Foreground` and application properties — instead of a literal value. The key resolves through the
  resources of the innermost open scope first and then each enclosing scope, then through `Application.Resources`, the
  documented lookup order. Inside one dictionary the own entries win, then `MergedDictionaries` newest-first, the
  precedence that makes a later merged dictionary override an earlier one; a theme resource adds the theme stage —
  the `ThemeDictionaries` entry named by the application's `RequestedTheme` (`Light` for the default 0, `Dark` for 1),
  falling back to `Default`. The resolved object is applied through the element's setter application (object properties
  through `raven_xaml_elements_set_object`, values through the same per-kind stores a style setter uses), so a resource
  holding a string reaches `Text` the way a literal does. A key that resolves nowhere is skipped when no target is
  present and is `E_NOTIMPL` with a target, consistent with the loader's other unapplied forms.
  `raven_xaml_element_resources_existing` hands the loader the slot without creating a dictionary for elements it only
  walks past. `scripts/inspect-xbf.py`'s sibling dumper (kept out of the tree) confirmed the operand form and the
  placement of these nodes in a real page document. The x64/x86 fixtures check a static lookup ending in the resolved
  `Text` (checks 719–725) and a theme lookup through a `Light` theme dictionary (checks 726–732).
  `ProvideStaticResourceValue`/`ProvideThemeResourceValue` (nodes 34/35) carry the key of another resource and had no
  collected document showing their operand form when lookup landed; the real `App.xbf` has since answered it (see the
  provided-value entry below).
  Merged precedence and the application stage are now exercised end to end: a page whose resources merge two
  dictionaries that both define the same key resolves to the later dictionary (checks 733–734), and an App.xaml-shaped
  document applied to the Application through the newly exported `raven_xaml_load_into` fills `Application.Resources`
  where a page with no such key then resolves it (checks 740–747). `raven_xaml_load_into` is the same target form
  `Application.LoadComponent` and `ResourceDictionary.Source` use internally; exporting it lets a fixture reach the path
  without a private copy.
- `Application.Resources` is never null (2026-11-02, stage 3): the getter now creates the dictionary on the first read and
  the application owns that reference, matching the platform where an application that never assigns `Resources` still
  has a dictionary to read and a compiled `App.xaml` fills the same one. The property can still be replaced by an
  assignment and cleared by a null assignment; a read after a clear creates a new empty dictionary. The existing
  `native_load_component` and `native_resource_dictionary` x64/x86 fixtures were updated to check the non-null, stable
  identity instead of the old null expectation: a failed `LoadComponent` leaves the same dictionary in place (checks
  9–10 of `load_component`), and the dictionary exists and stays the same across reads before any assignment
  (`resource_dictionary` checks 50 and 59).
- `SetCustomRuntimeData` (node 15) and deferred dictionary content (2026-11-02, stage 3): a compiled `App.xaml` does
  not emit its theme dictionaries' entries inline. The `ResourceDictionary` is created empty, and the node after it names
  the substream that builds it. The payload is a LEB128 substream index, the LEB128 legacy static- and theme-resource
  counts (each count is followed by that many persisted u16 node references; they are empty in current output), then the
  LEB128 runtime-data type index: `ResourceDictionary_v1` 371, v2 7, v3 10, v4 12. Because the dictionary is on the
  scope stack when the node arrives, the loader loads the named substream immediately and lets the loaded
  `ResourceDictionary` take the placeholder's place, so the enclosing `ThemeDictionaries` insertion receives the finished
  dictionary and the later lookup sees its entries. A substream load failure leaves the placeholder in place (the
  skip-rather-than-guess rule) unless a target is present. The type index was decoded from the Microsoft XBF reader
  sources and validated against the real `App.xbf` of the Shapr3D package: stream 1, both counts 0, type 10, and the
  payload consuming exactly up to the next node byte. The payload readers for the four dictionary versions and the
  predicates of the v3/v4 conditional maps consume the whole form so a document that carries entries instead of deferring
  them stays in sync; the type indices whose payload is not decoded — `Style` v1/v2/v3 (`2`, `8`, `11`), the legacy
  visual-state manager (`420`) and the deferred-element forms — return `E_NOTIMPL` rather than guess a length. The
  x64/x86 fixture builds a Grid whose `Light` theme dictionary is filled from substream 1 and checks the resolved
  `Text` (checks 752–753), and patches the type index to `Style_v1` to check the refusal (check 755).
- Provided resource values (2026-11-02, stage 3): `ProvideStaticResourceValue` (node 34) and
  `ProvideThemeResourceValue` (node 35) take one operand, a constant key (`XamlBinaryFormatSubReader2::ReadProvideStaticResourceValueNode`
  reads exactly a constant and nothing else). Substream 1 of the real `App.xbf` shows the shape: each provided value is
  immediately followed by `AddToDictionaryWithKey`, so the entry's value is "whatever the key names", not a copy taken
  when the dictionary is filled. The loader therefore stores a reference object — a dictionary-private `IInspectable`
  holding the key, its length and the theme flag — and `resolve_resource` follows references when the entry is read,
  through the same scope stack and theme stage and with a `RESOURCE_HOPS` bound so a reference cycle ends the search.
  This is what makes a compiled `App.xaml` work: an alias declared before its target, or one whose target lives in a
  merged dictionary or an enclosing scope, still resolves. A node 34/35 whose operand is not a string is skipped
  (`E_NOTIMPL` with a target), and a reference that resolves nowhere behaves like any missing key. The x64/x86 fixtures
  check a forward static alias (`Alias` → `Original`, defined after it, checks 756–757) and a forward theme alias inside
  the `Light` dictionary (checks 758–759). A WinRT caller that reads the dictionary directly sees the reference object;
  resolving it at that boundary is not implemented, since no engine path consumes resources that way yet.
- The compiled `App.xbf` walks the loader (2026-11-02, stage 3): the real document (4182 bytes, extracted from the
  x64 package's `resources.pri`, `python3 scripts/pri-extract.py <pri> --name /Files/App/App.xbf --out App.xbf`) now
  drives `load_nodes` past the walls that stopped it. Four fixes carry it. (1) A failed `create_reference` no longer
  poisons the load: the failure lived in `status` even when the skip convention tolerated it, and every later node's
  success left the stale `CLASS_E_CLASSNOTAVAILABLE` as the function's answer — prefix 133 of substream 0 (through
  `EndInitPopScope` after the `XamlControlsResources` creation) failed with it with and without a target. The reset
  clears non-`E_INVALIDARG` failures when no target demands them; a malformed reference (the metadata names no type:
  the URI-namespace and truncated-reference fixtures) still refuses. (2) `PushScopeAddNamespace` (node 18) no longer
  pushes a scope: its namespace bookkeeping is never consulted (document types resolve through the document's own type
  table), and the scope it pushed stood between the `MergedDictionaries` value scope and the `AddToCollection` that
  follows the created dictionary in the compiled stream. `CreateTypeBeginInit` (node 23) now opens the root scope when
  it creates the Application, owning its own reference — the compiled document creates the Application before any scope
  exists, and the dictionary that follows must see it as the parent whose Resources it becomes. (3) The document's tail
  is answered: `AddToDictionary` (node 9) enters a style under its `TargetType` (the implicit key, read back from the
  style and boxed like every other string key), a keyed entry whose value never materialized is skipped without a
  target, and the pop of a scope closes with a pending property assignment applied to the scope the pop closes — the
  compiled document ends `SetValue Application.Resources`, PopScope, with the dictionary the previous pop completed as
  the value. (4) `SetCustomRuntimeData` decodes the Style_v2 payload (seven instances in the real document fixed the
  shape: a setter count, then per setter a property as a stable index or as a name string beside its owner document
  type, and a value as a literal constant or as a theme/static resource marker plus a trailing byte whose meaning is
  not established), and a dictionary-typed payload now loads with the current dictionary as its target so the entries
  join what the scope already holds instead of replacing it. `Microsoft.UI.Xaml.Controls.XamlControlsResources` is
  served by the engine as a real ResourceDictionary of its runtime class (the WinUI default content itself is not
  shipped; its missing keys behave like the missing keys they are), and a Style's `BasedOn` applies through the
  resource-reference form. The x64/x86 fixtures check the choreography (the merged dictionary is created inside the
  namespace scope and its key resolves, 760-761), the served class name (762-764), the node-23 Application whose
  Resources the tail fills (765-773), the implicit-key style (774-775) and the Style_v2 payload consumption (776-777).
  The document's remaining refusals, in order: the six `ResourceDictionary_Source` loads (`ms-appx:///Themes/...`) —
  the PRI scan of this package answers `E_BOUNDS` for every theme path because the hierarchical schema's later entries
  decode to suffix fragments of their names (both `pri-extract.py` and the C reader; the extended `[def_hnames]` name
  form needs its per-entry offsets resolved), so the theme dictionaries stay empty; past that, the deferred setter
  values and the color constants on the brushes they carry (`SetValueConstant SolidColorBrush_Color`, kind-8
  constants) are the next forms to serve. `RoGetActivationFactory` for the class from the application itself is still
  the stage-7 wall below — the loader path serves it, the catalog does not.
- The PRI's theme names resolve (2026-11-02, stage 3): the `E_BOUNDS` the App.xbf Source loads hit was the index's
  name decoder, not a missing index. In the x64 package's extended `[def_hnamesx]` hierarchical schema (6277 entries,
  a 154294-byte ASCII name pool, `unicode_units` 0) the 16-bit per-entry name offset wraps: 4059 of the entries point
  past 64 Ki units, and the wrap truncated each such name to a suffix fragment (`/Files/ettings_offPlaneEdge`,
  `/Files/enu_tangent_secondary.svg`) while early entries — `App.xbf` among them — decoded whole, which is why the
  App path resolved and every theme path answered not-found. The offset's bits 16-17 ride in the entry flags' low two
  bits (`flags & 3 << 16`; the remaining flag bits are 0x10 scope and 0x20 ASCII-pool): oring them in decodes all 6277
  entries, each name's case-folded first character matching the schema's per-entry first-character byte (6276/6276,
  the root's empty name aside). Both decoders carried the bug and both carried a second one beside it: the data item
  sections' string table offsets are byte offsets in the real index (string 31 of section 16 lands on `XBF\0` with an
  exact length — the string pool holds binary XBF values too, 41 string entries among 4886 blobs), while the fixture
  generator and the readers doubled them as UTF-16 units; `pri-extract.py`, `mrt_manager.c` and `make_test_pri.py` now
  all read/write bytes and the checked-in `test_resources.pri` was regenerated to match. With the fix
  `pri-extract.py` resolves App.xbf and all six theme documents, and the extracted files parse (`xbf-dump.py`, XBF
  v2.1 each): `ms-appx:///Themes/Colors.xaml` → `/Files/Themes/Colors.xbf` (4157 bytes),
  `/Themes/DefaultButtonStyles.xaml` → `/Files/Themes/DefaultButtonStyles.xbf` (7383),
  `/Themes/GridViewItemStyle.xaml` → `/Files/Themes/GridViewItemStyle.xbf` (8122),
  `/Themes/ListViewItemStyle.xaml` → `/Files/Themes/ListViewItemStyle.xbf` (10344, candidate 5877 through the string
  table the old reader misread), `/Editor/Overlay/ContextMenu/ShaprMenuFlyoutItemStyle.xaml` →
  `/Files/Editor/Overlay/ContextMenu/ShaprMenuFlyoutItemStyle.xbf` (5068),
  `/Editor/Video/VideoPlayerWithCustomSubtitle.xaml` → `/Files/Editor/Video/VideoPlayerWithCustomSubtitle.xbf`
  (1841) — the compiled `.xaml` spellings map to `.xbf` candidates under the index's `/Files` root. This unlocks the
  six Source loads: the files exist to load; wiring `ResourceDictionary.Source` in the loader's Source setter to fetch
  them through `raven_pri_get_blob` (the C reader's blob path, whose name lookup this fix unbreaks for indexes with
  name pools past 64 Ki units) is the follow-up. The mrt/resource regression suite stays green with the regenerated
  fixtures (native_mrt_manager,
  native_mrt_extended, native_resource_dictionary, native_load_component, native_mrt, x64 and x86).
- Stage 7 re-probe after the resource-dictionary work (2026-11-01): with a
  fresh trace and the current component the first wall is still
  `Microsoft.UI.Xaml.Controls.XamlControlsResources` — `sharedlibrary.dll`
  asks `RoGetActivationFactory` for it, the catalog has no entry, and the
  managed runtime fail-fasts with `0x80040154` ("Class not registered",
  `launch-stage7i.log`). The trace also showed a stale `raven-xaml.dll`
  installed beside `Shapr3D.exe` shadowing the prefix's `System32` copy: the
  loader collapses an absolute `System32` path to the bare module name, so an
  application-local file wins where Windows would load the named path. The
  stale copy (an older build with a different node diagnostic) was removed
  from the test prefix; it had made earlier traces look as if the current
  component was running. `RAVEN_TRACE_STRINGS=1` now renders created
  HSTRINGs in the API trace, which is how the failing class name was read.
  Loading the prefix's `Microsoft.UI.Xaml.dll` from a freestanding probe and
  calling its `DllGetActivationFactory` directly no longer even answers:
  after the `PropertyMetadata` work it faults at load
  (`microsoft.ui.xaml.dll+45d8f`, reading `0x1400021d0`) before any factory
  is asked for. Its 148 imports include the eventing provider, threadpool,
  COM activation and locale surface, none of which this platform provides;
  the WinUI 2.x ceiling recorded above therefore still holds, now with a
  fault rather than a `CLASS_E_CLASSNOTAVAILABLE` answer as evidence. The
  probe and its copied library were removed again.
- `XamlControlsResources` activation through the catalog (2026-11-02,
  stage 7): the stage-7 wall is the managed side, not the loader —
  `sharedlibrary.dll` (the .NET Native runtime) constructs
  `new XamlControlsResources()` outside the XBF loader and asks
  `RoGetActivationFactory` for the class by name
  (`launch-stage7c-4.log` ~1557: the name HSTRING originates at
  `sharedlibrary.dll+9774f0`); the catalog had no entry, the runtime's
  `RegGetValueW` for
  `HKLM\SOFTWARE\Microsoft\WindowsRuntime\ActivatableClassId\Microsoft.UI.
  Xaml.Controls.XamlControlsResources\DllPath` returned error 2
  (~1559-1574) and the runtime fail-fasted with `0x80040154` — the same
  chain the successful `Windows.UI.Xaml.ResourceDictionary` activation in
  the same log shows resolved by a registry hit, `LoadLibraryExW` of
  `raven-xaml.dll` and its `DllGetActivationFactory` (~1355-1359). The
  catalog (`native/nt/registry.cpp`) now lists
  `Microsoft.UI.Xaml.Controls.XamlControlsResources` beside
  `Windows.UI.Xaml.ResourceDictionary` with the same `raven-xaml.dll`
  DllPath, and the component's `raven_dictionary_factory` matches the
  48-character Microsoft.UI name and hands back a dedicated controls
  factory whose `ActivateInstance` builds exactly the object the loader
  path already serves (see "The compiled `App.xbf` walks the loader",
  2026-11-02, stage 3, whose `raven_xaml_dictionary_create_controls_resources`
  this dispatch reuses): a real engine `ResourceDictionary`,
  empty — the WinUI generic.xaml content itself is not shipped and the
  missing default keys behave like the missing keys they are — with no
  `IXamlMetadataProvider` (QI answers `E_NOINTERFACE`) and no statics;
  the factory answers `IActivationFactory`, `IInspectable` and `IUnknown`
  only, and the instance reports the WinUI runtime class through the
  name override. Alongside, the dictionary's `IMap<Object,Object>` reads
  (`Lookup`, `HasKey`) now fall through to `MergedDictionaries`
  newest-first after the own store misses — the documented lookup order
  the loader's resource resolution already used — while `Insert`,
  `Remove`, `Clear`, `Size` and the map view stay on the own store, and
  the `ThemeDictionaries` map shares the code but never falls through.
  The resource fixture gained checks 60-65: the class activates by name
  through `RoGetActivationFactory` (60), the instance QIs
  `IMap<Object,Object>` and `IResourceDictionary` (61), reports size 0
  and `HasKey(missing) == false` (62), accepts a plain engine dictionary
  appended to `MergedDictionaries` (size 1, 63), resolves a key only the
  merged dictionary defines (a boxed string from a compiled document)
  while own entries keep precedence over the merged ones (64), and
  reports the runtime class
  `Microsoft.UI.Xaml.Controls.XamlControlsResources` (65).
  `native_resource_dictionary_x64`/`_x32` pass; the full regression set
  passes except the known `native_edit_text_contract`.
- Theme-dictionary value forms: the `Windows.UI.Color` constant and the
  `SolidColorBrush` (2026-11-02, stage 3). The six real Shapr3D theme
  documents (Colors, DefaultButtonStyles, GridViewItemStyle,
  ListViewItemStyle, ShaprMenuFlyoutItemStyle, VideoPlayerWithCustomSubtitle
  XBFs, extracted from
  `resources.pri` into `/tmp/pri-work/out/`) were dumped and their value
  forms inventoried before any code. The inventory, per node kind across
  all six: `SetValueConstant` 357 (colour payloads 17 —
  `SolidColorBrush.Color` 13, `AcrylicBrush.TintColor` and
  `XamlCompositionBrushBase.FallbackColor` 2 each — enum payloads 16,
  `AutomationProperties.AccessibilityView` 4 and
  `DragOverThemeAnimation.Direction` 12, the remaining 324
  booleans/integers/doubles), `SetValueTypeConvertedConstant` 191
  (KeyTime 80 and Duration 35, `Setter_Target` name strings 61,
  `ResourceDictionary.Source` URIs 6, KeySpline strings 4,
  `TintLuminosityOpacity`/`BackgroundSource` 2 each,
  `RenderTransformOrigin` 1), `SetValueFromThemeResource` 149 and
  `SetValueFromStaticResource` 29 (of which 14 target
  `SolidColorBrush.Color` — the brush built from a colour the dictionary
  does not itself define, e.g. the system accent colours — and 9
  `Setter_Value`), `PushScopeCreateTypeWithConstant` 45 (42 are
  `Windows.UI.Color` structs, all in Colors.xbf; 3 are `ElementTheme`
  enums), `AddToDictionaryWithKey` 37, `SetValue` 77, `SetName` 147,
  `SetValueTypeConvertedResolvedType` 16, `SetCustomRuntimeData` 21, plus
  object creation that is already
  served (`VisualState` 108, `Setter` 61, `Storyboard` 67, key-frame
  animation 150, `SolidColorBrush` 27, `ResourceDictionary` 15,
  `ControlTemplate` 9, `Style` 7, `AcrylicBrush` 2, and the animation
  theme classes). What the loader could not build was exactly the two
  colour forms: the kind-8 constant (a packed ARGB `Windows.UI.Color`,
  which the constant reader silently re-labelled an integer) and the brush
  that constant lands on. Implemented, in the inventory's own order:
  (a) the kind-8 constant is now its own value kind — four bytes read
  ARGB high-byte-first, the spelling `xbf-dump.py` prints (#aarrggbb) —
  and `apply_value` hands it to the engine brush; every other target (the
  `AcrylicBrush` tint colours, a foreign brush) stays the skip it was;
  (b) `SolidColorBrush` (stable type 265, verified against
  `scripts/xbf-stable-indices.json`) is served by the loader's
  `create_known` through a new engine class in `xaml_styles.c` reporting
  the runtime class `Microsoft.UI.Xaml.Media.SolidColorBrush` — the
  Microsoft.UI namespace is what the documents' own xmlns declares
  (`using:Microsoft.UI.Xaml.Media`); its `Color` (stable property 476)
  takes the kind-8 constant, and the readers are the engine's own exports
  (`raven_xaml_solid_brush_color`) rather than a hand-derived
  `ISolidColorBrush` IID, the convention the DependencyProperty interop
  established for surfaces whose projection IID no header in tree carries;
  (c) node 22 with stable type 42 (`Windows.UI.Color`, a struct, not
  activatable — the RoActivateInstance path never had it) boxes the four
  bytes into an engine colour value (`property_value.c`) reporting the
  runtime class `Windows.UI.Color` behind an engine-internal interop
  interface, so the 42 colour structs a real Colors.xbf declares become
  dictionary values instead of skips. Usability, not just storage: the
  element side's brush readers (`style_brush_color`, `Panel.Background`,
  `TextBlock.Foreground`) now read the engine brush's colour beside the
  composition colour brush, and the colour-name store keeps an owned brush
  reference when the value was one (it used to drop the object and keep
  only the colour, which made a dictionary brush invisible to the
  getter). The setter application (`grid_apply_setter`/
  `textblock_apply_setter`) is therefore the same path a theme brush
  travels end to end. Still refusing, honestly: `AcrylicBrush` creation
  and its tint constants (skip), `ElementTheme` enum boxing (skip), the
  `SetValueFromStaticResource`/`SetValueFromThemeResource` colour sources
  the documents reference but never define in these files
  (`SystemAccentColorLight1` and friends — the system theme generation
  owns them), the `SetValueTypeConvertedConstant` time-span/KeyTime forms,
  and the whole key-frame animation surface. Fixture checks 778-793 in
  `xaml_controls_client.c`: a dictionary root holding a kind-8-constant
  brush and a node-22 colour reads both back through `IMap.Lookup`
  (runtime classes and exact bytes, 778-786), and a second document
  applies the brush to `TextBlock.Foreground` through
  `SetValueFromStaticResource` with the brush object returning through
  `get_Foreground` (787-793). `native_xaml_controls_x64`/`_x32` pass; the
  full regression set passes except the known `native_edit_text_contract`.
- The six `ResourceDictionary.Source` loads resolve and the real `App.xbf`
  walks to its first one (2026-11-02, stage 3). The failure the last probe
  of the six Source loads reported was never the PRI mapping: a freestanding
  probe (a fresh `Windows.UI.Xaml.ResourceDictionary`, a
  `Windows.Foundation.Uri` `ms-appx:///Themes/Colors.xaml` through the
  public factory, `put_Source`) gets the `.xaml`→`/Files/….xbf` mapping,
  the large-index name lookup and the nested `raven_xaml_load_into` all
  working — `RavenXamlLastFailure` names the refusing node *inside*
  Colors.xbf, byte 0xb96 (the `SetCustomRuntimeData` whose deferred
  substream builds the theme dictionaries), status `0x80040111`
  (`CLASS_E_CLASSNOTAVAILABLE`): the first object the deferred substream
  creates under a target is the `ElementTheme` enum box (stable type 601,
  `stable-type#601=2` in substream 1 at 0xcd6), which the runtime cannot
  activate — the enum-boxing skip the previous bullet records, now
  refusing rather than skipping because the Source load runs with the
  adopting dictionary as its target. The end-to-end App.xbf probe
  (`RoActivateInstance("Windows.UI.Xaml.Application")`, then
  `raven_xaml_load_into` of the real 4182-byte document) exposed one real
  gap before the Sources: with a target the load refused at byte 0xbc4
  (the `AddToCollection` after the `XamlControlsResources` create,
  `E_INVALIDARG`). Root cause: the
  real document balances its node stream only if `PushScopeAddNamespace`
  (node 18) pushes a scope — the `EndInitPopScope` at 0xbc3 must close the
  namespace, leaving the `MergedDictionaries` collection scope beneath it
  for the `AddToCollection`, and the `XamlControlsResources` object the
  create inside the namespace built must ride out on the closing scope.
  The loader treated node 18 as a declaration (no push), so the pop closed
  the collection scope itself and destroyed the created object. Fix, in
  `xaml_loader.c`: node 18 now pushes an object-less namespace scope, and
  the pop handler collapses an empty namespace scope into the collection or
  map scope beneath it, so the choreography the compiled App.xaml runs for
  every merged dictionary (`GetValue MergedDictionaries`,
  `PushScopeAddNamespace`, create, `EndInitPopScope`, `AddToCollection`)
  delivers the created dictionary to the collection whether the create was
  a scoped node 20 or a node 23. After the fix the App.xbf probe walks past
  the `XamlControlsResources` append (0xbc4) and refuses at byte 0xbc8 —
  the first `SetValueTypeConvertedConstant ResourceDictionary_Source
  'ms-appx:///Themes/Colors.xaml'` — with the same `0x80040111` from the
  nested theme document's enum box; the remaining five Source URIs sit
  behind that one refusal, unchanged. Fixture: `xaml_controls_client.c`
  gained `build_app_merged_node23_document` (the real choreography with a
  node-23 create inside the namespace scope) and checks 4200-4211: loaded
  into a real `Application` with a target, the merged dictionary lands in
  `Application.Resources.MergedDictionaries` and its `Greeting` key
  resolves through the merged fall-through. `native_xaml_controls_x64`/
  `_x32` pass; the full regression set passes except the known
  `native_edit_text_contract`.
- Stage 7 re-probe after the catalog and colour work (2026-11-02): the
  `XamlControlsResources` activation wall is gone. A fresh traced launch
  (`RAVEN_TRACE_API=1 RAVEN_TRACE_FAIL=1 RAVEN_TRACE_STRINGS=1
  RAVEN_TRACE_EXCEPTIONS=1 raven --run-process --execution=native
  .../Shapr3D.exe`, log kept out of the tree) shows no catalog miss and no
  `0x80040154` anywhere — where `launch-stage7c-4.log` recorded
  `RegGetValueW(...ActivatableClassId\Microsoft.UI.Xaml.Controls.
  XamlControlsResources\DllPath) -> 2` and a fail-fast, the new trace
  records the loader-side `create_known` match (`raven-xaml.dll+40759`)
  serving the class and the walk proceeding into the compiled App.xbf's
  deferred dictionary content. The new wall at trace time was an honest
  refusal inside the loader: a substream `load_nodes` ended `E_INVALIDARG`,
  `Application.LoadComponent` returned `0x80070057`, and .NET Native
  originated the language exception and fail-fasted with the same code
  (`RoOriginateLanguageException`/`RaiseFailFastException` from
  `sharedlibrary.dll`) — the node-18 choreography the preceding bullet
  fixed, which is why the six theme `Source` loads never started in that
  trace. After that fix the same walk refuses at the first Source
  assignment's nested `ElementTheme` enum box, so the next increment is
  enum boxing (the inventory's 16 enum payloads and 3 `ElementTheme`
  structs); `RavenXamlLastFailure` is the designed probe for naming the
  exact node. The WinUI 2.x content ceiling stays recorded: the catalog
  serves the class, its dictionary is empty.
- ElementTheme resource values (2026-11-03): the loader boxes the kind-11
  enum constants used by the three `InverseTheme` entries in Colors.xbf,
  checks the embedded stable type against the created type, and applies a
  resolved enum through the integer property path. The x64 and x86 XAML
  controls fixtures verify dictionary lookup, runtime class, value, and a
  mismatched type. A fresh Shapr3D launch with the rebuilt component now
  fail-fasts with `0x80004001` (`E_NOTIMPL`); it does not open a window.
  The exact remaining node or operation still needs diagnosis.
- System colors and theme context (2026-11-03): the refusing operation was
  `SystemAltLowColor` in the Light dictionary of `Themes/Colors.xbf`.
  Deferred theme dictionaries now load when their Light or Dark key is known,
  so their static resources use the dictionary's theme. The initial fallback
  supplies the observed AltLow, AltHigh and BaseLow palette values, and boxed
  colors apply to a solid brush. Fixtures check both themes, resource-to-brush
  application, and enum resources on scalar and object properties; x64 and
  x86 XAML controls tests pass. Dynamic accent colors are not yet supplied.
  A fresh Shapr3D trace reaches the next failure: activation
  of `Microsoft.UI.Xaml.Media.AcrylicBrush` returns `0x80040154`.
- Acrylic brush fallback (2026-11-03): the XBF loader builds the observed
  `Microsoft.UI.Xaml.Media.AcrylicBrush`, stores its source, tint, opacity,
  luminosity opacity and fallback color, and renders its fallback color via
  the existing brush path. The compositor does not yet render acrylic effects.
  The x64 and x86 XAML controls fixtures check the authored values. A fresh
  Shapr3D launch passes this activation and stops later with `E_NOTIMPL`.
- High Contrast defaults (2026-11-03): the next failure is the same
  `SystemAltLowColor` reference in the High Contrast dictionary. The local
  Windows XAML resource's High Contrast substream supplies the same three
  literal fallback colors as its default dark palette, so Raven supplies those
  defaults too; live system overrides remain unsupported. The x64 and x86 XAML
  and MRT regression set passes (14 tests). The next Shapr3D launch gets past
  all three theme dictionaries and stops at Colors.xbf offset `0xe7a`, where
  `SystemAccentColorLight1` is requested; six dynamic accent shades follow.
- Accent resource fallback (2026-11-03): the seven system accent colors read
  the guest user's 32-byte `AccentPalette` when present, and otherwise use an
  approximate blue palette. The x64 and x86 XAML controls fixtures check all
  seven colors; isolated guest prefixes also check a custom palette, truncated
  data and the wrong registry type. Live accent updates and Windows' palette
  generation are not yet implemented. A Shapr3D trace now reaches the
  `Grey10Alpha` resource in Colors.xbf, whose key is in deferred dictionary
  runtime data rather than an explicit AddToDictionary node.
- Deferred dictionary keys (2026-11-03): ResourceDictionary_v3's resource
  map now associates creation-node offsets with keys, including aliases, and
  a dictionary being built participates in its own resource lookup. The x64
  and x86 controls fixtures cover mapped colors, an alias, a brush referencing
  an earlier color, and invalid offsets. The real Colors.xaml load completes;
  Shapr3D next reaches `DefaultButtonStyles.xaml`, whose `Style.BasedOn`
  needs `DefaultButtonStyle` from the WinUI framework dictionary. The package's
  framework PRI is not yet registered or loaded, and nested Source loads also
  need to retain their parent resource context.

- The WinUI framework package resolves and its theme resources merge (2026-11-03, stage 7): the manifest's
  `PackageDependency` elements now name what the engine looks for — `package.c` parses each dependency's Name,
  Publisher and MinVersion, and `raven_xaml_package_dependency_path` locates the dependency's package root in the
  layouts a deployment produces (`<package>\deps\<name>\resources.pri`, beside it, or the same two above the
  package). Each dependency's `resources.pri` registers as its own resource map keyed by the package name
  (`raven_pri_open_map`, four maps beside the main one; the parse runs against an activatable map state, and
  `body_matches` reads the active map's buffer — the bug the first framework probe caught). An `ms-appx` URI whose
  host names a package reads that map — `ms-appx://Microsoft.UI.Xaml.2.8/Microsoft.UI.Xaml/Themes/…` — the shape
  `Findings-M3` of the wine-uwp reference records for Windows. `XamlControlsResources`' creation registers the
  dependency maps and merges `21h1_themeresources.xbf` and `21h1_generic.xbf` from the WinUI framework package into
  the served dictionary's merged list, behind the app's own entries. The merge loads TOLERANTLY: a document whose
  properties and objects the engine cannot all store (Brush.Opacity floats, VisualStateGroups collections, the
  KeyTime storyboards) still merges what it can — the skip convention a load without a target already had, extended
  to the refuse-with-target sites through a `tolerant` context flag; the document's shape is still validated and a
  genuinely malformed form still refuses. Supporting fixes the walk demanded: the engine Uri parses the host form
  (`ms-appx://Microsoft.UI.Xaml.2.8/...` — the authority segment lifts the lower-case-only rule), keyed dictionary
  inserts box the colour and enum constants the framework dictionaries key directly (`AddToDictionaryWithKey` with a
  packed ARGB or an enum constant — the app documents only reached it through node 22), and a nested load's failure
  record survives the outer overwrite (`nested_failed` — the deepest node is the evidence). The public
  `Windows.Foundation.Uri` serves from raven-web.dll (see the converged-Uri commit): its urlmon parser handles the
  ms-appx schemes, and the weak-reference source .NET Native takes moved onto it.
- Platform default styles (2026-11-03, stage 7): the OS theme resources — the surface
  `Windows.UI.Xaml.Resources.*.dll` carries on Windows, per the wine-uwp overrides list — define the named default
  styles compiled documents base their own on. Raven authors them: the `Default*Style` family (Button, TextBox,
  ToggleSwitch, AppBarButton, CommandBar), `PaneToggleButtonStyle` and the named typography styles
  (`BaseTextBlockStyle`, `BodyTextBlockStyle`, `CaptionTextBlockStyle`, `TitleTextBlockStyle`,
  `SubtitleTextBlockStyle`) build once as sealed engine styles carrying their TargetTypes and no setter content — the
  setters the real ones have are the OS's, not ours to copy — and `resolve_resource` serves them below every
  dictionary, after the system-colour fallback. A style that bases on them brings its own setters; a control styled
  only by the platform style keeps its own authored defaults.
- Stage-7 wall moved (2026-11-03): with the framework merge and the platform styles, the traced Shapr3D launch walks
  the compiled `App.xbf`, merges its dictionaries, loads `Colors.xaml`, and reaches `DefaultButtonStyles.xaml`,
  whose `Style.BasedOn` now resolves `DefaultButtonStyle` through the platform fallback; the launch then builds
  Frame, Page, Grid, Button, TextBlock and ContentPresenter elements (29 Style objects in the trace) before the next
  `E_NOTIMPL` fail-fast. The next wall needs the freestanding probe's evidence — `RavenXamlLastFailure` names the
  node — and the probe harness (a guest client loading a real XBF beside the package, writing
  `RavenXamlLastFailure` to a file) is the tool the stage's next increment starts from.

- Stage-7 re-probe (2026-11-03): a copied Shapr3D prefix and an Application-target App.xbf probe exposed a missing
  stable property mapping: index 244 is `Style.BasedOn`. The platform-style implementation already existed, but the
  loader could not dispatch that property. The mapping and a targeted x64/x86 fixture now cover the lookup and assignment.
  `Style_v2` payloads now consume object and other deferred values as variable-length stream offsets, matching the WinUI
  serializer; the reader does not yet apply those setter values. Dictionary auto-undeferral entries are shared strings,
  and conditional-object maps contain a token before each predicate vector; fixtures cover both. A named, mapped resource
  that is not a FrameworkElement can use its matching map key as its name. Both native controls tests pass. A traced
  Shapr3D launch now reaches `GridViewItemStyle.xbf` at `0x1190`: an implicit style bases on `GridViewItemStyle`, whose
  mapped explicit style occurs later in the deferred stream at `0x11a2`. Resource resolution needs to materialize that
  forward-referenced entry before assigning BasedOn. The app has not reached a usable window.

- Deferred forward-resource lookup (2026-11-03): active v3 dictionary maps now let resource resolution load a requested
  entry at its stream offset. The nested walk shares the dictionary and map, records the consumed range so the outer walk
  reuses the same object, and guards cycles and excessive reference depth. The map also reads the implicit-style table:
  Shapr3D's implicit GridViewItem style has no trailing AddToDictionary node, so its runtime-data token must insert it on
  scope close. Synthetic regressions cover forward brush/color dependencies, shared-token aliases, implicit styles whose
  BasedOn is a later explicit style, object identity after the normal walk reaches that style, and a cyclic reference.
  Stable type names 560/561/489 (`GridViewItem`/`ListViewItem`/`ToggleSwitch`) allow Style.TargetType to store those names; these entries do
  not implement the controls. Loader failures now print the offset and HRESULT with `RAVEN_TRACE_DEBUG=1`, including
  validation failures after the stream walk. All launches use `/tmp/raven-shapr-probe/prefix`, leaving the original package
  prefix unchanged. Style_v2 setter values remain unapplied.
- Compact binary PRI items (2026-11-03): after the GridView dictionary loaded, the runtime rejected
  `/Files/Themes/ListViewItemStyle.xbf` before the loader ran. Its candidate points into the data section's 16-bit
  offset/length table, which is also valid for binary data; `pri_lookup_blob` had only accepted the wide table. It now
  reads either table as bounded byte ranges, matching `scripts/pri-extract.py`. The existing ambient-Source XBF fixture
  uses compact storage while the other XBFs retain wide storage, so the LoadComponent tests cover both. The 16 focused
  application/controls/MRT/resource/LoadComponent tests pass across x64/x86. The traced launch with this fix reaches
  ListViewItemStyle.xbf at 0x124e (Style.TargetType ListViewItem). With both item-style names and ToggleSwitch mapped,
  the rebuilt 16-test set passes again and Shapr3D gets beyond both dictionary Source loads and the ToggleSwitch style.
  The new wall is App.xbf 0xdb9: PushScopeCreateTypeWithConstant Double (stable 52) with a kind-3 float, 1000.0.
  The loader boxes Color and ElementTheme constants but has no Double boxing fallback, so it answers
  CLASS_E_CLASSNOTAVAILABLE (0x80040111). `property_value.c` already has the Double boxing implementation behind
  PropertyValue.CreateDouble; reuse it rather than invent another box. Evidence:
  `/tmp/raven-shapr-probe/launch-forward-complete.log`. Shapr3D still has no usable window.

- Double resources and framework URI regression (2026-11-03): the loader now boxes stable type 52 with a float
  constant through PropertyValue's existing Double box. A dictionary regression reads 1000.0 through IPropertyValue;
  the 18 focused x64/x86 tests pass. The launch gets past App.xbf 0xdb9 and then fails at 0xde3, the SolidColorBrush
  whose Color names ListViewItemBackground. This exposed a correction to the earlier framework-merge claim:
  `framework_themes` is a fixed-width WCHAR array, and `ARRAYSIZE(framework_themes[file]) - 1` passed embedded nulls
  to URI creation. The dependency PRI registered, but the actual theme documents did not merge. The builder now uses
  the filename's string length and restores the tolerant-load flag on early exits. The LoadComponent fixture now has
  an authored synthetic dependency with both framework theme paths and checks that two populated Source dictionaries
  merge. The rebuilt 18-test set passes. The real framework load then failed at 0x50834 (GradientBrush.GradientStops):
  unknown PushScopeGetValue properties still refused tolerant loads, and skipping them without pushing a collection
  scope unbalanced the document. Unsupported collection scopes now remain balanced and discard only their own values;
  strict application loads still refuse unsupported members. The synthetic framework theme includes that nested gradient
  shape followed by a known entry, so the regression checks the surviving dictionary contents. The 18 tests pass again.
  A traced launch now walks framework entries as far as MenuFlyoutSubItemRevealBorderBrushDisabled before the 20-second
  limit, with no XBF failure. An isolated Xvfb :97 launch with a larger startup allowance is being observed next.
  Logs: `/tmp/raven-shapr-probe/launch-double.log`, `launch-framework-uri.log`, `launch-framework-scope-trace.log`.

- Resource loading cost and type-name decoding (2026-11-03): with actual framework loads enabled, an isolated X11 run
  spent the entire 90-second startup allowance in dictionary key comparisons without creating a window. String keys now
  cache an FNV-1a hash used only as a comparison prefilter; equal hashes still use the original content comparison, and
  non-string keys retain IUnknown identity semantics. A regression uses the colliding keys `costarring` and `liquid`.
  All 26 XAML/style/resource/MRT/property-value/settings tests pass. The real launch then reaches concrete failures in
  the framework's implicit styles (theme document 0x692b8: MenuFlyoutPresenter; generic document 0x1a3ea: RadioButton).
  Type-valued properties had incorrectly reused the small instantiable-class table. A generated short-name table now
  derives from the repo's existing stable XBF index JSON, independently of activation support. Index zero and out-of-range
  indices remain unavailable; named application types still read the document's own metadata. The first run after this
  fix passed the RadioButton/MenuFlyoutPresenter failures and finished in 16.09 seconds, then exposed a retired stable
  name at theme offset 0x69b4a: ProgressBar_Deleted0 (528). Older compiled files legitimately use that index, so type-name
  decoding restores the original name by removing the metadata's retirement suffix. TargetType regressions cover all
  three observed controls. The expanded 26-test set passes after restoring those legacy names. Logs: `launch-framework-x11.log` (timeout),
  `launch-framework-hash.log` (the next failures), under `/tmp/raven-shapr-probe/`.

- Merged theme lookup (2026-11-03): both real framework documents now load without XBF failures, but App.xbf 0xde3
  still refuses its Color assignment. The resource lookup had two independent gaps: theme selection was only applied
  to the current dictionary, not its merged dictionaries, and StaticResource skipped theme dictionaries entirely.
  The dictionary's own entries and selected theme now precede recursive newest-first merged lookup. Both markup forms
  use the same initial theme selection, consistent with Microsoft's ThemeResource documentation:
  https://learn.microsoft.com/en-us/windows/apps/develop/platform/xaml/themeresource-markup-extension . Synthetic
  regressions cover own/merged theme dictionaries, static/theme reads, and a static alias inside a theme dictionary;
  all 26 tests pass. The real launch still reports App.xbf 0xde3 after 16.97 seconds, so lookup success versus value-type
  mismatch is being traced explicitly. Logs: `/tmp/raven-shapr-probe/launch-legacy-types.log`, `launch-merged-theme.log`.

- Persistent dictionary deferral (2026-11-03): the next real-app diagnostic showed that ListViewItemBackground resolves
  to a Microsoft.UI.Xaml.Media.SolidColorBrush, while App.xbf 0xde3 assigns it to another brush's Color. Do not silently
  coerce that object into a Color. The more basic issue was eager materialization: Windows retains compiled deferred
  entries until lookup, while Raven still constructed the entire stream and failed on an unused definition. See the
  deferred-resource behavior in Microsoft's ResourceDictionary implementation:
  https://github.com/microsoft/microsoft-ui-xaml/blob/main/dxaml/xcp/components/resources/ResourceDictionary.cpp .
  V3 dictionaries now retain a shared copy of their compiled document and per-key callbacks. Map and view lookups realize
  values on demand; HasKey and Size do not. Replacing/removing/clearing a key releases its deferred context, and Source
  adoption transfers entries without capturing the discarded dictionary object. Compiler-marked auto-undeferral keys
  are still created during load. Deferred loads retain strict validation, alias identity and cycle guards. The initial
  XBF load walks only the primary stream; named substreams are reached through their owning runtime-data nodes.
  Added regressions destroy the input buffer before lookup, retain a view after the root/map are released, keep an unused
  cyclic entry beside a valid value, and check that malformed/cyclic definitions fail when requested. All six focused
  controls/dictionary/LoadComponent tests pass on both widths. The effective-theme pointer now restores its prior value
  after each nested load so releasing a deferred document cannot leave a dangling theme slice. The final expanded suite
  passes all 26 x64/x86 tests (18.85 seconds). A real isolated X11 launch passes XBF resource loading and reaches
  ApplicationData.LocalSettings (`appstate.numberOfAppStarts`), then fails on a new activation request:
  `Windows.System.UserProfile.GlobalizationPreferences` is absent from Raven's WinRT catalog, returning 0x80040154.
  The untraced launch exits after 5.89 seconds, with no usable window. Evidence:
  `/tmp/raven-shapr-probe/launch-lazy-final.log`, `launch-lazy-final.time`, and `launch-lazy-final-trace.log`
  (the missing-class trace is around line 531186). This is the next startup blocker. Style_v2 setter values are still
  not applied. Work paused at the user's request to wrap up; changes remain uncommitted, and the isolated Xvfb is stopped.

## Performance rules

- One GPU compositor; XAML visuals and the 3D viewport composite on the GPU.
  A `SwapChainPanel` is a GPU texture, not pixels copied into `wl_shm`.
- Damage-tracked layers; text via glyph atlas/caching, not per-frame
  rasterization.
- Dispatcher work is per-frame bounded; input is delivered before rendering.
- Measure with `scripts/benchmark-frames.py` (DXVK + driver + compositor) to
  establish the viewport floor; the XAML engine must not lower it.

## Risks

- **XBF is undocumented.** Mitigation: reverse-engineer with REA against the
  app's `resources.pri` and the Windows XAML loader's behavior; keep the reader
  versioned and fail closed.
- **ABI drift.** The generated headers must be validated against
  `Windows.UI.Xaml.winmd` for the Windows release being targeted.
- **Default styles.** Authored by Raven, not copied from Windows.
- **Text and IME.** DirectWrite component already exists but is incomplete; UWP
  text layout (Windows.UI.Text) is a large surface.
- **Automation.** Stub peers initially; accessibility is a later deliverable.
- **Boolean ABI.** mingw's WinRT headers type `boolean` as one byte; validate
  against the winmd before relying on it for the app.

## Repository layout

- `native/upstream/raven-xaml/` — the component (guest WinRT classes).
- `native/nt/registry.cpp` — class catalog.
- `native/nt/system_exports.h` — `runtime_components` (loaded beside the runtime).
- `testdata/native_loader/` — guest clients, built per architecture.
- `native/CMakeLists.txt` — component build and test registration.
- `docs/uwp-xaml.md` — this plan.

## Delay-load failure hook (resolved 2026-10-28)

Shapr3D's .NET Native runtime delay-loads the optional `mrt100.dll`; its absence
is [documented as supported](https://github.com/MicrosoftDocs/windows-dev-docs/blob/docs/uwp/dotnet-native/net-native-and-compilation.md).
When no component hook answers the failure, the runtime calls
`KernelBase!DelayLoadFailureHook`, which Raven left unimplemented, so startup
stopped before XAML activation.

The hook's contract was taken from the mounted Windows 11 25H2 installation
rather than guessed. `KernelBase!DelayLoadFailureHook` (export ordinal 280)
first consults a registered hook and otherwise raises exception **`0x505`**,
flags **1** (noncontinuable), with two parameters: the library and procedure
names. `mrt100_app.dll` contains the matching filter (`cmp dword [record],
0x505`, `sete`) at `0x180083234`. The MSVC `0xc06d007e`/`0x7f` codes and Wine's
`0x80000100` are not what this runtime checks.

Raven now raises that exception from the `DelayLoadFailureHook` handler
(`native/nt/seh.cpp`), with the record built by `native_delay_load_failure`
(`native/nt/exceptions.cpp`) and covered by `native_exception_contract`. A
native launch of the local Shapr3D package with `mrt100.dll` absent now passes
the delay load, unwinds through `RtlUnwindEx` with code `0x505`, and reaches the
same Windows Runtime class-activation stop as with the DLL present; the trace
contains no unimplemented API. Evidence: `components/shapr3d/launch-dllhook2.log`.

The handler is registered in the 64-bit exception registry. The 32-bit path
dispatches exceptions inside the runtime component (`native/nt/x86_process.cpp`)
and does not serve `DelayLoadFailureHook` yet; no 32-bit Shapr3D launch exists to
require it.

Reproduce with `RAVEN_TRACE_API=1 RAVEN_TRACE_FAIL=1 build/native/raven
--run-process --execution=native --timeout-ms=15000 /path/to/Shapr3D.exe`.
The earlier `delay_failure_probe` and the MSVC/default-hook experiments below are
retained as methodology notes; actual Windows observations are no longer needed
for this decision.

Default-hook research: trial raises of the documented MSVC delay-load error
`0xc06d007e` and NT missing-module/missing-entrypoint statuses reached the .NET
runtime's handlers but were not handled. Changing the DLL callback's error from
NTSTATUS to Win32 did not recover startup either. These experiments were removed;
no guessed default-hook implementation remains in Raven.

Build `delay_failure_probe` to obtain `build/native/delay-failure-probe64.exe`
and `delay-failure-probe32.exe`. This standalone, CRT-free conformance probe
records the exception code, flags, parameter count and argument identities;
when applicable it decodes only the public `DelayLoadInfo` structure. Set
`RAVEN_DELAY_PROBE_CASE` to select:

| Case | Operation |
| --- | --- |
| 1 | Call the default hook directly with a missing DLL and named export |
| 2 | Call the default hook directly with an ordinal export |
| 3 | Resolve a missing DLL through the default system failure hook |
| 4 | Resolve a missing export in an existing DLL through that hook |

Run both executables on Windows, capturing each case's output; for example in
PowerShell, from the directory containing the probes:

```powershell
foreach ($width in 64, 32) {
    foreach ($case in 1..4) {
        $env:RAVEN_DELAY_PROBE_CASE = "$case"
        & ".\delay-failure-probe$width.exe" > "delay-$width-$case.txt"
    }
}
Remove-Item Env:RAVEN_DELAY_PROBE_CASE
```

Wine's x64 and x86 observations for all four cases are exception `0x80000100`, flags 1,
and two parameters matching the supplied DLL and procedure pointers (or
ordinal). That is Wine-specific behavior, not a Windows conformance result.
Actual Windows observations are still needed before choosing the default hook's
precise exception and payload. The existing ten selected loader/exception tests
remain passing after the trial implementation was removed.

## Immediate next increments

1. ~~Implement the observed `DelayLoadFailureHook` startup path and retry
   Shapr3D.~~ Done: the hook is implemented and the runtime now continues past
   the optional component's absence.
2. ~~WinRT header generation from `Windows.UI.Xaml.winmd` for `IApplication`.~~
   Done: `scripts/generate-xaml-header.py`, the generated header,
   `Windows.UI.Xaml.Application` activation and the two x64/x86 tests.
3. ~~Register `Windows.ApplicationModel.Resources.Core.ResourceContext` and
   `Windows.System.MemoryManager` with tests.~~ Done.
   ~~Parse the PRI resource descriptor index and the XBF node stream.~~ Done:
   `scripts/pri-extract.py`, `scripts/xbf-dump.py` and
   `scripts/xbf-stable-indices.json` resolve `App.xbf` and decode its nodes.
   `Windows.UI.Xaml.Window` (with `Window.Current`) is implemented and tested.
   ~~Metadata provider, XBF node instantiation and `Frame`/`Page`.~~ Done:
   `xaml_metadata.c`, `xaml_loader.c` (`RavenXamlLoad`) and `xaml_controls.c`,
   with `native_xaml_controls_x64`/`_x32`.
   ~~Apply node properties and names.~~ Done: the loader applies `SetName`
   (`Name`), `SetValueConstant`/`SetValueTypeConvertedConstant` and
   `SetValue`/`SetValueFromMarkupExtension` against both the stable property
   table and the document's own property table, and hosts a decoded page
   through `Window.Current`/`Window.Content` (pointer-equality checked).
   The controls test also builds a document that declares `Frame` and
   `CacheSize` in its own metadata tables, so untrusted type and property
   references resolve through the document rather than by stable index.
   ~~Resolve application-defined types through the app's
   `IXamlMetadataProvider` and grow the element tree into the control set.~~
   Done: provider resolution through `Application.Current` (see the 2026-10-28
   section under stage 3), and `Grid`/`TextBlock`/`Button` with children,
   text and content so a compiled page builds a real tree.
   Next: apply the remaining node property forms and expand layout and
   rendering beyond the implemented subset (see the stage 3 entries).
   XBF-declared templates and ContentPresenter support include stable-index
   references for Template/ControlTemplate/ContentPresenter, the generic
   template-store path, `ContentTemplate` 525 and nested-template application.
   Further template-adjacent members remain.
4. ~~Extend DispatcherQueue with standalone controllers and deferred shutdown
   when required by app traces.~~ Done (2026-10-31): standalone controllers
   landed 2026-10-28 and deferred shutdown (ShutdownStarting deferrals
   honored on the controller pump) on 2026-10-31 — remaining if app traces
   require it: deferral deadlines and the immediate-close deferral.
5. ~~Finish pointer history/static access, cursor objects and touch/pen
   delivery as native input sources permit.~~ Pointer history, the
   CoreCursor/`PointerCursor` objects and the `PointerDevice` statics are
   done (2026-10-31); touch delivery landed the same day over the
   host's `WM_POINTER` path (see the touch-delivery entry) — remaining:
   the host-side plumbing a real digitizer would need (`GetPointerInfo`,
   `EnableMouseInPointer`, `SendInput`, a populated pointer table).
6. ~~Extend application startup with manifest identity/entry-point data
   and `RunWithActivationFactories` as the app's startup path requires.~~
   Done as far as the emulated desktop honestly reaches (2026-10-31):
   `CoreApplication.Id` reads the manifest's
   application id and `Properties` is a real process property set;
   `RunWithActivationFactories` validates its callback and refuses the
   packaged run it cannot serve. Manifest-driven activation is
   as far as the emulated desktop honestly reaches (2026-10-31): the
   manifest `EntryPoint` travels to `IFrameworkView::Load` as the launch
   argument, but the entry-point class name itself is not activated by the
   component — it names the guest's own Application subclass, served
   through the guest's `IXamlMetadataProvider`, and no packaged guest
   process exists to probe host activation with. The launch-args interfaces
   that exist (`IActivatedEventArgs`, `ILaunchActivatedEventArgs`) are
   implemented; `IBackgroundActivatedEventArgs` was refused — no
   background activation is delivered.

- Globalization preferences (2026-11-04): registered `Windows.System.UserProfile.GlobalizationPreferences` and served its
  `IGlobalizationPreferencesStatics.Languages` using the existing ResourceContext UI-language view. Other preferences
  return `E_NOTIMPL`. The x64 and x86 resource fixtures exercise activation, list size, access and bounds. An isolated
  Shapr3D launch passes that activation and requests `Windows.Globalization.ApplicationLanguages`. That class now exposes
  its default language list and primary override. The latter persists in a per-application settings file and affects subsequent
  language views; the view currently contains only one language, and validation checks tag shape rather than the full BCP-47
  registry. Manifest language enumeration remains
  unimplemented. Both resource fixtures pass with read, write, invalid-tag and list checks. The isolated app launch now reaches
  the app-owned `Shapr.SPLib.MTLanguageIDMapping`, declared in its package manifest with server `Shapr.Native.dll`
  (`/tmp/raven-shapr-probe/launch-override.log`).

- Package WinRT activation and graphics (2026-11-04): on a registry miss, Raven now reads the current package's
  in-process activatable-class declarations, validates the relative server path, and loads the declared DLL. The x64 and
  x86 package fixtures cover activation, an unrelated extension category and path traversal. Framework-package declarations
  and namespace-prefixed manifest elements are not yet handled. Shapr3D now resolves its class to `Shapr.Native.dll`.
  Configured 64-bit DXVK and vkd3d-proton DLLs are copied beside the runtime and included in release inputs; a clean guest
  can find them without app-local copies or `RAVEN_D3D_DIRECTORY`. D3D12 device creation, clears, a drawn triangle and
  compute pass on both GPU and software Vulkan; device creation also passes through the new runtime DLL lookup on each.
  Shapr3D still exits while loading its native library: the trace reports missing `CoreMessaging.dll` and missing exports
  from kernel32, ICU and ucrtbase (`/tmp/raven-shapr-probe/launch-package-dxvk.log`). No Shapr3D window has been verified.

- Native-library imports (2026-11-04): added the CoreMessaging forwarder to the XAML dispatcher controller; dedicated
  queues without explicit COM apartment setup work, while current-thread and explicit-apartment modes return `E_NOTIMPL`.
  The forwarder carries relocations, tested with its preferred base occupied. Corrected version API-set routing and added
  `OpenFileMappingFromApp`. The UCRT now supplies the imported next-toward, remainder, error-function, string conversion,
  reverse and invalid-parameter handler entry points. The 64-bit UCRT fixture checks numerical results and thread-local
  handler isolation; a 32-bit guest also checked the math and conversions. Both dispatcher fixtures and the dependency test
  pass. The initial isolated trace (`/tmp/raven-shapr-probe/launch-erf.log`) then exposed missing ICU exports.

- Further Shapr3D startup (2026-11-04): ICU plural rules and list formatting now support English (US); unsupported
  locales return an error. Application data exposes a distinct LocalCacheFolder. `LoadPackagedLibrary` resolves libraries
  in the current package, but does not yet handle a full package dependency graph. Thread UI language is maintained per
  thread. Package and PackageId have distinct COM identities, and Package.InstalledLocation returns a read-only folder
  rooted at the manifest directory. Package and storage fixtures pass on x64 and x86, including installed-folder write
  denial. The latest isolated launch (`/tmp/raven-shapr-probe/launch-installed-location.log`) loads the native library
  and graphics DLLs. A launch with an older runtime reported `Windows.Data.Xml.Dom.XmlDocument` as unregistered;
  rebuilding the runtime and using the existing XML DOM component passes that point. Both XML DOM fixtures pass.
  A Shapr3D window and its use of DirectX 12 have not yet been verified.

- Guest heap comparison (2026-11-04): the configured RTL still faults in `RtlpRemoveFreeBlock` during thread
  teardown (`/tmp/raven-shapr-probe/launch-xml-current.log`). An isolated runtime built with
  `scripts/provision-rtl.py` passes the heap and heap-capacity fixtures and gets past that point; Shapr3D then
  reports the unregistered `Windows.System.Profile.SystemManufacturers.SystemSupportInfo` class
  (`/tmp/raven-shapr-probe/launch-own-rtl-trace.log`). This comparison does not establish that changing the
  configured RTL is safe for other applications, and the experiment did not change the configured component.

- Shapr3D continuation (2026-11-04): `SystemSupportInfo.LocalDeviceInfo` now answers from the guest system-information
  registry and hostname. Missing hardware information is empty; the other SystemSupportInfo statics remain unimplemented.
  LocalSettings can persist null values, including replacement and deletion. Thread-pool timers now schedule callbacks;
  cancellation of already submitted callbacks is not modeled. ApplicationView supports the current-view and default-launch
  statics, a window title, preferred minimum size, and a title-bar object. Title-bar colors are retained but not rendered
  by native decorations; fullscreen, some resizing and alternate launch modes remain incomplete. The x64/x86 system-support,
  local-settings and application-view fixtures pass, as does the x64 timer-queue fixture.

  With the isolated in-tree RTL build (`/tmp/raven-shapr-probe/runtime-own/raven` and
  `/tmp/raven-shapr-probe/own-rtl/raven-rtl.dll`), Shapr3D reaches application view setup and gets past its launch-mode
  request. The latest launch (`/tmp/raven-shapr-probe/launch-view-statics3b.log`) exits with `0x8000000e`
  (`E_ILLEGAL_METHOD_CALL`); identify the originating WinRT call next, for example by tracing
  `RoOriginateLanguageException` with `RAVEN_TRACE_STACK=1`. Another launch returned `0x800401f8`
  (`CO_E_DLLNOTFOUND`), so the class-loading race or missing library also needs investigation. The trace separately
  reports missing `Windows.Storage.Streams.DataWriter` and `Windows.Services.Store.StoreContext`; the app continued past
  those errors, but their functionality is not established. The normal configured RTL still has the earlier heap teardown
  crash. Neither a usable Shapr3D window nor its DirectX 12 renderer has been verified.

- The XML DOM (2026-11-04): `Windows.Data.Xml.Dom` is served in full for the surfaces a document consumer
  reaches — `xml_dom.c` in the XAML component carries a UTF-16 XML parser (declaration, comments, CDATA,
  processing instructions, DOCTYPE skipping with its close consumed, entity references, whitespace handling
  with a leading byte-order mark stripped), the node tree with the `IXmlNode`/`IXmlNodeSelector`/
  `IXmlNodeSerializer` triple every node serves, the kind-specific interfaces, `IXmlDocument`,
  `IXmlDocumentIO` (`LoadXml`, `LoadXmlWithSettings`, `SaveToFileAsync` writing UTF-8), `XmlNodeList`,
  `XmlNamedNodeMap` (its vtable declared locally because MinGW's drops the namespace form's arguments),
  `XmlDomImplementation`, `XmlLoadSettings` with the documented defaults, the statics' file loads, and the
  factory answering `IActivationFactory`, `IXmlDocumentStatics` and `IXmlLoadSettings` — a .NET Native
  program takes the statics interface from `RoGetActivationFactory` the way it takes the factory, and the
  same shape serves `IXmlLoadSettings` from its factory. The activation factories return object pointers
  whose first word is the vtable (an early build returned the vtable itself and the guest executed it as
  code). The XPath of the selector covers child and attribute steps, wildcards, `//` and the `[n]`/`[last()]`
  predicates; namespace URIs are not tracked. `raven_storage_file_path` (storage.c) and `raven_package_root`
  (package.c) support the file-backed loads. The `native_xml_dom_x64`/`_x32` fixtures parse a manifest-shaped
  document and walk the surfaces above. Parser fixes the fixtures demanded: child elements enter `p_element`
  past their `<` (only the root's caller skipped it before), and four growth sites used `HeapReAlloc` on a
  null block, which refuses, so first allocations failed (`realloc` semantics do not carry to `HeapReAlloc`).
- Activation-route repair (2026-11-04): a Shapr3D launch needs `RAVEN_PREFIX` pointing at the isolated
  prefix — without it the runtime's default prefix lacks the package's libraries (`missing PE module:
  sharedlibrary.dll`). The package directory carried a copy of `raven-xaml.dll` from the morning's build,
  which shadowed the runtime's fresh one for every launch (the loader prefers the program's folder); the
  copy is refreshed alongside the component now. The shipped `raven-shell.dll` was rebuilt from source with
  `scripts/provision-controls.py` — the checked-in binary predated the package-manifest activation, whose
  code an earlier `git checkout` of `winrt.c` had reverted (the package-server fallback is restored in the
  source and the rebuilt binaries).
- The startup surface behind the DOM (2026-11-04): the traced launch after the DOM work exposed the classes
  the runtime and the app ask for next, served in this round: `Windows.Storage.ApplicationData`'s
  `TemporaryFolder` (the `Temp` directory of the storage root, the LocalState pattern), `Windows.Foundation.
  Metadata.ApiInformation` (`metadata.c` — a type is present when the activation catalog carries it, members
  answer as their type, `UniversalApiContract` up to the mounted metadata's major), the .NET-Native-
  synthesized attribute family `Windows.Foundation.Metadata.ApiI` (a bare attribute object; the real
  `ApiInformation` factory is asked first, the family's prefix match would otherwise take the name), and
  `Application.UnhandledException` (the event-list pattern Suspending and Resuming use). Every `E_NOTIMPL`
  return across the XAML and storage components now names its function in the trace. The 38 focused
  x64/x86 tests pass; the pre-existing `native_xaml_application_x32` failure is unchanged and reproduces
  with the component's XML work removed.
- Stage-7 wall (2026-11-04, evening): the traced launch with all of the above activates `XmlDocument`
  (factory and statics), loads the app's Apple-PLIST documents, reads and writes
  `ApplicationData.LocalSettings` containers (the `.rvs` persistence files land in
  `LocalSettings\containers`), and reaches the .NET Native runtime's own fail-fast `0xC0000602` on a
  worker thread (94) in teardown — `ResolveDelayLoadedAPI` from `mrt100_app.dll` followed by
  `RaiseFailFastException` at `+454fe`. The failing late delay-load's descriptor is not decoded yet; that
  is the next increment's starting point. No Shapr3D window has been verified.

- Fiber-local callbacks at thread exit (2026-11-04, late): the fail-fast under the last bullet is the .NET
  runtime's thread teardown — the failing worker thread ran `RoInitialize(MTA)`, `RoUninitialize`, its FLS
  teardown, and then resolved `RaiseFailFastException` through its delay-import directory (decoding
  `mrt100_app.dll`'s `.didat`: `api-ms-win-core-errorhandling-l1-1-3!RaiseFailFastException`,
  `api-ms-win-core-memory-l1-1-3!SetProcessValidCallTargets`, `mrt100.dll!GetManagedRuntimeService`) and
  called it deliberately. The missing prerequisite is FLS callback delivery: Raven's `FlsAlloc` stored the
  guest's callbacks but nothing ever invoked them, and `mrt100_app` keeps its per-thread runtime state in
  FLS (its `FlsSetValue` of slot 1 during thread init). `fls.h` now carries the slots (the same object the
  `Fls*` handlers use, hung off `KernelContext`), and the thread-exit notifier runs each used slot's
  callback with the thread's value for it before the `DLL_THREAD_DETACH` notifications, the way
  `LdrShutdownThread` orders them; a callback's stop propagates like a TLS callback's. With delivery, the
  fail-fast is gone and the same teardown runs five FLS callbacks and 52 module detach notifications.
- The next wall (2026-11-04, late): the detach walk crashes in `raven-rtl.dll`'s `RtlpRemoveFreeBlock` — a
  free-list entry reading `0xa00000410` — inside `vcruntime140.dll`'s `DLL_THREAD_DETACH`, deterministically,
  after the FLS callbacks and the other 51 detaches ran. Without the FLS delivery the same launch returns
  to the `0xC0000602` fail-fast (gated by `RAVEN_SKIP_FLS_DETACH=1` for the comparison), so the delivery is
  correct and the crash is the next increment: a heap free-list corruption during the CRT teardown of a
  worker thread, with the other worker threads still allocating beside it. No Shapr3D window has been
  verified.

- The heap teardown crash was the stale RTL, not a corruption (2026-11-05): the `RtlpRemoveFreeBlock`
  fault decoded cleanly from the exception registers (the dump printer at `seh.cpp` emits the CONTEXT in
  layout order; `rcx + r11*8 + 0x418` with `r11 = Size-1 = 0xffffffff` because the free block's Size read
  zero — the ReactOS heap's `FreeHints[Size-1]` hint lookup out of bounds). The crash ran in the
  pre-replacement ReactOS build of `raven-rtl.dll` that `components/rtl` and `build/native` still carried
  from October 27; commit `7606d2f` had already replaced the heap with Raven's own, and the probe runtime
  picked the new build at 08:13. `components/rtl/raven-rtl.dll` is now provisioned from the in-tree source
  (`scripts/provision-rtl.py`) and copied into `build/native`, so the configured component matches the
  replacement the commit made; the heap and heap-capacity fixtures pass. The old "the experiment did not
  change the configured component" caveat is now resolved by that deployment, with the focused fixtures as
  its evidence rather than a broader proof.

- AutomationProperties (2026-11-05): the next wall after the RTL was `Windows.UI.Xaml.Automation.
  AutomationProperties` unregistered — the XAML framework's automation path activates its statics during
  startup, and the managed `Class not registered` went unhandled to a fail-fast. The class is now served
  (`automation_properties.c`, catalog entry in registry.cpp): the factory answers `IActivationFactory`
  and the nine `IAutomationPropertiesStatics` interfaces the winmd declares (generated into
  `windows_ui_xaml_automation_properties.h` by `scripts/generate-xaml-header.py` from the pinned
  Windows.UI.Xaml.winmd). Get/Set of the attached properties is real over an attached-property store
  keyed by the element pointer (the same keying the peers use, entries for the process lifetime): string
  properties answer their value or NULL, value properties answer the stored number or the documented
  unset default (IsDataValidForForm TRUE), Set of a null string clears, and GetName serves only the
  attached value — the peer's own precedence chain stays where it was. The DependencyProperty getters
  and the collection-valued members (ControlledPeers, Annotations, DescribedBy, FlowsTo, FlowsFrom) are
  not answered yet; the asking callers name themselves through `raven_trace_not_implemented`. An
  `automation` fixture for the statics is still to be written.

- The null-QueryInterface wall and agility (2026-11-05): with the class registered, the launch moved to a
  managed NullReferenceException right after the Frame activation — a null dereference inside the
  framework's `QueryInterface(obj, IID_IUnknown, out)` helper, raised as a real `c0000005` the runtime
  catches and re-raises (`RAVEN_TRACE_EXCEPTIONS` shows it; a diagnostic in the exception reporter prints
  the requested IID when `rcx` is null). The frame's QI was instrumented to name every interface it
  refuses, which exposed the .NET interop's agility negotiation: the framework asks a fresh XAML object
  for `IAgileObject` (94ea2b94-…), `INoMarshal` (ecc8691b-…) and `IMarshal`, and the refused
  `IAgileObject` sent the RCW down the marshaler path that ends in the null. The frame now answers
  `IAgileObject` — the objects run apartment-free in the engine's single process, so the marker is true —
  and the NRE is gone. Other objects (the ApplicationView pair reported `IJupiterObject` earlier) have
  not been through the same negotiation yet.

- Trace plumbing repaired (2026-11-05): `raven_trace_not_implemented` gated itself on the guest's
  `RAVEN_TRACE_DEBUG`, which the guest environment never carries (the trace shows the lookup failing with
  error 203), so every E_NOTIMPL note had been silent. The helpers now print through `OutputDebugStringA`
  ungated and the host plays the debugger under the host-side variable, the way the ApplicationView QI
  notes always worked. `raven-shell`'s `RoGetActivationFactory` traces each activation's class, requested
  IID and outcome (`winrt: activation …`, printed under `RAVEN_TRACE_DWRITE`); those traces are what
  identified both walls above.

- An instrumentation mistake, found and repaired (2026-11-05): a mechanical pass wrapping `return
  E_INVALIDARG;` with trace notes rewrote whole lines, deleting the conditions of seven checks in the
  storage component (the language-override validator among them, which made every `en-US` override write
  fail with E_INVALIDARG and briefly pointed the diagnosis at a rejection that did not exist). The
  conditions are restored with the notes kept behind them; the lesson recorded is that per-line
  instrumentation must only touch lines that are exactly the return.

- The current wall (2026-11-05): with the frame agile and AutomationProperties served, the launch reaches
  the app's own C++/WinRT layer, which throws `hresult_invalid_argument` ("Value does not fall within the
  expected range") during the early XAML style/dictionary setup — the throw site is inside
  `shapr.native.dll` (frames through `vcruntime140_app`'s C++ exception machinery, ~14 ms into the run),
  and per-site E_INVALIDARG notes across every raven component (storage, elements, loader, dictionary,
  styles, controls, composition, resources, pointer, dispatcher, xml_dom) never fire, so the failing call
  is inside the app's code or a surface outside those checks. The next increment needs deeper
  app-side instrumentation (RAVEN_STEP on the interop call before the throw, or decoding the C++
  exception's hresult source at `shapr.native.dll+b6b00`). No Shapr3D window has been verified.

- Inside the app's own settings handling (2026-11-05, later): Raven's step machinery (`RAVEN_STEP=API:
  occurrence:count`) walked the instructions from the last host call before the `hresult_invalid_argument`
  and pinned the failing round to the settings key `com.shapr3d.shapr.userDefaults.remoteConfiguration.
  expirationTimes`: the app's userDefaults wrapper reads that value through the container (the decode
  returns the JSON string), queries `IReference` on the boxed value, releases both — and then throws
  `hresult_invalid_argument` ("Value does not fall within the expected range") from inside
  `shapr.native.dll`. The persisted `.rvs` record for the key was decoded offline and is correct (type 12
  string, 8012 bytes, the 4006-character JSON balanced and complete), the E_INVALIDARG notes across every
  raven component never fire, and the originate carries no error info — the throw is the app's own code,
  most plausibly its parse or validation of the value (or a nested call into a class that is not
  registered — the trace separately shows the app handling "Class not registered" on the same thread
  before the fail-fast). No raven-side surface for this failure is identified yet; the next lever is
  watching what the app does with the decoded string (its JSON parse loop) or serving the classes its
  remote-configuration path activates. The trace runs: /tmp/raven-shapr-probe/step-invalidarg{2,3,4}.log,
  launch-nre{12..19}.log.

- DataWriter (2026-11-05, evening): the remote-configuration handler's HTTP setup activates
  `Windows.Storage.Streams.DataWriter` — the only class miss in the app's active path — and the class is
  now served (`data_writer.c`, catalog entry): the factory answers `IActivationFactory` and
  `IDataWriterFactory` (CreateDataWriter over an IOutputStream), the writer's Write* methods really
  append honoring ByteOrder (big-endian values byte-reversed) and UnicodeEncoding (UTF-8 with surrogate
  pairs, UTF-16LE/BE), MeasureString answers the encoded size, DetachBuffer hands the accumulated bytes
  out as an IBuffer with IBufferByteAccess and empties the writer, StoreAsync and FlushAsync complete at
  once (a given stream is written through IOutputStream::WriteAsync; without one the memory store is its
  own store), and the buffer's Length/Capacity/put_Length are real. The interface declarations are
  MinGW's windows.storage.streams.h — whose slot orders and identifiers match the pinned winmd, though
  its StoreAsync/FlushAsync/DetachStream signatures spell older types, so the three slots forward through
  wrappers that carry the real ABI. The class is not the cause of the remaining wall: with it served the
  handler still throws `hresult_invalid_argument` before any DataWriter call, on a settings round where
  a 48-character string value is set into a child container whose previous record (the 98-character
  JSON) decodes cleanly.

- The writer's first growth hit the null-HeapReAlloc refusal again (2026-11-05): `writer_grow` called
  `HeapReAlloc` on a null block and the program's C++/WinRT layer surfaced the E_OUTOFMEMORY as
  std::bad_alloc (the app fail-fasts on it). Windows' HeapReAlloc refuses a null block the way the XML
  DOM's four growth sites already learned; the writer allocates for its first growth. A refusal diagnostic
  (size, pending, extra) stays behind the failure path.

- The launcher traces (launch-dw1/2/3, step-dw1/2) show the app's sequence around the remaining wall:
  the app's `Shapr.CommonApp.MTRemoteConfigurationHandler` (its own class, served from Shapr.Native.dll
  through the package-manifest route) reads the container, sets a 48-character string value (the round's
  PropertyValue CreateString precedes the child-container open), the previous record's JSON decodes, and
  the wrapper throws — with no error info on the originate, an explicit throw in the app's code. The
  48-character value is not printable ASCII; identifying it (and what the wrapper validates) needs the
  app's own symbols or a working-Windows comparison of that settings round.

- The DataWriter wall moved (2026-11-06): with DataWriter served and its access bug fixed, the
  `hresult_invalid_argument` round is gone and the launch fails differently — the .NET framework's own
  dead-reference marker (`0xBAADF00D`, stored by SharedLibrary at an object's +0x10 slot) is later
  dereferenced as a pointer (`c0000005 reading ffffffffbaadf01d` from the interop at
  `sharedlibrary.dll+82844e` under the app's startup continuation frames `shapr3d.dll+686b0/68928`), and
  the runtime fail-fasts with `E_FAIL`. A dead RCW means a backing object's lifetime ended early — an
  unbalanced Release on some cached raven object (the Application/Window/Frame family the startup
  continuations hold) is the likely shape; identifying which object needs the RCW's identity at the
  marker write (the store site is SharedLibrary's `mov qword [rdi+0x10], 0xBAADF00D`, reachable by
  stepping from the last WinRT call before the fail-fast).

- DataWriter (2026-11-05/06): the class the remote-configuration HTTP path activates is served
  (`data_writer.c`, catalog entry, `native_data_writer_x64/_x32` fixtures). The factory answers
  `IActivationFactory` and `IDataWriterFactory`; the writer's Write* methods append honoring ByteOrder
  (big-endian byte-reversed) and UnicodeEncoding (UTF-8 with surrogate pairs, UTF-16LE/BE),
  MeasureString answers the encoded size, DetachBuffer hands the bytes out as an IBuffer with
  IBufferByteAccess and empties the writer, StoreAsync/FlushAsync complete at once (a stream is written
  through IOutputStream::WriteAsync; the memory store is its own store). The declarations are MinGW's
  windows.storage.streams.h — slot orders match the pinned winmd, and the three slots MinGW spells with
  older types (StoreAsync, FlushAsync, DetachStream) forward through wrappers carrying the real ABI.

- Two component bugs the fixture demanded (2026-11-06): `buffer_access_data` cast the IBufferByteAccess
  part pointer to the owner without the offsetof adjustment, so Buffer() answered the bytes at
  capacity/length instead of the payload (the fixture read `0x700000007` — the two length dwords — where
  the heap pointer should have been); and the writer's first growth used HeapReAlloc on a null block,
  the Windows refusal the XML DOM growth sites already learned, which the app surfaced as std::bad_alloc.
  A component can arm a hardware write watch through a temporary host hook (`OutputDebugStringA` with a
  "raven-watch <hex>" line) — the watch did not fire in this mode and the struct-byte dump identified the
  accessor bug instead.

- The baadf00d wall decoded as a double teardown, not a dead RCW (2026-11-06): the faulting code is
  SharedLibrary's wrapper destructor (`+8283e0`) — it walks a node list at object+0x10 releasing each
  node, then marks the list head dead with `movq $0xbaadf00d,0x10(%rdi)`; the fault is a SECOND pass
  dereferencing the marker. Both passes entered through the same `mrt100+44590 → sharedlibrary+828c56 →
  Release(+828350)` chain, so the wrapper's refcount reached zero twice — an over-release. Catching the
  first teardown needed the step machinery anchored on the frame's CreateInstance note
  (`RAVEN_STEP='OutputDebugStringA:N:count'`, the notes being the only per-run-stable landmarks).

- The over-release was the frame's agility answer (2026-11-06): `frame_query_any` served IID_IAgileObject
  with `&self->first` — a delegating part whose Release lands on the outer when the frame is aggregated —
  while AddRef'ing the frame itself. Every agility negotiation consumed one outer reference the outer
  never gained, and the wrapper machinery tore its wrapper down twice. The fix answers with the
  non-delegating inner part when an outer is present, so the handed pointer releases against whatever was
  AddRef'd. With the fix the double teardown is gone and the launch's exit moved from E_FAIL (5) to a
  managed null reference (E_POINTER, 3).

- IReferenceTracker (2026-11-06): the .NET runtime's lifetime-coordination face most XAML framework
  objects carry ("implemented by most XAML framework objects" per the SDK docs; IID 11D3B13A-180E) — the
  traced run asks it of the Frame exactly once, and it was the only QI refusal in the whole run. Served
  now (`reference_tracker.c`, a shared part any owner can embed following the weak-source shape, with
  real per-owner counters for the connect/disconnect/peg notifications), plus the one process-wide
  IReferenceTrackerManager the runtime drives its collection rounds against. FindTrackerTargets reports
  no targets: the components hold only strong native references, so reporting none can only delay a
  collection, never collect a live object. Fixture `native_reference_tracker_x64/_x32`.

- IDependencyObject (2026-11-06): the base XAML property face (IID 5C526665-F60E) the interop asks for
  right after activating the Frame; its refusal made the app die on a null reference right after naming
  the frame RootFrame. The frame answers it (`frame_dependency_part`), with the dependency-value methods
  traced E_NOTIMPL and get_Dispatcher the view's own. The interface is hand-declared in xaml_controls.c
  (the IControlTemplate precedent) because the component's hand headers predate it — the elements will
  share the declaration when the trace brings them here.

- The null-HeapReAlloc bug class, third occurrence (2026-11-06): the app sets
  `AutomationProperties.SetAutomationId(RootFrame, "RootFrame")` and the setter returned E_POINTER —
  the AutomationProperties attached-store's first growth called HeapReAlloc on the NULL vector, the
  Windows refusal the XML DOM growth sites and the DataWriter learned before. `attached_find` now
  HeapAllocs its first allocation; SetAutomationId succeeds and the launch moved again.

- The unset-attached-getter default (2026-11-06): reading an unset attached property answers its
  documented default (Level 0, IsPeripheral false, LabeledBy null), not E_POINTER — `number_get` writes
  the 0 default for an absent slot. Fixture `native_automation_properties_x64/_x32` covers the defaults,
  twenty sets through the store's growth, and a second element's isolation. AutomationProperties'
  factory also answers IID_IActivationFactory now (the app had only asked for the statics directly, so
  the gap was invisible to the launch).

- Inside the marshalling wall (2026-11-06, later): the 80131500 "EETypeRva:0x0001B1C0 is missing interop
  type marshalling data" failure is the .NET interop's marshalling-data lookup for the app's own type,
  during the setup of the call that follows SetAutomationId — no raven API is on the failing path. The
  register-aware step machinery (RAVEN_STEP_REGS printing the register state, @ with the absolute rip,
  and RAVEN_STEP_DUMP=module+offset dumping the descriptor at Rcx) decoded the failing lookup: the
  call-site descriptor carries module = Shapr3D.dll (0x180010000 in the traced run), its interop-data
  table slot is NULL, and the fallback reads the baked RVA 0x1B1C0 and throws. Per the public CoreRT
  sources, that data lives in per-module blobs registered through RhpRegisterModule and found by hashing
  type handles across every loaded module's blob (RhFindBlob); the null slot says the app module's
  registered interop block is missing or its blob never resolved. All three .NET Native modules carry an
  empty PE CLR-header directory (their normal shape — the module header is a plain structure passed to
  RhpRegisterModule), so the anomaly is not there. Next: decode the module header RhpRegisterModule
  receives (trap the export or dump its argument), or compare a working-Windows run of the same lookup.
  Further decode: the descriptor's pointer fields carry no strings (it is a signature descriptor — a
  SharedLibrary code pointer, an app data pointer and a 0x3a7e hash), the failing call is set up entirely
  inside SharedLibrary's argument-marshalling driver (no app stub on the failing path), and the context's
  table slot is a stack-local the machinery never filled — so the real failure is the fallback blob lookup
  itself. The run's early 80040154 throws are C++/WinRT class-not-registered raises during
  PropertyValue-related initialization (background threads, caught, the run continues past them); whether
  they leave the runtime's module bookkeeping damaged is one candidate the next increment should test
  before decoding the module header. The module bases are stable across runs (Shapr3D.dll 0x180010000,
  SharedLibrary.dll 0x182df0000), so the failing context is app-owned (its call-site pointer sits in the
  app's text) while offset 0x1B1C0 reads as a plausible EEType only in SharedLibrary — the missing
  marshalling data is likely for a framework type, consulted through the app call site's context. The
  failing helper is AOT-compiled managed code (the null "table" is a managed array — length at +8, data
  at +0x10 — never built), whose fallback scans every loaded module's marshalling blob; those blobs are
  found by mrt100_app!RhFindBlob (+0x46b60), which walks the runtime's registered modules and reads each
  module's section directory at [module+0x60]. The registration chain is decoded: the app calls
  RhpRegisterModule(header) once (its IAT entry is a by-name import at slot 0x1370948, called from
  RVA 0x181d82f) with the header at **RVA 0x1000** (.rdata's first bytes: dwords 3, 0xc, 0x1ac8c,
  0x0117f2dc, ... — a table of small RVAs/counts, not CoreRT's 'RTR' ReadyToRun header; this is .NET
  Native 2.x's own layout). mrt100_app's chain: RhpRegisterModule (+0x45450) → ModuleManager::RegisterModule
  (+0x4db94, a linked-list insert) → Module::Create (+0x50214, a 0xb8-byte object storing the header at
  +0x18; the constructor reads header fields at +0x2c (a dword RVA) and +0xc0 (the module base pointer)).
  The next increment decodes which header field becomes the Module's [0x60] section directory (the
  +0x28 sub-object constructors at +0x55e6c are string-table readers so far) and statically verifies the
  app's header carries the blob section the interop lookup needs — separating "the header lacks it" from
  "raven's loader breaks the walk". Then a reframe: the failing helper's fallback throws WITHOUT calling
  RhFindBlob — it reads the type's RVA straight from a baked descriptor in the app's own .rdata (the
  descriptor's first pointer, app+0x9b7ff8) — so this call site was COMPILED as a direct throw stub: the
  .NET Native toolchain knew at build time that this signature's marshalling data is absent and emitted a
  throw. Both module headers (app and SharedLibrary, both at RVA 0x1000, same layout: version dword 3,
  section sizes at +0x90 matching each module's sections, and RVA|0x80000000 section descriptors at
  +0xb0..) are structurally identical, so nothing is wrong with the app's header. On Windows this call
  site is dead code — the app's managed logic never reaches it — which moves the hunt EARLIER: some
  raven-served result before OnLaunched's frame setup diverges from Windows and steers the app's branch
  onto the path with the throw stub. Identifying the divergence means diffing the app's decision points
  (the settings rounds, the early 80040154 class-not-registered throws, Window.Current and license state)
  against what Windows would answer. Then two corrections from a register-aware rerun: the accumulated
  settings state is NOT the divergence (a fresh-state launch fails identically, the state restored
  after), and the failing round DOES call RhFindBlob — early in its setup (mrt100_app+46b60 with the app
  module and blob type 1), its result feeding a ~4000-step SharedLibrary processing chain before the
  throw — so the blob lookup's outcome and that chain remain undecoded. The failing call's signature
  descriptor carries a hash (0x3a7e): if the earlier divergence changed WHICH type the app passes to the
  interop (a different event handler or struct than Windows' path uses), the signature itself differs and
  its marshalling data is genuinely absent — making the divergence hunt, not blob mechanics, the right
  front. Two decisive steps followed. First, a static scan of the app image for a delegate- or
  struct-marshalling entry keyed by the failing type's EEType pointer finds NONE — the data is genuinely
  absent from the binary, so the same call would fail on Windows if reached. Second, an env-gated
  diagnostic (RAVEN_TOLERATE_FAILFAST: RaiseFailFastException returns instead of ending the process) shows
  the program SURVIVES the wall — it runs past OnLaunched's abort and settles into the dispatcher loop
  (PeekMessageW/MsgWaitForMultipleObjectsEx from the dispatcher) for the rest of the 120-second run, but
  no window is created because OnLaunched was aborted before the view setup. The way to a real fix is
  naming the failing type: the app embeds two NativeFormat metadata blobs (signature 0xDEADDFFD at file
  offsets 0xa38344 and 0xc1fb61), and a parser for that format (CoreRT's generated reader documents it)
  resolving EETypeRva 0x1b1c0 to a type name would say what the app was subscribing or calling when the
  divergence steered it onto the uncompiled path. The apartments are ruled out (the app's own code calls
  RoInitialize(1) — MTA — on the main thread, the same code Windows runs); the activation landscape is
  clean (no failed factory asks, no unimplemented-surface notes in the final run). The tolerated run also
  shows the window pipeline starts EARLY: raven creates its CoreWindow host (CreateWindowExW
  "Raven.Xaml.CoreWindow" in the first dozens of APIs), the app resolves the EGL entry points
  (EGL_CreateContext/CreateWindowSurface/SwapBuffers through libegl.dll) and activates
  Windows.UI.Core.CoreWindow — all before the marshalling wall — and the rendering would come after the
  view setup the aborted OnLaunched skipped. The blob type passed to RhFindBlob in the failing round is 1
  = the TypeMap (reflection), not the delegate/struct stub maps (16/17), so the delegate lookup rides the
  per-module thunks instead; scans of all three modules for a marshalling entry keyed by the failing
  type's EEType (pointer- and RVA-keyed) find nothing in any of them. Neither is the failing RVA a plain
  module-relative EEType: the cells at offset 0x1b1c0 in both the app (a .text pointer, zeroed flags) and
  SharedLibrary (pairs of values with a 0x180 stride) are not EETypes, and TypeManager-relative candidates
  (the header's +0x2c field plus its self-pointer) land on unrelated data — so the message's RVA uses a
  base this increment has not identified, and naming the type needs the NativeFormat metadata decode (the
  two 0xDEADDFFD blobs) or a working-Windows comparison of the same startup. Further narrowing: the
  frame's interface ledger shows the app never asked the frame for IFrame2 (the interface carrying
  add_NavigationFailed), so the failing call is NOT the frame event subscription; the round also consults
  RhFindBlob with blob type 1 = the TypeMap (the reflection type map) rather than the delegate/struct stub
  maps, and it processes a managed object in the runtime's 0x200000000 arena — the marshal driver's
  working set — which cannot be named from outside. The app's whole native stack comes up before the wall
  (window host, libegl/libGLESv2 initialized, CoreWindow activated); the remaining identification routes
  are a full NativeFormat metadata + type-manager decode or a working-Windows capture of the same startup
  to diff the branch. The two 0xDEADDFFD blobs proved to be metadata islands rather than a directly
  parseable NativeFormat stream (the second carries the app's real assembly/type strings —
  "$Shapr3D.McgInterop", the CommunityToolkit and Sentry references), the lone BSJB hit in the image is a
  false positive inside code, and the host window is never SHOWN (no ShowWindow call in the tolerated
  run): the show rides the view pipeline that the aborted OnLaunched gates. The practical state is
  therefore: with RAVEN_TOLERATE_FAILFAST the process is alive indefinitely with its full native stack up
  (window host created, EGL/GLES initialized, CoreWindow activated, dispatcher pumping), and the single
  remaining blocker is the one interop marshalling round whose compiled data does not exist in any module.
  The failing type is now mechanically identified: a scan for EEType-shaped cells finds the app's 8409
  EETypes in .rdata, and the message's EETypeRva is relative to the module header (RVA 0x1000) — the
  failing EEType sits at RVA 0x1c1c0 with flags 0x410c0000 = a type created DYNAMICALLY at runtime
  (IsDynamicTypeFlag) with a dispatch map and GC pointers. A runtime-created generic instantiation has no
  compiled marshalling data by construction; the runtime should specialize it from the generic
  definition's data and that lookup failed too. The UniversalApiContract answer (lowered 25 → 15, the
  mounted 26100 metadata's real version) did not change the wall, the newly served IReferenceTracker is
  exonerated (refusing the ask leaves the wall identical; the tracker branch is restored), and the
  Application event subscriptions (Suspending/Resuming/UnhandledException) all REACH raven before the
  wall — so the delegate stubs for those exist and the failing round is later in OnLaunched, plausibly
  the Navigate(System.Type) round whose type-name marshalling rides the very TypeMap the failing round
  consults. The divergence that makes the app reach this round on raven and not on Windows remains the
  unknown; naming the dynamic type's generic definition (the metadata decode) or a working-Windows
  capture are the two remaining identification routes. The round's managed call chain is now captured
  (RAVEN_DUMP_FAILFAST_STACK dumps the fail-fast thread's stack page by page, its values resolved to
  modules offline): OnLaunched's managed code (frames shapr3d+0x1a33c2f/0x2b1f8e1/0x2b20f6a) enters a
  LAZY-INITIALIZED cached-WinRT-object getter (shapr3d+0x2b20fe0: a state flag at [ctx], a cached value
  at [ctx+0x30], the slow path building the object through interop wrappers shapr3d+0x182b1f7d0 and
  0x182b1f8f0), and the failing marshalling round happens inside that lazy init. SharedLibrary's
  marshaller matcher (sl+0x82d6e0) walks a STATIC compiled list (its getter sl+0x82d870) of registered
  interface marshallers, matching by signature — the runtime-created interface is not in the list, the
  app's blob cannot hold it (created at runtime), and the MissingInteropData throw follows. On Windows
  the same getter would fail identically IF reached — so the divergence is that raven's run reaches this
  getter's slow path (its cached object was initialized earlier on Windows, or an earlier branch
  differs). Also learned: the app's state written during the tolerated 120-second runs (telemetry.db,
  containers) poisons the NEXT startup — shapr.native's background init then fail-fasts early with
  E_INVALIDARG ("Value does not fall within the expected range") — so every tolerated run must be
  followed by a state reset (LocalState/LocalSettings moved aside and restored).
  The blob machinery decoded further: mrt100_app's module objects are tables of function pointers whose
  [+0x60] is the shared blob-finder (mrt100_app+0x50e20, returning [module+0x68]), and the blob region
  comes from the module header itself — offset header[0x44] (0x8306c8), size header[0x48] (0x94ec13),
  anchored at the header's self-pointer header[0xc0] — verified present in the app image. The region
  holds PACKED data (zero absolute pointers), so blob entries encode RVAs/offsets, not relocated
  pointers. Candidate packed pairs matching the failing type's RVA were found ({0x1b1c0, 0x5d0b0} and
  {0x1b1c0, 0x241230}), but the 2.x blob-id semantics are unverified (the ReflectionMapBlob enum fetched
  from current dotnet/runtime may not match the 2.x module format), so whether the TypeMap entry exists
  is UNCERTAIN. The relocation-delta hypothesis is DEAD: Shapr3D.exe (the 24KB .NET Native launcher) has
  no relocation table and always occupies 0x180000000, so Shapr3D.dll's relocation delta exists on
  Windows too (the experiment to bind the exe elsewhere failed — "PE image has no relocation table for
  the requested base" — and was reverted). The managed exception object's dump (added to
  RoOriginateLanguageException's handler) shows a small SharedLibrary MethodTable (sl+0x55fbe8 = the
  MissingInteropDataException class) with no captured stack trace.
  The 2.x blob-id table is now DECODED: the module's blob region (header[0x44]/[0x48]) is a sequence of
  tagged blobs {uint32 reserved; uint32 id; uint32 size; data}, walked by RhFindBlob's found-path
  (comparing entry+4 to the requested id, advancing by size+12). The app's region holds 27 blobs whose
  ids match the modern ReflectionMapBlob names: 1=TypeMap (107292 bytes), 2=ArrayMap, 9=FieldAccessMap,
  7=VirtualInvokeMap, 6=InvokeMap, 21/22=TypeTemplate/GenericMethodsTemplate, 13=EmbeddedMetadata (the
  0xDEADDFFD blob, 3455251 bytes), 26=StackTraceEmbeddedMetadata, 8=CommonFixupsTable, and newer ids
  30-36 — but **no 16=StructMarshallingStubMap and no 17=DelegateMarshallingStubMap**. The failing type
  has no entry in the TypeMap (its head is an ascending 16-bit bucket-offset table; no encoding of
  EETypeRva 0x1b1c0 appears), and the marshalling stub maps the runtime's fallback needs for a
  runtime-created type are absent from the app entirely — the compiled call sites carry their stubs
  directly (the MCG pattern), so the blob maps were never generated. A runtime-created generic
  instantiation therefore has no marshalling data anywhere, and the MissingInteropData throw is
  deterministic IF the round is reached. The complete story: OnLaunched's RootFrame lazy getter makes an
  interop call with a runtime-created interface instantiation; the fallback lookup fails exactly as it
  would on Windows; the divergence is that raven's run REACHES this call — the upstream branch in
  OnLaunched's managed decisions remains the one unknown, and identifying it needs a working-Windows
  capture of the same startup (no binary-only route remains).
  A correction to the failing type's kind: the modern nativeaot flag decode of 0x410c0000 reads
  ElementTypeMask (0x7C000000, shift 26) = **ValueType** (0x10), with HasPointersFlag, HasDispatchMap and
  IsDynamicTypeFlag set — the failing type is a RUNTIME-CREATED GENERIC STRUCT with GC pointers (the
  KeyValuePair/Linq-Grouping shape), not an interface. That matches the missing blob exactly: the app has
  no StructMarshallingStubMap (16), so a dynamically created struct instantiation has no marshalling data
  anywhere, and the MCG-compiled call sites carry their struct stubs directly — the fallback lookup fails
  deterministically. A runtime-created generic struct at startup points at reflection-driven generic code
  in the getter's path (Dictionary/Linq enumeration shapes — KeyValuePair, Grouping`2 — whose type
  arguments depend on earlier app state), which is where the Windows-vs-raven branch divergence lives.
  The failing round is now pinned to an exact position in the non-tolerated launch: the app's style and
  dictionary setup runs (ContentPresenter/Setter/DependencyProperty/PropertyMetadata activations), the
  attached-property registration DependencyProperty.Register("KeepFocusOnNavigate", …) REACHES raven and
  succeeds (the name created at raven-xaml+3c56e, no xaml_styles.c:688 note — that note only appears in
  the tolerated run where the zeroed marshalling product corrupts a later Register), the AutomationProperties
  statics activate (IID b618fd7b-32d0-4970-9c42-7c039ac7be78), AutomationProperties.SetAutomationId(frame,
  "RootFrame") SUCCEEDS (the raven-autoprops note answers 0), and the VERY NEXT managed interop call — with
  no raven-visible API between — needs the marshalling for the runtime-created generic struct
  (EETypeRva 0x1b1c0) and throws. The subagent's RAVEN_TOLERATE_MISSING_INTEROP diagnostic (patching the
  shared marshalling-failure helper sl+0x8621c0 and the factory sl+0x847fd0 to return null) moves execution
  past the round but the zeroed struct corrupts the next consumer (a Register with a null-Name TypeName →
  xaml_styles.c:688 E_INVALIDARG → InvalidCastException 80131534), so it is diagnostic only, not a fix.
  Also noted: the launch requires RAVEN_PREFIX=<repo>/components/shapr3d/prefix (without it the default
  prefix lacks SharedLibrary.dll and the process dies at load).
  A correction and a sharpening from a descriptor dump at the failing stub (RAVEN_STEP_DUMP2=rbx at
  shapr3d.dll+21790c8, anchored on the SetAutomationId note — OutputDebugStringA occurrence 19 in a fresh
  traced run, the wall following ~19 log lines later): the marshal driver's descriptor (arena 0x20042d0b8)
  is {sl+0x1fd40 (.rdata — an MCG marshaller RECORD, not code: a flags pair, a pointer, and ~40 code
  stubs — the per-signature marshaller class), zeros, the type handle 0x18002b1c0, sl+0xed970, a GC
  object}. The type handle 0x18002b1c0 = loaded_base(0x180010000) + 0x1b1c0 — the message's EETypeRva is
  LOAD-relative, and the EEType's FILE RVA is 0x2b1c0 — so the earlier flags decode ("ValueType, flags
  0x410c0000") read the WRONG cell (preferred-RVA 0x1c1c0, a different type) and is retracted. The real
  cell's content: two adjacent code pointers (0x182c62570/78, the .text tail), two data pointers
  (0x180136e2b9 .data, 0x1808281dc .rdata), then packed values — a 2.x structure whose layout is still
  unpinned. The MCG record at sl+0x1fd40 (flags 0x00200000/0x40, a pointer, ~40 code stubs) is the
  per-signature marshaller class the static list search failed to match.
  The type handle's cell (app RVA 0x8281dc, from the descriptor's 0x1808281dc pointer) sits inside a
  dense NativeFormat COMPRESSED token stream (recurring prefix-encoded handles: bytes "82 08", "01 00 22
  82", "08 81", "30 83" — the generated-reader encoding, not flat tables). Naming the failing type
  therefore needs an implementation of the NativeFormat compressed reader over the 0xDEADDFFD
  EmbeddedMetadata blob (type 13, file offset 0xc1fb61, 3455251 bytes) plus the handle-encoding rules —
  a bounded but standalone piece — or a working-Windows startup capture to diff the branch.
  The OnNavigatedTo null-read decoded (2026-11-14): the crash is `cmpb $0,(%rdi)` with rdi = NULL — the
  result of a call through the app's import cell 0x1370908 (the .data cells, not the earlier-guessed
  0x1360xxx addresses — the RAVEN_DUMP_CELLS anchor needed the corrected offsets and an occurrence-count
  suffix added to the diagnostic) — resolving to **mrt100_app!RhTypeCast_CheckCastClass**. The failing
  sequence: a managed get-type-by-name helper (a static cell + a 7-character string argument) resolves a
  type, `RhTypeCast_CheckCastClass(result, target-static)` returns NULL (the cast fails — the object is
  not of the looked-up type), and the MCG code checks the result's first byte without a null test. The
  target type's descriptor carries name hash 0x600a88de at +0x14 (the validated name-hash convention);
  naming it needs the metadata hash matcher rebuilt (the earlier parser was lost with the /tmp cleanup).
  The cell-resolution work also corrected the RAVEN_DUMP_CELLS anchor semantics (the cells resolve lazily,
  so the anchor must fire after the failing call's first use — the early anchors dump unfilled fixup
  content).
  The fault's own call sequence decoded (2026-11-14, from the disassembly around shapr3d.dll+2b1cbb0):
  the OnNavigatedTo code looks up a type by a 7-character managed string (a static cell + string + length 7
  through a resolve helper), then calls **mrt100_app!RhTypeCast_CheckCastClass(result, target-static)** (via
  the import cell shapr3d.dll+0x1370908 — the .data cells, correcting the earlier 0x1360xxx guesses), stores
  the result through a second helper into rsi+0x68, and the code's next `cmpb $0,(%rdi)` on the
  CheckCastClass result faults because **the cast returned NULL** — the object is not of the looked-up type.
  The MCG code has no null check. A same-block helper (sl+0x7e3460, rcx=ctx/rdx=value/r8=code) is a
  set-cached-value whose rdx NULL would take its own throw path — a different, untriggered failure. Naming
  the 7-character string requires capturing it at the helper call (the fault-time Rbx register was reused —
  it holds a 4.5MB-length non-string at the fault), i.e. a stepped capture anchored at shapr3d.dll+2b1cb68.
  The stepped capture of the 7-char string is blocked by run-to-run flow variance: fresh-state launches
  alternate between reaching the OnNavigatedTo cast (cd17) and fail-fasting at the codec marshaler wall
  (the FindAllAsync nested-generic NullReferenceException) before it. The RAVEN_STEP window anchored at
  the SetAutomationId note consumed its budget in mrt100 GC internals (a 40k-step window never reached
  shapr3d.dll+2b1cb68). The reference state remains cd18: with RAVEN_TOLERATE_FAILFAST=1 the app runs the
  full window — ShowWindow, the WelcomePage content building, one tolerated codec fail-fast. The two
  in-image walls (the nested-generic marshaler NRE; the OnNavigatedTo IPageOverrides cast) are documented
  as raven-side unfixable; the remaining routes are a working-Windows capture of the same startup, or
  naming the runtime-created struct via the 2.x MethodTable decode from mrt100_app's own consumers.
  Fresh-state launch state (2026-11-15, current tree): the app runs its ENTIRE startup with zero fail-fasts and exits cleanly (exit 3) — the codec marshaler wall and the marshalling-failure round are NOT reached in this flow; the XBF dictionary loads (all the theme/style XBFs), the KeepFocusOnNavigate Register succeeds, ShowWindow fires, the WelcomePage Navigate type name is created (the activation through IXamlMetadataProvider), and the remaining failure is the ThemedImage activation failing E_NOTIMPL (0x80004001 — the app's marshaler NotImplementedException, thrown inside the app's ctor path — the documented in-image gap), after which the app shuts down cleanly. The next surface: what the ThemedImage ctor's E_NOTIMPL round needed (the exception's method identity via the dump diagnostics) — implementing it lets the Navigate complete and the page content build.
  No Shapr3D window has been verified.
- Interop-wall anatomy and the stand-in-marshalling experiment (2026-11-06): three decisive
  decodings, then an env-gated diagnostic that moves the wall but cannot carry OnLaunched to
  completion. First, hypothesis A is answered: SharedLibrary.dll's blob region (header+0x44/0x48,
  the same 2.x layout) holds the SAME 27 tagged blob ids as the app — 1,2,9,7,6,23,21,22,14,15,10,5,
  19,18,8,26,27,13,24,25,30,32,34,35,31,33,36 — and NO 16=StructMarshallingStubMap and NO
  17=DelegateMarshallingStubMap. The framework module carries no marshalling stub maps either, so
  searching "the other module" was never a route: MCG-compiled marshalling is inline in code in both
  modules, and the runtime fallback for the failing type has nothing to find anywhere. Second, a
  launch-note correction: the launch requires RAVEN_PREFIX=<repo>/components/shapr3d/prefix (the doc's
  earlier command lines ran with it exported); without it raven resolves the prefix to
  ~/.local/share/raven/default, whose system32 has no SharedLibrary.dll, and the process dies at load
  with "missing PE module: sharedlibrary.dll" — an apparent regression that is only a missing
  environment variable. Third, the failing round is now decoded operationally end to end with two new
  raven diagnostics (both temporary, both env-gated): RAVEN_DUMP_CELLS=<api>/
  RAVEN_DUMP_CELLS_LIST="module:off,..." dumps resolved .NET Native indirect cells at a chosen API
  (the cells carry only fixup indices in the file and name their targets only at runtime), and it
  shows the app's interop cells resolve into SharedLibrary helpers — the failing frame shapr3d+0x2b1f8e1
  (the last call of wrapper 0x182b1f7d0) goes through cell shapr3d+0x136a568 to sl+0x8980a0, a
  SharedLibrary static-singleton getter (RhpCheckCctor2/RhpNewFast/flags 0x2000400 at +0x30), while the
  wrapper itself is the ExecutionContext/SynchronizationContext save-run-restore pattern around
  sl+0x7d7e30 (thread-static CurrentThread) — the MCG shape for marshalled calls with context capture.
  Single-stepping (RAVEN_STEP='CoGetApartmentType:788:N' — the round calls CoGetApartmentType as its
  last raven-visible act) shows the round: apartment query, a consult of shapr.native.dll, then the
  SharedLibrary interface-marshaller searches (the sl+0x864012 loop over the static list, entry matcher
  sl+0x82d8d0, equivalence test RhTypeCast_AreTypesEquivalent via cell sl:55c160), interop calls into
  raven's frame surfaces (IDependencyObject/IFrame5/IReferenceTracker queries all served, CoreDispatcher
  and Windows.Foundation.Metadata.Api* attribute objects among them), and finally the shared
  marshalling-failure helper sl+0x8621c0 building the "EETypeRva:0x0001B1C0 is missing interop type
  marshalling data" exception (message formatted at sl+0x8480b0/0x848190, thrown by the caller) —
  RoOriginateLanguageException(80131500) then RaiseFailFastException from sl+0x7e38de. The helper
  sl+0x8621c0 has 11 call sites across SharedLibrary, all marshalling failures, and a second factory
  sl+0x847fd0 builds the same exception for the compiled marshallers; both are only reached on failure.
  The diagnostic (RAVEN_TOLERATE_MISSING_INTEROP, in native/nt/providers.cpp at API dispatch, clearly
  marked temporary) patches both to "xor eax,eax; ret" in guest memory (protect_image RWX, write,
  restore RX, with expected-byte checks). With it the 80131500 wall is GONE: the app's OnLaunched round
  runs past the marshalling failure and reaches materially further — in the traced run it got to
  materializing the string literal "KeepFocusOnNavigate" and into raven's DependencyProperty.Register
  handler, which refused E_INVALIDARG at xaml_styles.c:688 (a TypeName argument with a null Name), and
  the app then fail-fasted with 80131534 (InvalidCastException). That is the honest mechanism of the
  stand-in: the failed marshalling had a STRUCT VALUE as its product, and returning null leaves the
  struct zeroed — the next call receives a corrupted argument (TypeName{Name:null}) — so the window
  pipeline (ShowWindow/Navigate/present) still never starts and OnLaunched still aborts, just one wall
  later and with a different exception. No raven-served surface was found returning wrong data that
  steers the app onto the failing path (the frame queries, IFrame5, dispatcher and metadata-attribute
  rounds in the round all return success before the failure); the divergence — why raven's run reaches
  this marshalling round at all, plausibly the DependencyProperty.Register("KeepFocusOnNavigate",…)
  path whose PropertyMetadata/struct argument needed the runtime-created generic instantiation —
  remains unidentified, and with a working-Windows capture still the only remaining identification
  route. The fixture set passes (16/16); the app state directories were moved to /tmp/shapr_state_backup
  before the experiments and the runs' rewritten LocalState/LocalSettings were deleted after each run;
  no Shapr3D window has been verified.

- Upstream merge continuation (2026-11-07): merged upstream main through 74e3e3e, retaining the XAML startup surfaces and upstream's metadata, memory and runtime additions. A launch using the merged runtime, a copied prefix and the freshly built XAML component again reaches the missing interop marshalling error `80131500` for `EETypeRva:0x0001B1C0`; no window was verified. The earlier application-view `E_ILLEGAL_METHOD_CALL` entry is superseded by the later interop-wall entries above. The exception diagnostic now catches guest memory faults when inspecting pointer-like fields, so an unreadable diagnostic pointer does not change the API result. Probe log: `/tmp/raven-merge-probe/launch.log`.
- Tolerated interop rerun (2026-11-07): after clearing only the copied prefix's app state, `RAVEN_TOLERATE_MISSING_INTEROP=1` passes the missing-marshalling throw and reaches the predicted next failure, `InvalidCastException` (`80131534`). The preceding failed call was carrying a required value struct; the stand-in returns a zero value, and the subsequent XAML registration sees a nameless `TypeName`. This confirms the tolerance hook cannot produce a valid launch. Log: `/tmp/raven-merge-probe/launch-tolerated.log`; copied state preserved under `/tmp/raven-merge-probe/LocalState.before` and `LocalSettings.before`.
- Live EEType probe (2026-11-07): added env-gated `RAVEN_DUMP_SHAPR_EETYPE` in `RaiseFailFastException` to inspect candidate addresses in Shapr3D's mapped image at the known fixed base. Both `Shapr3D.dll+0x1b1c0` (the error's literal RVA) and `+0x1c1c0` are pointer tables, not EEType headers: each 8-byte slot points to runtime data and interleaves self-relative descriptor addresses. This disproves the earlier claim that `+0x1c1c0` is the dynamic EEType and the modern MethodTable layout guess. Probe logs: `/tmp/raven-merge-probe/eetype-memory.log` and `eetype-1b1c0.log`. Next trace the `EETypeRva` formatting routine's base calculation and identify the referenced descriptor/table before interpreting fields.
- Corrected live EEType base (2026-11-07): the first probe used the Shapr3D.exe preferred base, not the separately loaded `Shapr3D.dll` base. `RAVEN_DUMP_SHAPR_EETYPE` now finds `shapr3d.dll` in the module set and reads from its actual loaded base. At `shapr3d.dll+0x1c1c0`, the live descriptor has flags `0x410c0000`, base size `0x18`, three vtable slots and three interface slots; this confirms the runtime-created GC-bearing value type from the error. The adjacent variable fields remain to be decoded against the 2.2 layout. `/tmp/raven-merge-probe/eetype-realbase.log` is the live evidence. `shapr3d.dll+0x1b1c0` is a separate table and was the misleading candidate.
- EEType field alignment (2026-11-07): the live probe now prints dwords, showing the exact boundary: flags at `+0x08`, base size at `+0x0c`, slot/interface counts at `+0x18`, and the `0x410c0000` flags. The dynamic tail after the interface map includes zero qwords at `+0x68` and `+0x70`, followed by dword `0xcb1a7b28` at `+0x74`; this is not yet safely interpretable as a template pointer. The probe also confirms `+0x1b1c0` is unrelated to the EEType. Log: `/tmp/raven-merge-probe/eetype-dwords.log`.
- Matching .NET Native runtime retry (2026-11-07): building MRT 2.2 had been blocked by four duplicate CRT definitions in `ucrtbase_vc2022.c`, despite its `_o___std_type_info_destroy_list` export being required by MRT 2.2. Consolidating on the existing legacy/Office definitions lets the CRT build and exports the missing symbol. With MRT 2.2.29722.0 and its matching `mrt100_app.dll` in the isolated Shapr3D prefix, the app reaches the same `0x80131500` missing struct-marshalling-data fail-fast and stack as MRT 1.6, ruling out the runtime-version mismatch as the cause. Log: `/tmp/raven-merge-probe/launch-mrt22.log`. No window was verified.
- Fresh rebuild and managed type-object trace (2026-11-07): rebuilding the current `raven` target and launching Shapr3D from the saved clean `LocalState`/`LocalSettings` reproduces the same `0x80131500` fail-fast at `sharedlibrary.dll+0x7e38de`, ending in `shapr3d.dll+0x2b210e2`. `RAVEN_TRACE_EXCEPTIONS` now dumps 0x100 bytes of the exception and 0x80 bytes for referenced objects, printing any recognized managed strings. The exception message remains `EETypeRva:0x0001B1C0`; the larger trace reaches the runtime type-manager metadata but has not yet identified a generic definition or safe marshalling path. Log: `/tmp/raven-merge-probe/launch-deep-trace.log`. No window was verified.
- Direct execution-breakpoint correction (2026-11-07): an isolated launch with a temporary hardware execution breakpoint at SharedLibrary+0x8621c0 showed many successful calls, including one whose argument is the managed string `inputDevice.usedPointerDevices` and the last whose argument is the already formatted missing-marshalling error message. Disassembly shows this helper converts managed strings; the earlier characterization of it as a marshalling-failure-only helper is incorrect. Breakpoints at +0x847fd0 and +0x8480b0 did not fire before the same 0x80131500 fail-fast in this launch, so those prior labels are also not established for this failure. The failure's origin and type remain unresolved; do not infer them from these helper addresses. Logs: `/tmp/raven-break-all.log`, `/tmp/raven-break-factory.log`, `/tmp/raven-break-format.log`. The breakpoint instrumentation was removed after the probe.
- Actual missing type and throw path (2026-11-07): a temporary hardware execution breakpoint at SharedLibrary+0x827fe0, identified by the direct reference to its `EETypeRva:0x` literal, captured the type pointer before its formatter subtracts the module base. For the failing message the pointer was `0x18002b1c0`, with Shapr3D.dll loaded at `0x180010000`: the type is at **Shapr3D.dll+0x1b1c0**, not +0x1c1c0. Its first qword was `0x0000001802000000`, followed by zero and `0xd0e95d2400000000`; the prior assertion that the failing type had flags `0x410c0000` and was runtime-created came from the wrong address and is withdrawn. In current NativeAOT's layout 0x02000000 is a generic flag and does not set the dynamic-type flag, but the exact .NET Native 2.x layout and type identity remain unverified. The captured caller stack reaches SharedLibrary+0x846a35; disassembly shows +0x8468f0 branches to +0x846a04 when its initial lookup (+0x85dcf0) returns null and its secondary path is unavailable or also returns null. That branch constructs and throws the missing-marshalling exception. This is the useful next trace point, rather than +0x8621c0. Logs: `/tmp/raven-eetype-all.log`, `/tmp/raven-eetype-real.log`. Temporary breakpoint code was removed. REA/Ghidra's xref request timed out during initial analysis; the literal's direct reference and caller were independently located in objdump, and the target session was closed.
- TypeMap and reflection-block verification (2026-11-07): decoded .NET Native's hash buckets and common-fixup RVAs against the actual type. The type hash at +0x14 is `0xd0e95d24`; TypeMap blob 1's bucket for that hash contains no entry with low hash byte `0x24`. Common-fixup index 129160 points to `Shapr3D.dll+0x1b1c0`, and blob 5's hash bucket resolves that same hash and index, which in CoreRT's corresponding format denotes a reflection-blocked type. Control: applying the same decoder to SharedLibrary's known string EEType at +0x11ed8 finds its hash, fixup index 465, and TypeMap metadata handle `0x3c0029db`. This validates the byte order, buckets and 2.x RVA-style fixups, though it does not identify the blocked type or prove that all CoreRT semantics exactly match this .NET Native build. Two direct code references to the failing EEType occur in an app stub at preferred VA 0x182bf33d6/0x182bf33e9; the stub fills an array of marshalled references after a call through app+0x1b56210, which is a candidate for further identification, not proof that this stub is the active failing caller. Probe: `/tmp/raven-typemap-probe.py`, control: `/tmp/raven-typemap-control.py`, launch log: `/tmp/raven-interop-api.log`. The actual 0x80131500 remains unchanged.
- Aggregated Frame identity and dependency activation (2026-11-07): the marshalling lookup captured a wrapper of the aggregated Frame with IID_IDependencyObject on its stack. Frame's dependency-object face previously acquired the inner reference but released the outer reference, and its QueryInterface returned the inner identity. The face now delegates identity and uses the controlling outer's count when aggregated, like the other public Frame interfaces. The x64 and x86 control fixtures check both identity and balanced references. With that fix, Shapr3D passes the earlier missing-marshalling exception and sets the RootFrame automation name, then requests `Microsoft.UI.Xaml.Controls.BackdropMaterial` from the declared Microsoft.UI.Xaml.2.8 dependency. Package activation now searches declared dependency manifests in the installed dependency layouts after the app manifest, checks identity name, publisher and minimum version, and uses the matched server's relative path. The x64 and x86 package fixtures cover successful dependency activation and reject wrong publishers and too-old versions. All four fixtures returned zero directly and passed through CTest (4/4); their CTest limits were increased to 120 seconds because isolated prefix provisioning exceeded 20 seconds. The x86 component directory needed the already-built 32-bit shell and user32 components because this build was configured without RAVEN_CONTROLS32_COMPONENT_DIR.
  A fresh isolated Shapr3D launch loads the actual WinUI dependency and then hits a null read in Microsoft.UI.Xaml.dll+0x105003. The immediately preceding trace shows that the WinUI call asked the Frame for IControl (IID a8912263-2951-4f58-a9c5-5a134eaa7f07), which Frame does not yet expose; the faulting WinUI code tries to query the resulting null object for IDependencyObject. This is the next concrete implementation gap, not a verified window. Logs: `/tmp/raven-frame-fix-api.log`, `/tmp/raven-package-graph-launch.log`, `/tmp/raven-winui-fault.log`.
- Shapr3D WinUI startup continuation: Frame now exposes IControl and IFrameworkElement6 with aggregation identity, and stores local dependency values and raises property-change callbacks. DependencyObject has a composable factory, and Window3 returns Raven's compositor. UISettings supports weak references and AccessibilitySettings activates with a fixed light/high-contrast-off answer. Theme inheritance, theme-change notifications, and Frame's Control properties remain incomplete. The x64/x86 controls fixtures and direct x64 UISettings fixture pass. A copied-prefix launch passes these earlier failures and reaches an unregistered `Windows.UI.Xaml.Media.SolidColorBrush` factory (`ISolidColorBrushFactory`, IID d935ce0c-86f5-4da6-8a27-b1619ef7f92b); it exits with `80040154` before a window is verified. Log: `/tmp/raven-accessibility-launch.log`.
- Further startup continuation: Windows SolidColorBrush now activates through its color factory and round-trips Color; Frame retains its Background and exposes IFrameworkElement Name and event registrations. The style and controls fixtures passed on x64 and x86. Frame does not yet paint Background, raise the registered FrameworkElement events, or implement most FrameworkElement properties. The next traced failure is a request for `Windows.UI.Xaml.UIElement` / `IUIElementStatics` (IID 58d3573b-f52c-45be-988b-a5869564873c); Frame also refuses IUIElement. This remains before a verified Shapr3D window. Logs: `/tmp/raven-framework-events-launch.log`, `/tmp/raven-framework-factory.log`.
- UIElement continuation: Frame now shares the existing UIElement pointer-event part, with aggregation identity, and UIElement statics return stable RoutedEvent identities. AddHandler and RemoveHandler connect supported pointer events to the input router; key-down/up handlers can be retained, but keyboard focus and event delivery remain unimplemented, as does handledEventsToo. The x64/x86 controls fixtures pass; x64/x86 input fixtures pass with a real pointer message delivered through AddHandler and revoked through RemoveHandler. A copied-prefix launch passes the UIElement activation and handler registrations, reaches settings initialization, and fails with E_BOUNDS from ApplicationDataContainerSettings.Remove on a missing key. Its correctness for this particular Windows class remains unverified, so the return was not changed. Log: `/tmp/raven-key-registration-launch.log`. No Shapr3D window has been verified.
- Settings and display continuation: the absent removed key was the optional `com.shapr3d.shapr.userDefaults.features.lagrangeSolverEnabled`, which the app removes without a presence check. The settings container now treats a missing removal as a no-op, inferred from this app behavior; direct Windows verification is still needed. The focused empty-store/no-notification checks passed on x64 and x86 in an isolated prepared prefix. The full cross-process CTest runs did not pass: x32 timed out during prefix provisioning and x64 hit its overall timeout.
  DisplayInformation now exposes current-view logical DPI, scale, nullable physical diagonal and primary-screen pixel dimensions, and Frame exposes ContentControl backed by its existing content slot. The x64/x86 window and controls fixtures passed. Raw DPI currently follows logical DPI; per-monitor dimensions, display-change delivery and keyboard focus/routing are incomplete. Shapr3D now reaches managed task setup and fails with `80131509`: `The current SynchronizationContext may not be used as a TaskScheduler.` A trace shows the main thread called RoInitialize(1), joining the MTA; whether and how the .NET Native UI synchronization context should be established remains unresolved. Logs: `/tmp/raven-content-launch.log`, `/tmp/raven-sync-trace.log`. No usable Shapr3D window has been verified.
- 32-bit installer components and the configure blocker (2026-11-10): `ninja -C build` failed at CMake configure on
  "Configured installer component does not exist: components/installer32/mspatcha.dll" — the interim session added
  mspatcha.dll to the installer component list and built the 64-bit one, and the configure check walks both widths.
  The 32-bit build mirrors the installer64 provenance commands (compile api.c/lzxd.c/pa19.c with
  `-O2 -std=c11 -fno-builtin -D_WIN32_WINNT=0x600 -DUNICODE`, link `-shared -nostdlib --entry,DllMain` with
  mspatcha.def and -lmsvcrt -lkernel32 -lgcc) with i686-w64-mingw32-gcc; this machine's mingw gcc driver (repackaged
  as the ZCode AppImage) cannot find its own cc1 or assembler, so the builds add `-B /usr/i686-w64-mingw32/bin/
  -B /usr/lib/gcc/i686-w64-mingw32/16.2.0/` and explicit `-isystem` include paths — a workaround recorded in
  installer32/provenance.json, not a source change. The configure then uncovered four more missing 32-bit
  components (vbscript.dll, scrrun.dll, wshom.ocx, jscript.dll — Raven-own, added to the component list after the
  last x86 provisioning run); all four were built for x86 mirroring scripts/provision-installer.py's recipe
  (per-source compiles, stdcall-decorated PRIVATE def entries, `-Wl,--kill-at`, entry `_DllMain@12`), and the
  provenance records the mirror. `ninja -C build` configures and completes cleanly.
- The SynchronizationContext wall passed by fixing RoInitialize's apartment mapping (2026-11-10): the doc's last
  entry recorded `80131509` "The current SynchronizationContext may not be used as a TaskScheduler." and the
  observation "the main thread called RoInitialize(1), joining the MTA". The observation was the bug: RO_INIT_SINGLETHREADED
  is 1 and RO_INIT_MULTITHREADED is 0, so RoInitialize(1) on Windows makes the main thread the (main) STA, while
  raven's mapping passed `call.u32(0) == 1 ? 4 : 6` — joining the MTA. The mapping is corrected to `1 ? 6 : 4`
  (platform_api.cpp), the main thread becomes the main STA, CoGetApartmentType answers it as such, and the launch
  runs past the missing-SynchronizationContext throw entirely — the app's settings initialization, telemetry,
  sqlite user cache and background tasks all proceed.
- SuppressNavigationTransitionInfo activation and the ShowWindow milestone (2026-11-10): the next wall was the
  app building `Windows.UI.Xaml.Media.Animation.SuppressNavigationTransitionInfo` for Frame.Navigate and dying with
  `80040154` ("Class not registered", raven-shell's `winrt: no class` trace) because neither the runtime catalog nor
  the app manifest declared it. The class is now in the registry catalog (registry.cpp, raven-xaml.dll) and
  raven_xaml_controls_factory serves an activatable instance answering IUnknown/IInspectable, IAgileObject (the
  engine's objects are apartment-free) and IWeakReferenceSource. A new fixture (testdata/native_loader/
  suppress_transition_client.c, native_suppress_transition_x64) drives RoActivateInstance by name through combase and
  checks the identity, the weak-source round trip (called through the Windows ABI layout — mingw's weakreference.h
  models the interfaces as IUnknown-only, but the component implements the IInspectable-derived seven-slot tables)
  and the refused class casts. The launch's cast walk asked two further interfaces of the instance — the class
  marker 11d3b13a-180e-4789-a8be-7712882893e6 and a runtime-computed second face a9b05091-ae4a-4372-8625-21b7a8b98ca4 —
  and granting either led the SharedLibrary cast machinery into an `IInspectable::GetIids` call whose out pointer is
  not a pointer any live implementation can accept (it faults, and after an honest E_POINTER the driver itself
  faults), so both casts are refused with a comment pinning the evidence; the refused cast is the behavior the
  launch tolerates and runs past. A defensive plausibility check in raven_xaml_element_pointer/raven_xaml_element_bubble
  (temporary diagnosis, traced with raven_trace_note) skips delivery into a recorded element whose IUIElement face is
  implausible — the freed-element delivery the earlier runs crashed in; the guard has not fired since the apartment
  fix, so the underlying use-after-free, if it still exists, is unobserved.
  With those, the launch completes its whole window: the app shows its window (ShowWindow(hwnd, SW_SHOW) from
  raven-xaml), pumps messages (PeekMessageW/TranslateMessageW/DispatchMessageW, WM_NCHITTEST and mouse traffic on
  the shown window), navigates the RootFrame — the Navigate call carries "Shapr3D.App.WelcomePage, Shapr3D, ..." and
  raven's frame answers "frame navigate found no page" (materializing a managed page type through the app's
  IXamlMetadataProvider is the next surface, not yet implemented) — initializes its renderer (D3D11/DXVK device
  creation), writes its state (LocalState telemetry.db/user.db/projects, LocalSettings containers), and runs the
  full 60-second trace window with no exception before a clean teardown. Logs: /tmp/raven-launch-1.log through
  /tmp/raven-launch-9.log and /tmp/raven-launch-final.log. Still not verified: an actual visible Shapr3D UI (the
  window is shown and receives input, but no XAML content page exists inside it yet), and the fixture suite
  (18/18 including the new one) passes with all changes uncommitted.
  Independent verification (2026-11-10): a fresh-state launch of the current tree shows the window —
  ShowWindow(hwnd, SW_SHOW) from raven-xaml.dll — with zero fail-fasts, only the two benign early
  80040154 background throws, the RootFrame Navigate attempt ("found no page" — managed page
  materialization through IXamlMetadataProvider is the next surface), and a clean shutdown (exit code 3
  after teardown). The apartment mapping fix is the deepest single fix of the Shapr3D effort: the app ran
  in the MTA for every session since the first launch, and the marshalling-wall investigation chased
  consequences of that (apartment-dependent marshalling lookups, timing, and init order) before the
  swapped constant was found.
- Frame.Navigate resolves application pages through the app's IXamlMetadataProvider, and WelcomePage materializes (2026-11-10): the
  "frame navigate found no page" wall is gone and the page construction now runs deep into the app's real content. Four fixes and
  their evidence:
  - The metadata-provider path (`create_from_provider` in xaml_metadata.c) existed but called only the GetXamlType(String) overload
    with the assembly-qualified name the Navigate carried, which the generated XamlTypeInfo map (keyed by plain full names) never
    answers. The path now tries, in order: GetXamlType(TypeName) with the caller's TypeName unchanged (the Windows-framework form;
    the managed thunk turns the struct back into System.Type, and it resolves), GetXamlType(String) with the full name, and
    GetXamlType(String) with the plain name before the assembly qualifier. Trace notes record each stage; the launch log shows the
    full-name overload answering no type and the plain-name overload answering the WelcomePage type. The frame's Navigate falls back
    to this after the runtime-catalog walk (`raven_xaml_create_from_type`, xaml_internal.h).
  - The managed page constructor composes with the CCW as the outer, and `page_create` ignored it (inner=NULL) — the .NET Native
    projection then died on a null interface call inside the constructor (c0000005 → NullReferenceException). Page now aggregates
    exactly like the frame: outer retained, a non-delegating inner part handed out, and every page face (IPage, IContentControl)
    delegating identity and lifetime to the outer when composed.
  - The compiled WelcomePage.xbf (decoded from resources.pri blob 223, scripts/xbf-walk equivalent in /tmp) opens with a
    SetConnectionId-shaped kind-12 constant node and binds its root by the stable Page entry while the instance is the derived
    class. `use_target` now binds a stable-525 (Page) reference through the target's IPage face — the managed CCW's
    GetRuntimeClassName is unusable for binding (it answers "Windows.UI.Xaml.IDependencyObject") — and kind-12/14 constants skip
    under a target load. The document then builds two DeferredElement subtrees (stable 746, CustomRuntimeData type 9, content in
    substreams 1 and 3) appended to the Grid's Children through the panel's live IVector<UIElement> (kind 19 now serves
    Children/RowDefinitions/ColumnDefinitions and the generalized append drives the parameterized vectors), and Content is applied
    at the page-scope pop from the object the Children scope left in last_object.
  - The WinUI dependency's ProgressRing composition needed the base class: "Windows.UI.Xaml.Controls.Control" is registered
    (native/nt/registry.cpp) and raven_xaml_controls_factory serves it with IControlFactory (composable CreateInstance over the
    frame machinery with a standalone_control flag, the object named Windows.UI.Xaml.Controls.Control) and IControlStatics — twenty
    real dependency properties registered through a new raven_xaml_property_register export (xaml_styles.c) and cached per name.
  The launch evidence: provider QI succeeds; the plain-name overload returns the type; ActivateInstance runs the app's page
  constructor; InitializeComponent loads ms-appx:///App/WelcomePage.xaml and the loader serves Button, TextBlock, Grid activations;
  the WinUI dependency activates Microsoft.UI.Xaml.Controls.ProgressRing and registers its IndeterminateSource/DeterminateSource
  properties through raven's DependencyProperty store. The remaining wall is inside Microsoft.UI.Xaml.dll itself: c0000005 at
  microsoft.ui.xaml.dll+25919 (reading 0) in its own code shortly after its DXAML layer registers ProgressRing properties through
  raven — whether it is reached from the deferred subtree or the page's direct construction is not pinned (the
  RAVEN_XAML_SKIP_DEFERRED_CONTENT placeholder did not move it, so the deferred-subtree attribution is unproven). This is
  WinUI-internals emulation, not the navigation mechanism. The Navigated event is now stored per frame and
  raised after a completed navigation with a synthesized INavigationEventArgs (the hand-declared ABI), followed by the page's
  IPageOverrides::OnNavigatedTo; raven_xaml_page_set_frame no longer leaks a frame reference for managed page faces. Fixture suite
  green (the 12 required plus the 12 xaml/frame x64+x32 fixtures). Logs: /tmp/raven-provider-probe/launch-1.log through
  launch-10.log. App state was rewritten during the runs and moved to /tmp/raven-provider-probe/state-after; no Shapr3D UI has been
  verified.
- The microsoft.ui.xaml.dll+25919 null-interface wall is passed, and the launch runs the whole 60-second window with zero
  exceptions (2026-11-10). The diagnosis started by falsifying the earlier stack scan: raven-xaml.dll+78fb8 is not code — it
  is the .rdata address of a "Microsoft.UI.Xaml.Controls" UTF-16 literal, and the two "+78fb8 frames" were that string pointer
  sitting in registers/stack (r12 held it), not calls into raven. The real chain, decoded from Microsoft.UI.Xaml.dll's own
  disassembly plus a RAVEN_STEP run anchored on the last raven-visible API before the fault: the app's managed WelcomePage
  construction creates a WinUI 2.8 DXAML core object (microsoft.ui.xaml.dll+391480 → +3910b0 → the 0x118-byte object ctor
  +39a69c); the ctor's ensure path stores the CoreServices-held peer face into the object's +0x60 member, and that peer face
  is raven's frame — the frame's non-delegating inner (its vtable in the step dump: frame_inner_table), registered into
  WinUI's CoreServices singleton earlier in the run. The ctor then queries that face for two Microsoft.UI.Xaml.dll-private
  interfaces without checking the QI results: 1791587a-2a7c-4981-a2d8-ab99fffc65b9 at +1695ae (refused by frame_query_any →
  the refused out pointer dereferenced at +25919 as a vtable fetch) and, one fix later, 29fed85d-3d22-43a1-add0-17027c08b212
  at +3827aa (same unchecked pattern, generated accessor). The bridge's second method is called with the control's class
  name ("Microsoft.UI.Xaml.Controls.ProgressRing", length 39 in the step registers) boxed as IReference<String>; the
  PropertyValue activation raven serves immediately before the fault is that boxing.
  The fix treats those asks as what they are on Windows — the OS XAML peer's private bridge to the WinUI 2.8 DXAML layer —
  and implements both faces on the engine's frame (xaml_controls.c: frame_peer_bridge / frame_peer_shim parts, answered from
  frame_query_any). The bridge's second method accepts the delivered IReference<HSTRING>, traces the class name, and answers
  S_OK (the caller HRESULT-checks it); its first method has no observed caller and answers E_NOTIMPL with a note. The shim's
  first method takes the two framework context objects the generated accessors resolved plus an out whose value the traced
  construction ignores, and answers S_OK with the out unset. The third ask the crash path makes (5ebb51d5-9e58-49b7-bc2d-
  0155ff118664, queried on the bridge face) is HRESULT-checked by WinUI and is still refused — the tolerated refusal.
  The next wall was raven's own: append_dictionary (xaml_loader.c) declared the IVector`1 QI out as a vtbl-typed pointer and
  called `vector2->Append(...)` off the object rather than through the face's lpVtbl — an out-of-bounds read past the engine's
  56-byte children_vector into heap reuse, the wild call that landed (depending on the run) inside a __func__ string of
  xaml_elements.c, at address 0, or at raven-xaml.dll+6910a. The call now goes through the face's lpVtbl, and the append path
  gained the same freed-collection plausibility guard the element delivery already carries (the face's vtable must resolve
  into raven-xaml.dll's image via VirtualQuery/AllocationBase).
  Launch evidence under headless Weston (wayland-99), fresh state: exit code 3 after the full 60-second trace window, zero
  c0000005 and zero fail-fasts (reproduced on a second fresh-state run), ShowWindow(SW_SHOW) from raven-xaml.dll, the
  WelcomePage content tree building further than any earlier run (18 TextBlock, 12 Grid, 10 Button, 10 RowDefinition, 12
  Style, 8 ControlTemplate, 20 ResourceDictionary activations), the WinUI dependency's ProgressRing, BackdropMaterial and
  XamlControlsXamlMetaDataProvider activating, and the DXVK/D3D11 renderer initializing with a render thread. No rendered
  XAML pixels are verified yet: the next wall is renderer-level — DXVK's dxgi answers
  `CreateSwapChainForComposition: Not implemented` (logged four times), so the app's composition swapchain is refused and
  nothing reaches the virtual display. The required fixture suite passes 14/14 with the changes uncommitted. Logs:
  /tmp/raven-shapr-probe/vd13-fix4.log and vd14-verify.log (clean runs), vd6-fix1/vd8-fix2 (the intermediate walls),
  vd3/vd5/vd7/vd11 step captures, mux.dis/rx*.dis disassemblies. App state from the runs moved to /tmp/raven-shapr-probe/
  st1*-LS. What remains honest caveats: the peer bridge's first method and the shim's second method have no observed callers,
  so their behavior beyond the answered QI is unexercised; the CoreServices registration that made raven's frame the WinUI
  peer was traced only to its effect (the frame inner inside WinUI's singleton), not to the registering call; and the
  freed-collection guard has not fired since the lpVtbl fix, so the append-path use-after-free, if it still exists behind
  correct calling conventions, is unobserved.
- The composition swapchain wall, first layer passed: `CreateSwapChainForComposition` now succeeds and a GPU
  swapchain is created against the virtual display, though the app does not present yet (2026-11-10). The
  "CreateSwapChainForComposition: Not implemented" refusal in the last entry is not Raven's — DXVK's dxgi.dll
  (the guest DXGI component, v3.1.1) refuses the call unless the build option `dxgi.enableDummyCompositionSwapchain`
  is set (dxgi_factory.cpp gates on it and then runs the normal `CreateSwapChainBase` with a null HWND; its
  `DxgiSurfaceFactory` makes a real `DXVKDUMMYWINDOW` through RegisterClassExW/CreateWindowW). The option reaches
  DXVK through the guest environment, which carries only Raven's own defaults plus host variables named
  `RAVEN_GUEST_<NAME>` (default_environment in system_api.cpp — `DXVK_CONFIG` itself does not pass through), so
  launches now set `RAVEN_GUEST_DXVK_CONFIG='dxgi.enableDummyCompositionSwapchain=True'`.
  How the stack works, as traced: Shapr3D's renderer thread creates a D3D12 device through vkd3d-proton
  (d3d12core.dll) and asks DXVK's dxgi for a composition swapchain on it; DXVK queries the device for its private
  `IDXGIVkSwapChainFactory` (vkd3d-proton implements it, GUID present in both binaries) and the vkd3d presenter
  creates the surface by calling `vkCreateWin32SurfaceKHR` with the dummy HWND; Raven's synthetic vulkan-1.dll
  (exports are traps into `VulkanBridge`) translates that to `vkCreateWaylandSurfaceKHR` over
  `windows.host_surface(window, hidden=true)`, and its swapchain-capability interception overrides the extents with
  the window's client size. Two Raven-side findings and fixes sit under that:
  - The virtual display itself could not receive GPU frames from any driver: headless Weston with the pixman
    renderer advertises no `zwp_linux_dmabuf_v1`, and Mesa's Wayland WSI binds only dmabuf for hardware devices
    (registry_handle_global: wl_shm is bound only when `sw`), so `wsi_wl_display_init` fails and every surface
    formats/capabilities query of a GPU driver answers `VK_ERROR_SURFACE_LOST` (0xC4653600 — the value that sat
    unexplained in the trace). Reproduced standalone on a roleless *and* on a configured-unmapped xdg toplevel
    (/tmp/wl-roleless-test), and fixed by restarting Weston with `--renderer=gl` (`weston --backend=
    headless-backend.so --renderer=gl --socket=wayland-99 --width=1920 --height=1080`), which advertises dmabuf v5
    and after which RADV and NVIDIA both answer 14/24 surface formats.
  - A driver surface whose window has no presented host window used to be a roleless wl_surface ("without a role
    while the root has no host window"); it now keeps an unmapped xdg toplevel of its own (`Hosted::own_shell`,
    native/desktop/wayland.cpp), configured before `host_surface` returns so the driver may attach its first buffer
    legally, and dropped again when a presented host window appears to attach to. This is what carries the
    composition swapchain's dummy-window surface. Traces of both (role creation, configure arrival) are gated on
    `RAVEN_TRACE_API`.
  Launch evidence (fresh state, exit 3 after the full window, zero c0000005, window shown): DXVK logs "Creating
  dummy swap chain", creates DXVKDUMMYWINDOW through Raven's user32, `vkCreateWin32SurfaceKHR` returns VK_SUCCESS,
  surface support is answered, and vkd3d-proton's presenter runs its whole init ("Creating swapchain (%u x %u),
  BufferCount = %u", user buffers, sync objects, waiter thread, low latency, frame limiter); DXVK's
  `DxgiSwapChain` constructor then runs its color-space support query — four surface-format queries, all
  VK_SUCCESS — and the `DestroyWindow` of the dummy that follows is `CreateSwapChainBase`'s own surface-factory
  destructor on the success path (the Com<> local leaves scope after the frontend swapchain is built; the VkSurface
  stays with the vkd3d chain). Shapr3D_dxgi.log records no failure HRESULT, so the composition swapchain is handed
  to the app with S_OK, twice (two creation rounds).
  What is still not verified, honestly: the app never presents — no `vkAcquireNextImageKHR`/`vkQueuePresentKHR`
  call is traced, and the vkd3d swapchain is torn down (vkDestroySurfaceKHR plus a null vkDestroySwapchainKHR)
  about two thousand trace lines after creation, after the app stringified the adapter description; thread 162
  ends with the tolerated 80040154s. The next wall is therefore not creation but attachment and presentation:
  whichever WinUI 2.8 DXAML bridge would bind the swapchain into the OS XAML peer (the refused frame interfaces
  `ae2d9271-3b4a-45fc-8468-f7949548f4d5`, asked 82 times in this launch, alongside the known `fd104460`/`280335ae`
  refusals) and Raven's own composition side, where `xaml_composition.c` refuses surface brushes and
  `swapchain_panel.h` refuses `ISwapChainPanelNative::SetSwapChain`, are unexercised by any present. A missing
  `Windows.UI.Xaml.Controls.UserControl` activation (winrt: no class) surfaced during the swapchain window and is
  one candidate content gap. Logs: /tmp/raven-comp-probe/comp1.log (env without RAVEN_GUEST_ did nothing),
  comp2.log (dummy chain created, roleless surface refused as SURFACE_LOST on pixman Weston), comp5.log (role
  configured, still SURFACE_LOST — pinned the compositor as the cause), comp6.log (GL Weston, full success path).
  The required fixture suite passes 14/14, as do native_wayland_popup_focus, native_composition_x64/_x32,
  native_d3d11_triangle and native_d3d12_draw_gpu (that one renders offscreen, so the present path itself is
  verified only through the queries above).
- The broken build, the fixture regression, and the 82 ITextBlock asks resolved (2026-11-11): the session opened on a tree
  that did not build. `data_writer.c` declared the CryptographicBuffer statics typedef against the struct tag
  `__x_ABI_CWindows_CSecurity_CCryptography_CICryptographicBufferStatics` (an extra `C` — `CSecurity`) while the interface
  body below it declared `__x_ABI_CWindows_Security_...`: two distinct tags, so the typedef's struct was never completed and
  line 2246's `static cryptobuffer_statics_i cryptobuffer_statics_instance` had an incomplete type. The fix is one character
  — the typedef now aliases the tag the interface body actually declares. Separately, the fixture suite failed
  `native_xaml_controls_x64` with a c0000005 at raven-xaml.dll+274c8: the previous session's "temporary unconditional"
  frame-QI diagnostic had `if (false && !GetEnvironmentVariableA(...))`, which defeated its own gate, so every refusal
  executed `__builtin_return_address(1)` — whose generated code walks the caller's saved-rbp chain — and the fixture's
  mingw callers are built without frame pointers, so the walk dereferenced garbage (fault address 0x10 = garbage frame + 8).
  The gate is restored (the fixture run never reaches the walk) and the lookup is cached in a static; the Shapr3D launches
  pass `RAVEN_TRACE_FRAME_QI=1` plus `RAVEN_GUEST_RAVEN_TRACE_FRAME_QI=1` (the guest environment only carries
  `RAVEN_GUEST_<NAME>` variables). The diagnostic itself was upgraded to print module file names instead of an anonymous
  `mod+offset` (GetModuleFileNameA resolved off RtlPcToFileHeader's base), which is load-bearing for the identification
  below; the `line` buffer grew to 256 to hold the longer text. The fixture suite passes 14/14 with these changes.
  The diagnosis, then: the 82 ITextBlock asks are NOT misrouted TextBlock objects. With module names visible the asker is
  SharedLibrary.dll+82c9a4 — the .NET Native runtime in the guest — and every one of the 328 refused asks (ITextBlock 82,
  IGrid 82, IButton 164) lands on the SAME object: the real Frame adapter (kind d0 s0, one outer pointer for the whole
  run). All four IIDs share one via offset, so it is a single variable-IID call site: the .NET Native projected-type
  resolution probe, which QIs an object crossing into managed code against a table of candidate interfaces. Each burst
  refuses IGrid, IButton twice, ITextBlock, then answers IFrame and IContentControl — the probe finds the Frame type and
  continues; bursts correlate one-to-one with element handoffs (they follow TextBlock, Button and SolidColorBrush
  activations in the trace). The frame's E_NOINTERFACE is therefore the honest and correct answer — the object genuinely
  is not a TextBlock — and no routing fix was made: answering ITextBlock on the Frame would be dishonest. The second,
  smaller asker shape (14 asks) comes from Microsoft.UI.Xaml.dll+15f157, reached from raven's own
  raven_xaml_element_template_child and raven_xaml_element_render_info, probing the standalone Control adapters for
  IGrid/IButton; it stays a tolerated refusal. The remaining "no class" set this run: HttpBufferContent (asked twice, the
  app throws and catches a C++ exception each time), StoreContext, AsyncCausalityTracer, CBSPackageDetector, and
  SystemDiagnosticInfo (asked once near the end of the window; the exception on the asking thread precedes the orderly
  teardown and status 3, the same ending as earlier runs). Launch evidence on a fresh state: exit 3 at the 60-second
  window, zero c0000005, window shown, and the previous session's CryptographicBuffer/HashAlgorithmProvider increment
  demonstrably served (CryptographicBuffer activated 4 times, HashAlgorithmProvider 2), plus UserControl — the "missing
  Windows.UI.Xaml.Controls.UserControl activation" of the last entry — now served, and the HttpClient stack
  (HttpClient/HttpRequestMessage/HttpMethod/HttpBaseProtocolFilter) activating. One environment finding cost a rebuild
  round: the inherited wayland-99 compositor had been started WITHOUT `--renderer=gl` (pixman headless), and every
  vkGetPhysicalDeviceSurfaceFormatsKHR answered VK_ERROR_SURFACE_LOST (0xC4653600, twelve hits) — the documented pixman
  failure mode, in a run that otherwise looked healthy. Restarting weston with `--renderer=gl` (GL renderer, NVIDIA EGL)
  took the count to zero, after which the run renders for real: 380 vkCmd submissions from the D3D11/D3D12 devices, two
  full vkd3d composition swapchain inits (sync objects, latency handles, KHR_present_wait negotiation) and DXVK's d3d11
  resolving vkAcquireNextImageKHR/vkQueuePresentKHR into its WSI. The wall is unchanged and now measured precisely: the
  present functions are resolved but never called — no vkAcquireNextImageKHR( or vkQueuePresentKHR( call appears — so the
  WinUI 2.8 DXAML bridge binding the composition swapchain into the peer (raven's swapchain_panel SetSwapChain refusal and
  xaml_composition.c's surface-brush refusal remain unexercised) is still the gate between rendering and pixels. Logs:
  /tmp/raven-shapr-probe/launch-qi3.log (pixman Weston, healthy run ending in SURFACE_LOST format queries),
  launch-qi4.log (named-module diagnostic), launch-qi5.log and launch-qi6.log (GL Weston, clean), weston-gl.log. App
  state from the runs moved to /tmp/raven-shapr-probe/st20..st23-*. The diagnostic stays env-gated and marked TEMPORARY
  for removal once the element-interface ask picture is no longer needed.
- The swapchain ATTACHMENT path, layer one: SetSwapChain stores, the Control scalars store, and one XBF gate passed —
  the app now walks into new code and dies on a named wall instead of limping to the timeout (2026-11-11). The
  `ISwapChainPanelNative::SetSwapChain` refusal is replaced with honest storage: the panel keeps the composition
  swapchain reference (AddRef/Release like the internal contract storage) and traces attach/detach. Presentation rides
  the chain's own path — the caller's Present runs the DXVK/vkd3d presenter over the DXVKDUMMYWINDOW hwnd, whose raven
  host surface (the unmapped xdg toplevel the docs' composition entry created) carries whatever frames the caller
  presents; the panel composes no pixels itself and still exposes no IUIElement/IPanel. Who attaches is now known:
  Shapr.Native.dll (the app's renderer) is the only guest module referencing ISwapChainPanelNative's IID, and no
  SwapChainPanel has ever been activated (zero `winrt: activation Windows.UI.Xaml.Controls.SwapChainPanel`, zero QIs for
  f92f19d2 in any run) — the panel is missing, not the wiring after it. What gates the panel's existence: the app's
  control XBFs. The managed ActivateInstance diagnostic (now carrying the type name) and the ms-appx string in the trace
  pinned the first of three XBF failures to `Shapr3D.UILibrary.TitleBarControl` and `Shapr3D.UILibrary.ThemedImage`
  throwing NotImplementedException out of their constructors because the constructor's InitializeComponent loads the
  control's own XBF (`ms-appx:///Shapr3D.UILibrary/TitleBarControl.xaml`) through raven's loader, and the loader failed
  it. Two raven-side causes were fixed. First: the projected IControl face was ~40 E_NOTIMPL stubs — any
  Control-derived constructor that set FontSize/IsEnabled/IsTabStop/TabIndex/FontWeight/FontFamily/Foreground/
  Template/BorderBrush or the alignment enums threw. They now store honestly over the frame's own fields with the UWP
  defaults (FontSize 14 float-bits per the ABI convention the TextBlock accessors already decode, IsEnabled true,
  IsTabStop false, TabNavigation Once, alignments centered), the object-typed members hold references like Background,
  FocusState answers Unfocused, and the members that would have to lie (the never-firing IsEnabledChanged subscription,
  ApplyTemplate, Focus) stay E_NOTIMPL. Second: `raven_xaml_element_set_object`'s own contract says S_FALSE means "the
  element has no such property, the caller reads it as the skip", but five loader assignment sites treated any non-S_OK
  as a document-fatal E_NOTIMPL — so one application-defined property (something TitleBarControl/ThemedImage set that
  the engine does not model) killed the whole XBF. Only a FAILED status is fatal now; S_FALSE skips, the way the
  without-target walk already behaved. Result: XBF failures 6 → 2 (offset 0xe2c's document now loads completely), the
  surviving failures are TitleBarControl/ThemedImage activations whose constructors still throw — the remaining refused
  call inside those two XBFs is unnamed (the RAVEN_TRACE_XBF_KEYS diagnosis shows it is NOT the unconsumed-resource-map
  check; the failure is a real E_NOTIMPL from some element setter the engine answers E_NOTIMPL rather than S_FALSE).
  With more content loading, the app walked further than any earlier run and reached a new, harder wall: the .NET Native
  runtime fail-fasts (`RaiseFailFastException` from sharedlibrary through mrt100_app with shapr3d.dll frames under
  Application.Start's statics_Start) on an unhandled "Class not registered" (80040154, exit status 2147746132) — earlier
  than the old natural 60-second exit, i.e. forward progress that is fatal instead of tolerated. The refuse-and-continue
  asks right before it are new too: `Windows.Media.Core.CodecQuery` and `Windows.UI.Xaml.Controls.ContentDialog` (both
  `no class`), the ContentDialog refusal the strongest candidate for the fail-fast — the app's own projection dies
  ~840 trace lines after it, and the fail-fast's activation is invisible to both trace layers I added this round (the
  `RAVEN_TRACE_ACT_REFUSAL` gate on DllGetActivationFactory's refusal tail and the shell's `no class` print), so it
  travels a third path, or the exception is the delayed rethrow of the ContentDialog refusal. The honest launch evidence
  for the attach goal itself is unchanged and precisely bounded: two composition swapchain rounds create and tear down,
  zero SURFACE_LOST on the GL-renderer Weston, and zero `vkAcquireNextImageKHR(`/`vkQueuePresentKHR(` calls — the
  renderer never presents because nothing attaches the chain, and nothing attaches because the SwapChainPanel does not
  exist until the last two control XBFs load. The next increment is concrete: serve Windows.UI.Xaml.Controls.ContentDialog
  through the controls factory (an IContentDialog face over the frame's Control machinery with honest string/content
  storage and a refusing ShowAsync) and Windows.Media.Core.CodecQuery's statics, then name the last refused call inside
  TitleBarControl/ThemedImage's XBFs. Diagnostics added this round (all env-gated, marked TEMPORARY): the ActivateInstance
  type-name print, the unconsumed-resource-key print (RAVEN_TRACE_XBF_KEYS, unused so far), the
  DllGetActivationFactory refusal-name print (RAVEN_TRACE_ACT_REFUSAL), and the swapchain-panel attach/detach note.
  Fixture suite passes 14/14 throughout. Logs: /tmp/raven-shapr-probe/launch-sc1.log (set-swapchain + Control scalars),
  sc2.log (XBF key diagnosis instrumented), sc3.log (S_FALSE skip; exit 84), sc4.log (refusal naming; same fail-fast).
  App state from the runs moved to /tmp/raven-shapr-probe/st24..st27-*.
- ContentDialog and CodecQuery served, the fail-fast walked through four codes, and the wall is now a vtable shape
  (2026-11-11). The classes the media path asked for entered the runtime catalog (native/nt/registry.cpp) and the
  prefix's registry.dat directly, and raven-xaml.dll serves them. ContentDialog rides the frame machinery: the
  controls factory gained factory_contentdialog, whose composable IContentDialogFactory (05557178-9d8e-4315-b37d-
  680c14012c35, the IID the launch showed RoGetActivationFactory being asked) composes the instance with the managed
  CCW as the outer the way Control/UserControl do, names itself the ContentDialog class over the inner, and keeps the
  whole Control scalar surface; the dialog's own face joins when its cast IID lands in the frame's full-GUID refusal
  trace. Windows.Media.Core.CodecQuery and CodecSubtypes live in the new codec_query.c: CodecQuery activates
  (new CodecQuery() — the RoGetActivationFactory(IActivationFactory) plus ActivateInstance pair the trace shows) and
  its factory answers ICodecQueryStatics (ecc8691b-c1db-4dc0-855e-65f6c551af49, from the refused-cast trace) whose
  FindAllAsync(kind, category) returns a completed-shaped IAsyncOperation`1<IVectorView`1<CodecInfo>> over an empty
  set (raven ships no codec registry) with an IAsyncInfo part for the await's status reads, the 9-slot async layout
  the engine's storage machinery already carries; CodecSubtypes' factory answers its statics (a66ac4f2-888b-4224-
  8cf6-2a8d4eb02382, likewise from the refused RoGetActivationFactory cast) with GetSubtypes over an empty list.
  What the round bought, honestly measured by the fail-fast under Application.Start (shapr3d.dll+2075fae, the same
  unguarded interop site each time): 80040154 "Class not registered" → (one round lost to a self-inflicted factory
  bug — the codec factory object was handed out as the vtable address itself, so the first QI dereferenced a code
  pointer as a vtable; the object is a struct around the pointer now, the shape every other raven factory uses) →
  80004001 (the CodecQuery instance the projection constructs did not exist) → 80004002 "Specified cast is not valid"
  (the CodecSubtypes statics cast refused) → the current wall 80004003 (E_POINTER): the subtypes statics face is
  served, the .NET identity probes on it are visible (IAgileObject, IMarshal as 00000003 — NOT IInspectable, which is
  af86e2e0-...; IReferenceTracker), and the trace shows GetSubtypes is NEVER invoked, so the projection calls a slot
  outside my single-method vtable — the real ICodecSubtypesStatics member list differs from the one-method guess
  (an earlier member, a different arity, or the empty-IVectorView`1<CodecSubtype> face the result carries). Each
  refused probe is named by the TEMPORARY full-GUID trace (raven-xaml: codec query has no interface ...), and the
  same trace will name the vector face the moment a result reaches a cast. The XBF picture is unchanged this round:
  the two surviving failures (Shapr3D.UILibrary.TitleBarControl / ThemedImage constructors throwing through their own
  XBF loads, offsets 0x15df/0x163c) were never reached in cd5/cd6 — the media-path fail-fast fires first — so their
  naming remains queued behind the codec wall. ContentDialog's activation was likewise never reached in cd6 (the
  codec path fires first), so its factory face is unexercised by the app so far. Fixture suite passes 14/14 across
  all rounds; nothing committed. Logs: /tmp/raven-shapr-probe/launch-cd1.log (CodecQuery catalog first served, the
  vtable bug), cd2.log (statics IID revealed, fail-fast → 80004001), cd3.log (instance served, → 80004002),
  cd4.log/cd5.log (subtypes catalog, the cast refusal), cd6.log (subtypes statics served, → 80004003 E_POINTER).
  App state from the runs moved to /tmp/raven-shapr-probe/st28..st33-*.
- The CodecSubtypes statics ABI pinned from the system winmd and implemented; the fail-fast moved to a
  NullReferenceException (2026-11-11). The real member list came from the mounted Windows metadata:
  Windows.Media.winmd (WinSxS amd64_microsoft-windows-runtime-metadata_31bf3856ad364e35_10.0.26100.9549) parsed
  with a hand ECMA-335 reader (BSJB root through #~/#Strings/#Blob, TypeDef/MethodDef tables; note the heap
  index convention: #Strings index i → byte offset i, #Blob index i → offset i, and the winmd TypeDef row
  carries FieldList and MethodList at +14/+16 in this writer's order — the MethodList column validated against
  ICodecInfo's exactly-known members). The parsed truth for the codec family: Windows.Media.Core.CodecQuery and
  CodecSubtypes are classes with EMPTY MethodLists; their interfaces carry the members — **ICodecInfo** (rows:
  get_Kind, get_Category, get_Subtypes, get_DisplayName, get_IsTrusted — get_Subtypes() returns
  IVectorView`1<string>), **ICodecQuery** (one member: FindAllAsync), **ICodecSubtypesStatics** = FIFTY-ONE
  string properties get_VideoFormatDV25…get_AudioFormatWMAudioV9 (29 video + 22 audio FOURCC/GUID names, in
  metadata order — not alphabetical), and **ICodecSubtypesStatics2** = one member, get_VideoFormatAv1. There is
  no GetSubtypes on the statics and no get_Subtypes either — the previous one-method guess was wrong in both
  shape and identity, which is exactly why the projection's slot call never landed. The vtable now implements
  the real list: all 51 properties answer an empty HSTRING (raven ships no codec registry — the honest value
  when no codec backs a format), plus ICodecSubtypesStatics2's Av1 getter, served on both the CodecSubtypes
  factory and the statics object (the runtime probes the statics object for Statics2 too). The instance-side
  face also moved: the per-object refusal trace (each refusing object now names its kind — TEMPORARY) showed
  the ecc8691b cast lands on the CodecSubtypes statics and the 222a953a cast on the CodecQuery INSTANCE, so the
  instance now answers 222a953a with a FindAllAsync face returning the completed empty-set operation object.
  Launch evidence: the fail-fast walked 80004002 → 80004003 and the refusal list is now down to benign identity
  probes only (IAgileObject, IMarshal as 00000003, IReferenceTracker — no interface casts refused anywhere in
  the codec family). The current wall is a .NET NullReferenceException ("Object reference not set to an
  instance of an object.", 80004003, same shapr3d.dll+2075fae interop site): the codec call chain completes
  without a refusal, and the null the projection dereferences is almost certainly the FindAllAsync return-path
  mismatch — the winmd signature decode (hand walker, partially reliable) reads ICodecQuery.FindAllAsync as
  returning IVectorView`1<string> DIRECTLY (not IAsyncOperation`1<...>), and the parameter list decode is
  ambiguous (2 or 3 parameters; my vtable has (kind, category, out) with the retval in r9 — if the projection
  passes 3 parameters the retval rides the stack, which would hand the caller a stale register → the null).
  Next increment: resolve the exact FindAllAsync parameter list (finish the winmd signature decode — the
  TypeRef resolution machinery now exists: IVectorView`1 resolves to Windows.Foundation.Collections.IVectorView`1
  — or verify by making the operation/ vector object the retval under both arities), and reconsider whether
  FindAllAsync should return the plain empty IVectorView<string> the winmd signature seems to name. The ContentDialog
  instance face and the two remaining control XBFs (TitleBarControl/ThemedImage, offsets 0x15df/0x163c) stay
  queued behind this. Fixture suite 14/14 across all rounds; nothing committed; the refusal traces remain
  env-gated and marked TEMPORARY. Logs: /tmp/raven-shapr-probe/launch-cd7.log (51-member vtable first served),
  cd8.log (Statics2 IID named), cd9.log (statics2 served, both casts persist), cd10.log (per-object naming:
  ecc8691b on subtypes statics, 222a953a on codecquery instance), cd11.log (no IID refusals; wall = 80004003
  NullReferenceException). App state moved to /tmp/raven-shapr-probe/st34..st38-*.
- The FindAllAsync signature decoded from the winmd bytes, the return-path fixed, and the await machinery reached
  (2026-11-11). The definitive MethodDefSig decode (ECMA-335, with TypeRef resolution — TypeRef 1648 =
  Windows.Foundation.Collections.IVectorView`1, 1650 = IIterable`1, 521 = CodecInfo, 522 = CodecKind, 520 =
  CodecCategory; the signature blob offset convention settled as blob index i → heap byte i, verified against
  get_Kind) reads ICodecQuery.FindAllAsync's sig "20 03 15 12 99b1 01 15 12 99c1 01 12 8825 11 8829 11 8821 0e"
  as: **FindAllAsync(IIterable`1<CodecInfo>, CodecKind, CodecCategory) -> IVectorView`1<string>** — three
  parameters, HASTHIS, not generic, and the return is a DIRECT IVectorView`1<string> with no IAsyncOperation
  wrapper in the OS-internal interface. The previous vtable read the retval from r9 while the real ABI carries
  it on the STACK as the fifth argument — that stale-register read was the NullReferenceException. The vtable
  now declares five parameters (This, the IIterable input — ignored, kind, category, results-out on the stack)
  and returns the answer directly. The launch (cd12) proved the call completes: "codec instance FindAllAsync
  answered empty vector" fires, and the returned object's QI pattern names the caller's expectation: it casts
  for c1c59efc-71df-5534-b03f-e18a8fbe78bb (a version-5/variant parameterized GUID — the
  IAsyncOperation`1<IVectorView`1<CodecInfo>> face of the projected public API) and asks for IAsyncInfo
  (00000036) — the .NET await machinery treats FindAllAsync's return as an awaitable, matching the public-API
  shape rather than the OS-internal one. The wrapper was therefore changed again: FindAllAsync now returns a
  completed-operation object answering the c1c59efc parameterized face plus IAsyncInfo (Status Completed, Id 1,
  ErrorCode S_OK, Cancel/Close no-ops), whose GetResults hands the empty IVectorView object (the traced launch
  shows "codec instance FindAllAsync answered the empty completed operation"). Launch cd13: the fail-fast moved
  again, from 80004002 to **1007** — the .NET Native runtime fail-fasts in its await/interop machinery
  (sharedlibrary.dll+98b94c → mrt100_app.dll+3a472, the same path that followed the first CodecQuery
  activation) AFTER receiving the completed operation, with no refusals traced — the shape mismatch is now
  inside the await itself: the most probable cause is the operation object's slot layout (my wrapper carries
  put_Completed/get_Completed/GetResults at slots 6-8 in the mingw-template order, while the real
  IAsyncOperation`1<T> ABI flattens IAsyncInfo's five members (Cancel, Close, get_ErrorCode, get_Id,
  get_Status) after IInspectable, pushing put_Completed to slot 11 — if the .NET awaiter calls slots on the
  flattened layout, slot 6 lands in my put_Completed vs its Cancel and every subsequent call misfires), or the
  completed-handler invocation convention, or the GetResults return being the vector whose own parameterized
  IVectorView`1<string> face (its IID would be the next trace) is not yet served. The exact IVectorView`1<string>
  parameterized IID and the handler shape will both name themselves in the next trace once the operation's
  slot layout matches — the fix is to serve the flattened 14-slot layout (IInspectable + IAsyncInfo's five +
  put_Completed/get_Completed/GetResults, mirroring the storage.c precedent only where it is proven) with the
  IVectorView`1<string> face for GetResults implemented on the vector object. Honest state: the codec call
  chain is refusal-free end to end (activation, instance, cast, call, completed-operation return all traced),
  the app still fail-fasts in the .NET await on the shape inside that operation, and the two control XBFs
  (TitleBarControl/ThemedImage) plus ContentDialog's instance face remain queued behind it. Fixture suite
  passes 14/14; nothing committed; diagnostics env-gated and marked TEMPORARY. Logs:
  /tmp/raven-shapr-probe/launch-cd12.log (decoded ABI first served, vector return traced, cast+IAsyncInfo
  named) and cd13.log (awaitable wrapper, fail-fast 1007). App state moved to /tmp/raven-shapr-probe/st39..40-*.
- The FindAllAsync ABI resolved from the caller's registers, the flattened operation layout landed, and the wall
  moved inside the app's .NET runtime (2026-11-11). Two evidence layers settled the arity question the last entry
  left open. First, the winmd signature decode was completed properly: MethodDef row 2749's sig
  "20 03 15 12 99b1 01 15 12 99c1 01 12 8825 11 8829 11 8821 0e" decodes (with TypeRef 1648 =
  Windows.Foundation.Collections.IVectorView`1, 1650 = IIterable`1, 521 = CodecInfo, 522 = CodecKind, 520 =
  CodecCategory) as **FindAllAsync(IIterable`1<CodecInfo>, CodecKind, CodecCategory) -> IVectorView`1<string>**
  — the OS-internal interface genuinely carries three parameters and returns the vector directly. Second, the
  TEMPORARY raw-register trace on the FindAllAsync slot (RAVEN_TRACE_CODEC_ARGS) captured the caller's actual
  arguments: kind=1 category=1 — **enum-sized values in rdx/r8, meaning the caller passes TWO parameters**
  (CodecKind, CodecCategory — the public-API form) with the retval in r9. The projection's ABI and the OS
  winmd's ABI disagree; the caller is authoritative, so the vtable slot is the two-parameter form with the
  retval in r9 (the winmd's three-parameter first argument — the IIterable`1<CodecInfo> — would have arrived
  as a heap pointer in rdx, and it did not). With the retval slot finally aligned, the call completes and the
  operation object reaches the marshaler — but the failure moved rather than resolved: the .NET runtime raises
  NullReferenceException (80004003, "Object reference not set to an instance of an object.") immediately after
  the return, BEFORE calling any slot on the returned operation (no put_Completed, no get_Status, no GetResults
  traced — the object sits untouched), and without a QI refusal (the c1c59efc cast and IAsyncInfo are answered
  by the flattened 14-slot face — IInspectable 0-5, get_Id 6, get_Status 7, get_ErrorCode 8, Cancel 9, Close 10,
  put_Completed 11, get_Completed 12, GetResults 13, implemented per the SDK's asyncinfo.h member order). The
  failure is therefore inside the .NET Native marshaler/type system while wrapping the return: creating the
  managed wrapper for the nested generic instantiation IAsyncOperation`1<IVectorView`1<CodecInfo>> — a nesting
  the app's .NET Native image may simply not carry (on Windows this internal-OS-interface path is not exercised
  through .NET with these exact types). Raven-side options for the ABI are exhausted: activation, instance,
  cast, call, arguments, retval, and the returned object's cast all trace correctly now. Next increment
  candidates, in order of cheapness: (1) return NULL with S_OK from FindAllAsync — if the marshaler converts
  the null into a null Task the app's null-check path may proceed differently, and the failure mode names the
  marshal stage; (2) check whether the c1c59efc face should instead be answered by an object whose
  GetRuntimeClassName is the parameterized type name the marshaler may read before the null deref; (3) accept
  the wall as the app's .NET Native image lacking interop metadata for the nested generic — a fixed limitation
  for this internal-API path — and move to the ContentDialog instance face and the two control XBFs
  (TitleBarControl/ThemedImage) which are queued and independent of the codec path. The flattened operation
  layout and the two-parameter slot are kept (they match the caller's observed ABI); the raw-args trace is now
  env-gated (RAVEN_TRACE_CODEC_ARGS). Fixture suite 14/14; nothing committed. Logs:
  /tmp/raven-shapr-probe/launch-cd14.log (flattened layout, still 1007 — the layout was not the blocker),
  cd15.log (two-param retval, 80004003 immediately at the return), cd16.log (raw-register capture: kind=1
  category=1 — two-param form confirmed; the marshaler's NullReferenceException right after the answered
  return). App state moved to /tmp/raven-shapr-probe/st41..43-*.
- The loader restored from the stash; the tolerate workflow's state; the fixture regression and the honest handoff
  (2026-11-11). Two things this round settled. First, the tolerate workflow works as designed:
  RAVEN_TOLERATE_FAILFAST=1 survives the codec round's fail-fast and the app ran its full window with
  ShowWindow (cd18, the pre-reconstruction loader) — but the app state it leaves is polluted and must be cleared
  (done, st45-*). Second, the null-return probe (candidate 1) was NOT exercised: the app's flow varies run to
  run — cd17 never called FindAllAsync at all (the codec cast failed during page navigation instead: the
  exception message names it — "Unable to cast object of type 'EETypeRva:0x000DFAC0(Shapr3D.App.WelcomePage)' to
  type 'EETypeRva:0x0001E688(Windows.UI.Xaml.Controls.IPageOverrides)'", thrown inside the app's
  OnNavigatedTo override, which raven's frame machinery invokes through the IPageOverrides face its CCW serves
  and tolerates as 0x80004002 per design). The managed castclass of the WelcomePage to IPageOverrides failing
  while the WinRT QI for the same IID succeeds is a .NET Native interface-map state — raven-side unfixable.
  Third, a work accident and its recovery: a mechanical attempt to annotate the loader's failure sites corrupted
  xaml_loader.c, and the reconstruction from HEAD silently DROPPED the previous sessions' uncommitted loader
  work (the freed-collection plausibility guard on the append path, the guarded element-collection function, the
  empty-Grid substream substitution). The corrupted working tree was recovered from the dangling stash commit
  bf3348ae0c2605f02cf6db2bd2c28e117915cb27 (found via git fsck — the stash had captured the pre-botch cd16-state
  loader, 2776 lines with the guard intact); the botch artifacts (37 inert site-marker assignments, one
  duplicated else-if line, the stale site-note prints) were removed and the loader rebuilt. Verification: the
  fixture suite passes 14/14 EXCEPT native_xaml_controls_x64, which returns 714 from its own code
  (xaml_controls_client.c line 2654: the loaded style's get_BasedOn answers no base style — the Style.BasedOn
  stable-property reference "DefaultButtonStyle" no longer resolves). That regression is NOT explained by the
  restored loader (the cd16 state it restores is the state the tolerate run exercised, and the tolerate run
  built content) — it first appeared in this round before any loader edit and survives the restore; the next
  session must diagnose it first (the candidate causes: the S_FALSE skip now passing a load whose BasedOn
  resolution silently failed — trace_resource_failure at the case-23/24 site names the unresolved key — or the
  static-resource resolution against the platform default styles having changed with the tolerate runs' state
  pollution of the default prefix, which the fixture uses). The tolerate run with the restored loader (cd19)
  died early (fail-fast 80070057 during GC/MemoryManager interop, then the tolerated entry hung at 0x0) — the
  tolerate state after a fail-fast is undefined per the design comment; cd18's clean tolerate run is the
  reference. Honest handoff: the codec chain is ABI-correct and refusal-free (activation, instance, call,
  retval, cast, IAsyncInfo all traced); the walls now live in the app's .NET Native image (the nested-generic
  wrapper NullReferenceException; the WelcomePage IPageOverrides managed cast) and in the fixture regression
  (714 at get_BasedOn). Nothing committed; all diagnostics env-gated and marked TEMPORARY. Logs:
  /tmp/raven-shapr-probe/launch-cd17.log (null probe unreached; the OnNavigatedTo cast named), cd18.log
  (tolerate, pre-reconstruction loader, full window), cd19.log (tolerate, reconstructed loader, early death),
  /tmp/fixture-x64c.log (the 714 fixture trace). App state moved to /tmp/raven-shapr-probe/st44..46-*.
- The fixture regression root-caused and fixed: the botch cleanup had left duplicated `goto done;` lines, and the
  unbraced ones silently truncated every load that reached them (2026-11-14). The 714 in native_xaml_controls_x64
  was never a resource-resolution failure at all — the loader's resource path is intact (the platform lookup answers
  DefaultButtonStyle, resolve_resource walks scope/ambient/application dictionaries down to raven_xaml_platform_resource
  and returns it, and raven_xaml_element_apply_value stores it through put_BasedOn). What the previous entry's
  candidate (a) guessed ("the S_FALSE skip passing a load whose BasedOn resolution silently failed") was close in
  shape but wrong in mechanism: the load did not skip one node — it STOPPED at the first SetValueFromStaticResource
  (node 30) and answered S_OK. The mechanism: last round's mechanical annotation botch on xaml_loader.c had
  duplicated `goto done;` after many failure checks; the cleanup removed the site-marker assignments and one
  duplicated else-if but missed 35 duplicated control-flow lines, and where the original was an UNBRACED
  `if (FAILED(status))\n\t\t\tgoto done;` the duplicate `goto done;` executes unconditionally — case 30 (26/27/36
  and several other cases) abandoned the document at that node with the current S_OK status, no failure recorded,
  BasedOn never applied, get_BasedOn answered null. This is why the regression "first appeared before any loader
  edit and survived restores": the stash-restored cd16 loader already carried the duplicates, and every document
  with a static-resource reference died at its first one. Diagnosis path: RAVEN_TRACE_DEBUG node-walk notes showed
  the walk reaching `node kind 30 at 176` and then exiting the loop with no case-body trace, no trace_resource_failure
  print, no XBF-failure record — a silent mid-document exit; a wsprintfA note is a trap here (raven's wsprintfA has
  no %zu/%p support and truncates the message at the first unsupported conversion). Fix: removed all 32 duplicated
  artifact lines (the second of each consecutive pair: 30 unbraced/braced `goto done;` duplicates plus the duplicated
  `if (FAILED(status)) goto done;`, the duplicated create-reference guard and the duplicated E_NOTIMPL guard in
  case 28/31/38) from native/upstream/raven-xaml/xaml_loader.c; the legitimate double `next_u16(&r)` in case 28/31/38
  kept. Verification: native_xaml_controls_x64 AND native_xaml_controls_x32 green (first time since the regression
  appeared), the standard suite green (native_data_writer, native_storage, native_xml_dom, native_automation_properties,
  native_reference_tracker, native_suppress_transition_x64, native_mspatcha_x64/x86 — 13/13 across widths), and the
  wider xaml/style/window fixtures 19/19. Launch re-verified with the rebuilt component copied to the prefix
  system32 under the GL-renderer headless Weston (wayland-99, restarted this session with --renderer=gl): the run
  is byte-for-byte the documented reference wall — the codec chain activation/cast/identity probes all traced
  refusal-free, XBF failures exactly the two documented survivors (0x15df/0x163c, TitleBarControl/ThemedImage
  constructors, 80004001), and the fail-fast remains the .NET Native marshaler NullReferenceException (80004003,
  shapr3d.dll+2075fae, mrt100_app under sharedlibrary) at the FindAllAsync return — the app's .NET Native image
  wall, unchanged, exactly the state the loader should reproduce now that its documents load completely. All
  round-added diagnostics removed; the pre-existing TEMPORARY traces (RAVEN_TRACE_XBF_KEYS, the failure-site note)
  remain env-gated as before. Nothing committed. Logs: /tmp/raven-shapr-probe/launch-bf1.log (untraced launch,
  exit 3 / 80004003), launch-bf2.log (traced, 634k lines, the same wall with the refusal picture above). App
  state moved to /tmp/raven-shapr-probe/st47-*, st47b-*.
- The control-XBF chain opened: five raven-side walls fell in one round and TitleBarControl.xbf now loads
  completely, but both control constructors still die — the walls have moved inside the app's .NET Native
  image (2026-11-14). The loader fix did not unlock the two control XBFs by itself; each launch named the next
  gate and the round tore through them. First: the UILibrary controls' generated InitializeComponent calls the
  three-argument Application.LoadComponent overload with **location=1** where the app pages pass 0, and
  statics_LoadComponent2 answered location!=0 with E_NOTIMPL/E_INVALIDARG — the caller is authoritative, both
  values now load the same way (the two failure-path notes in load_component — locator QI, URI guards, pri
  blob, load_into — are the round's kept TEMPORARY diagnosis; the entry/exit chatter was removed). With the
  load reached, the XBF failure note finally named real nodes. Second: TitleBarControl.xbf's root binds by the
  **stable UserControl entry (491)** — the stable class table only knew Page (525) — and use_target's
  GetRuntimeClassName match cannot bind the derived managed instance, so the entry was added and the Page
  precedent extended: the stable-491 root binds through an IUserControl QI on the target (the winmd IID
  A7A69EC9-EA35-4679-BF29-F4F09286D314, the face xaml_controls.c's factory composes). Third: node 0x277
  (SetValueConstant ColumnDefinition_Width) died because property_reference's stable path answered only the
  hand-curated property table (25 entries) — compiled documents name every property by stable index — so the
  generator now emits **xbf_property_names[]** (the full table from xbf-stable-indices.json, short names, the
  same source the inspection tools decode) and property_reference falls back to it; the curated table stays
  first. That fix exposed a real latent bug the new name flow reached: apply_value's ResourceDictionary-Source
  branch QI'd a NULL scope object (the guard existed only on the colour path) — c0000005 in apply_value —
  fixed with the skip convention at the top of apply_value and at the resolved-resource apply site (a skipped
  scope's assignments are S_FALSE, not document-fatal). Fourth: the shapes and the visual-state tree the two
  XBFs declare did not exist — Border (286), StackPanel (481), Image (326), Setter (263), VisualState (137),
  VisualStateGroup (138) joined the stable table and the runtime catalog (registry.cpp and both registry.dat
  hives), and the engine grew them: Border/StackPanel/Image build over the grid element struct (children,
  scalar sizes, attached slots, the UIElement face; correct class names, IGrid stays the grid's), and
  VisualStateGroup/VisualState are honest IInspectable objects with name storage and live collections; the
  loader's collection push accepts VisualStateGroups/States/Setters through the new
  raven_xaml_elements_named_collection. Fifth: case 17 (SetDeferredProperty) was document-fatal with a target —
  ThemedImage.xbf's FrameworkTemplate_Template rides it — and now loads the named substream the way case 15's
  ElementContent does and hands the root to raven_xaml_control_template_put_root; the ControlTemplate scope
  owns the tree. Also removed: one leftover duplicated `if (target && !context->tolerant) goto done;` in case
  10 (inert — both lines conditional — but the same botch family). A frame-QI experiment (forwarding the
  refusal tail to the managed outer, TLS-guarded) was reverted: it looped with the CCW's own QI and did not
  move either constructor wall. Launch evidence (launch-final.log, exit 3): zero load_into failures —
  TitleBarControl.xbf loads end to end; ThemedImage's ctor throws NotImplementedException (80004001) BEFORE
  its LoadComponent — the trace shows the frame refusing **IDependencyPropertyStatics (49e5f28f)** as an
  instance ask right before the throw, the .NET Native interface-map confusion class (an instance asked for a
  statics interface — no raven E_NOTIMPL site fired, all now trace); TitleBarControl's ctor runs into app code
  and takes c0000005 at shapr3d.dll+282c07f (reading 0) right after the frame refused
  **f5f69427-55ed-5512-8429-d4f6626dfcdd** — an IID carried in NO app binary or winmd (data3 0x5512 breaks
  UUID version bits; a corrupted or runtime-synthesized ask through SharedLibrary.dll+82c9a4, the
  non-delegating face, frame kind d0 s1 outer set) — activation fails 80004003; the page documents still fail
  at the two control nodes (0x15df/0x163c, the stale-record notes), SwapChainPanel activation count stays
  zero, zero SetSwapChain calls — the render attach remains gated behind both constructors. The
  composition_client fixture was stale against the pre-session attach contract (it still expected native
  SetSwapChain to answer E_NOTIMPL; the sc1 round replaced that refusal with honest storage) — composition_x32
  had been failing since that round against a synced prefix and the failure surfaced this round when the
  default prefix's stale dlls were refreshed; the fixture now checks the attach contract (S_OK, ref counts,
  stored-chain identity) and **composition_x64/x32 are green**. Fixture suite green: 50/50 on the
  xaml/style/window/composition/render/dispatcher set and 9/9 on the standard set; controls x64/x32 green.
  Nothing committed. Logs: /tmp/raven-shapr-probe/launch-iter1.log (the documented reference wall reproduced),
  iter3/iter4/iter5 (LoadComponent2 location=1, use_target UserControl, property-stable 95 named), iter6.log
  (Border CLASS_E_CLASSNOTAVAILABLE at 0x29d), iter8.log (TitleBarControl.xbf loads; the shapr3d.dll AV),
  iter9.log (the frame-QI caller diagnosis: via SharedLibrary.dll+82c9a4, s1, outer set), iter10.log (the
  reverted forwarding experiment's QI loop), launch-final.log (the recorded state above). App state moved to
  /tmp/raven-shapr-probe/st48..st60-*.
- The two constructor walls diagnosed trace-first: f5f69427 is IMap&lt;Object,Object&gt; (served honestly now), the
  TitleBarControl c0000005 is a null double-unbox in the constructor after its InitializeComponent completes, and the
  ThemedImage NotImplementedException is the .NET Native marshaler failing inside a CoreDispatcher-typed wrapper —
  three honest raven surfaces landed, neither constructor wall moves yet (2026-11-14). The launch procedure this
  round also got its missing piece written down: raven's own entry timeout defaults to **5000 ms** (`--timeout-ms`
  in main.cpp), so every launch must pass `--timeout-ms=60000` (or larger) or the run dies "native entry timed out"
  after five seconds looking like a hang — the earlier reference runs' "exit 3 after the full 60-second trace
  window" is this flag, and the t1..t5 probes that produced 32-line logs and 300-second timeouts were the default
  killing the process, not a new wall. First: the mystery IID. **f5f69427-55ed-5512-8429-d4f6626dfcdd is
  Windows.Foundation.Collections.IMap`2&lt;IInspectable,IInspectable&gt;** — the parameterized-GUID SHA1 rule over the
  signature `pinterface({3c2925fe-8519-45c1-aa79-197b6718c1c1};cinterface(IInspectable);cinterface(IInspectable))`;
  raven already carried the full seven-method vtable and the derivation comment in
  windows_ui_xaml_dictionary.h (IIMap_2_IInspectable_IInspectable, "appears once in the real Windows.UI.Xaml.dll")
  — the previous entry's "data3 0x5512 breaks UUID version bits" was wrong: 0x5512 is a proper v5-formatted hash,
  and the IID is neither corrupted nor synthesized per-run; it is a REAL parameterized interface the runtime carries.
  The ask arrives via SharedLibrary.dll+82c9a4 (the .NET Native CCW's non-delegating face) when the app's
  dictionary-typed access resolves against the object. The frame now answers it: a new `resources` slot on the
  frame struct, created lazily through raven_xaml_create_stable(371) (the engine ResourceDictionary, which answers
  the map IID through the loader's own is_map_object path) and QI'd for IMap&lt;Object,Object&gt;; Lookup/HasKey/Insert
  land in the element's real resources, released with the frame. Evidence: the reference log's refusal tail ("frame
  has no interface f5f69427...") is gone — launch-final2.log shows seven `query f5f69427` asks, zero refusals. The
  crash itself, decoded from the exception scan and a disassembly of shapr3d.dll+282c07f: the faulting instruction
  is `movsd xmm2,[rbx+0x8]` with rbx=0 (the register scan's fourth slot, per seh.cpp's Rax-to-Rdi order), i.e. the
  app's constructor **unboxes a boxed double from an object that came back null** — the pattern around it reads a
  16-byte GridLength struct out of a helper call, a bool and a double out of two more helpers, two
  GetValue-shaped reads (rcx=receiver, rdx=a DP object loaded from a static table) each followed by the
  sharedlibrary+0xd6978 unbox/cast cell (dl=0xd), and a three-double call — the shape of reading
  SystemButtonsColumn's Width/MinWidth/MaxWidth bounds. The reads sit AFTER the load frames (load_component,
  create_reference, create_known on the stack BELOW provider_activate), so the constructor's InitializeComponent
  has completed and the ctor body is running. Raven-side candidates for the null were eliminated one by one:
  frame_dependency_get_value answered the DP's metadata default (or null) — now it returns
  **DependencyProperty.UnsetValue** when neither a stored value nor a metadata default exists (the real runtime's
  GetValue never hands out a null reference; the UnsetValue singleton is exported as
  raven_xaml_dependency_unset_value); ColumnDefinition/RowDefinition **Min/Max/Actual were E_NOTIMPL stubs whose
  NULL out slot the caller reads unchecked** — implemented honestly over new min/max/actual storage (min 0,
  max PositiveInfinity per the real metadata defaults, actual 0 until a measure pass exists; the put slots now
  take the double per the ABI, the hand declarations in windows_ui_xaml_layout.h corrected), and the crash still
  lands at the same address with the same registers — so the failing read's receiver or DP is still unresolved
  (the remaining candidates: the frame's own IFrameworkElement extent slots, all framework_unimplemented with an
  untouched out slot, whose exact ABI order within the 46-slot hand table this round did not reconstruct; or a
  DP raven never registered). The CoGetApartmentType bursts, the IButton refusal on the root frame
  (SharedLibrary+82c9a4 again) and the engine's FillRect paint immediately precede the crash every run.
  Second: ThemedImage. The docs' "instance ask for IDependencyPropertyStatics" reading was imprecise: the trace
  shows the 49e5f28f asks are **activation-statics asks** ("winrt: activation iid 49e5f28f..." from
  raven-shell.dll+fa47, the DependencyProperty factory — SERVED, the DP registrations complete, PropertyValue
  boxing served) — the statics path works. The throw's real context: the NotImplementedException's object
  (t8/t9/t14 exception scans) embeds **"Dispatcher" and "Windows.UI.Core.CoreDispatcher"**, and the preceding
  API stream shows dp_register receiving name="Dispatcher" type="Windows.UI.Core.CoreDispatcher" from the app —
  the ThemedImage constructor registers or touches a Dispatcher-typed property; the marshaler then fails while
  completing a managed wrapper (the exception object carries the partially-built wrapper's raven vtable/function
  pointers — control_put_template among them), throwing NotImplementedException from its own prebaked message.
  Every raven-visible ask around the throw is served (DependencyObject, the peer bridge, IFrameworkElement,
  IControl, IReferenceTracker); a temporary gated refusal trace on the dispatcher's QI fired only for the
  tolerated IReferenceTracker ask at startup, never at the throw, and a gated GetRuntimeClassName caller trace on
  the definition vectors never fired at all (the marshaler does not read their class names) — both traces removed.
  This places the ThemedImage wall inside the app's .NET Native image (the interop-metadata class, like the codec
  round's nested generic): the marshaler cannot build the wrapper for the CoreDispatcher-typed property, and no
  raven-side surface is refused at that point. What the round landed: the frame's Resources map face (above), the
  UnsetValue semantics on frame_dependency_get_value, and the honest Row/ColumnDefinition bound surface (six
  refusal sites gone). Launch evidence (launch-final2.log, exit 3): zero f5f69427 refusals, ThemedImage still
  80004001 before its LoadComponent, TitleBarControl still c0000005 at shapr3d.dll+282c07f → 80004003, the page
  documents still fail at the two control nodes (0x15df/0x163c), SwapChainPanel string-mentions only (14, the
  manifest/class-name compares), zero SetSwapChain. Fixture suite green after each change: 35/35 on the
  xaml/style/window/composition/render/dispatcher/core_window set, 13/13 on the standard set, controls x64/x32
  green. Nothing committed; the round's diagnostics were removed, the kept surfaces are honest implementations.
  Logs: /tmp/raven-shapr-probe/launch-t1..t5 (the 5-second default-timeout probes), launch-t6.log (the full
  reference reproduction with the FRAME_QI gate), t7..t9 (IMap serve, vector/dispatcher traces), t10..t15 (the
  single-step probes: GetRestrictedErrorInfo already carries 80004001 when reached; the CoGetApartmentType-anchored
  steps drowned in memcpy/spin loops), t16..t18 (UnsetValue, cell dumps — the crash-site cells 0x136e3c0 →
  sharedlibrary+0xd6978, the other three not yet resolved), t19 (40k-step walk, same loops), launch-final2.log
  (the recorded state). App state moved to /tmp/raven-shapr-probe/st61-*, st62-*.

- The IFrameworkElement vtable map is finally authoritative, and following it moved BOTH constructor walls
  and the page-activation wall (2026-11-15). The reconstruction source is not the mingw header (the local
  windows.ui.xaml.h predates the interface entirely — zero "FrameworkElement" matches) but the REAL
  Windows.UI.Xaml.winmd at ~/.local/share/wine-uwp/.../WinMetadata (SHA-256 c55875b0..., the exact file the
  generated headers came from, verified against its GuidAttribute blob): IFrameworkElement's identifier is
  a391d09b-4a99-4b7c-9d8d-6fa5d01f6fbf — precisely raven's IID_frame_framework, so the 46-slot hand face IS
  the projected interface. The winmd slot order: 6 get_Triggers, 7/8 Resources get/put, 9/10 Tag get/put,
  11/12 Language get/put, 13 ActualWidth, 14 ActualHeight, 15/16 Width get/put, 17/18 Height get/put,
  19/20 MinWidth get/put, 21/22 MaxWidth get/put, 23/24 MinHeight get/put, 25/26 MaxHeight get/put,
  27/28 HorizontalAlignment get/put, 29/30 VerticalAlignment get/put, 31/32 Margin get/put, 33/34 Name
  get/put (the fixture's slot assertions confirm), 35 BaseUri, 36/37 DataContext get/put, 38/39 Style
  get/put, 40 Parent, 41/42 FlowDirection get/put, 43-50 the Loaded/Unloaded/SizeChanged/LayoutUpdated
  event pairs, 51 FindName, 52 SetBinding — the hand table was order-correct but one slot SHORT (SetBinding
  was missing; a caller through slot 52 would read garbage past the table) and every extent getter was an
  E_NOTIMPL stub with an untouched out slot. First honest surface: all 47 slots implemented over real frame
  storage — Width/Height NaN (auto), Min 0, Max PositiveInfinity (the column-definition constant),
  ActualWidth/Height 0 until a measure pass exists, else the current CoreWindow bounds (raven_current_window
  + get_Bounds), alignments Stretch (3), FlowDirection LeftToRight, empty Margin, Language empty, Tag/
  DataContext/Style stored refs, Resources = the same dictionary the IMap ask creates (get_Resources now
  hands the map face), BaseUri/Parent honest nulls, every getter writes the out slot, every putter stores,
  SetBinding a table-bound E_NOTIMPL stub. The getter/putter stub split is deliberate: getters write NULL
  into the out before refusing so an unchecked caller reads a safe null. Second honest surface (the
  ThemedImage dispatcher hint): no face on the control path returns a null dispatcher — DependencyObject and
  Window both serve the view's CoreDispatcher (verified in xaml_controls.c/xaml_window.c) — but the
  Dispatcher-typed DP READ was the gap: frame_dependency_get_value now reads the registered property type
  and, when it is Windows.UI.Core.CoreDispatcher and no stored value or metadata default exists, answers the
  view's own CoreDispatcher instead of UnsetValue (the wrapper-build failure the marshaler threw on). Third
  honest surface: IFrameworkElement2/3/4 (f19104be/c81c2720/6b765bb3, hand-declared vtbls — members
  RequestedTheme + DataContextChanged + GetBindingExpression, Loading, and the seven focus-visual members)
  answered by the frame over the same storage, because the generated app code casts the object to the exact
  version interface declaring the member it touches. And the deepest fix: the XBF loader's kinds 12/14 were
  skipped as "bookkeeping constants" — they are SetConnectionId, the compiled page's hook into the generated
  IComponentConnector. The traced TitleBarControl.xbf stream (TEMP-CONNECT trace, removed with the
  implementation) shows the contract: id 1 rides the control root, 2 the SystemButtonsColumn ColumnDefinition,
  3 the DraggableBorder, 5/6 the Image and TextBlock inside it, 4 a ContentPresenter — each node sits INSIDE
  the element's scope and Connect(connectionId, stack-top) binds the control's named fields and event
  handlers; skipping it is exactly why the ctor read a null SystemButtonsColumn field. The true faulting
  instruction (re-disassembled at the correct image base — the previous round's "movsd [rbx+8] unbox" decode
  was taken from the wrong offset) is cmpb $0x0,(%rbx) at shapr3d.dll+282c07f with rbx = this->[0x60] loaded
  at 282c03c: a null BOUND FIELD, not a null unbox. The loader now QIs the target for IComponentConnector
  (f6790987) and IComponentConnector2 (dc8f368b, GetBindingConnector at slot 6 — the first attempt used slot
  7 and failed fast with garbage), preferring the binding connector the projected I2 hands back, and calls
  Connect. The WelcomePage activation needed the same family completed at the object level: the bare page
  struct answered only IPage/IContentControl, so the CCW's interface walk refused IPageOverrides (the
  80004002 "cast WelcomePage to EETypeRva 0x1e688") and IFrameworkElement/3 (TEMP page-QI trace names the
  refused batch; kept gated as RAVEN_TRACE_PAGE_QI). Page composition now mirrors the UserControl precedent:
  the page factory composes a frame with standalone_page, the instance face is the frame's IPage (get_Frame
  null until navigation takes the page, NavigationCacheMode and the app bars stored), IPageOverrides accepts
  the lifecycle calls (the CCW's own override serves the managed dispatch — "page OnNavigatedTo" now RUNS
  the app's override, 0x80004003, previously unreachable), and the object names itself the Page class.
  Launch evidence (launch-fe12.log, exit 3; fe13 with traces): the 282c07f wall is GONE — TitleBarControl's
  ctor completes, zero c0000005 there, zero 80004003 activation failures; WelcomePage constructs past its
  whole InitializeComponent and reaches OnNavigatedTo; the ThemedImage throw moved but not off the wall —
  activation still fails 80004001 before its LoadComponent, and the exception object's embedded strings
  changed from the "Dispatcher"/"Windows.UI.Core.CoreDispatcher" wrapper evidence to an "Activ…"-prefixed
  marshaler string, so the Dispatcher-typed DP default moved it past that read into the next marshaler gap.
  The next walls, named: (1) shapr3d.dll+2b1cbb0 c0000005 reading 0 INSIDE the managed
  WelcomePage.OnNavigatedTo (the IPageOverrides EEType 0x1e688 is on the crash stack; the app's override
  dereferences something null — its own state or a raven surface not yet surfaced), (2) the ThemedImage
  marshaler throw above, (3) the long-standing fail-fast 80004003 at shapr3d.dll+2075fae via sharedlibrary
  (the documented marshaler wall) still terminates the run. Fixture suite green after every change: 41/41
  across the xaml/style/window/composition/render/dispatcher/core_window/automation set and the standard
  13, controls x64/x32 included. Nothing committed. Kept diagnostics (all env-gated, marked TEMPORARY):
  RAVEN_TRACE_FRAME_DP (GetValue DP names + answer kind), RAVEN_TRACE_FRAME_FE (the 47-slot face's getter
  traffic), RAVEN_TRACE_PAGE_QI (the page inner's refused IIDs); the loader's TEMP-CONNECT trace was removed
  together with the skip it diagnosed. Logs: /tmp/raven-shapr-probe/launch-fe1.log (the reference wall
  reproduced with the honest getters alone — identical 282c07f), fe2 (RAVEN_TRACE_DEBUG: the ctor's sparse
  refusal tail: IApplication/IGrid/ITextBlock/IButton asks), fe3 (the DP/FE gates fired ZERO lines — the
  ctor never calls GetValue or the extent getters, which redirected the hunt), fe4/fe5 (the TEMP-CONNECT
  stream), fe6 (the slot-7 fail-fast e92fdada), fe7/fe8 (the cast-failure state: three 80004002 patterns
  during WelcomePage construction), fe9 (the page-QI refusal batch), fe10/fe11 (page composition + the IF3
  refusal pinned as the cast target), fe12 (the recorded state), fe13 (the full traced state with the
  OnNavigatedTo crash window). App state left untouched (the run rewrote the prefix's LocalState/LocalSettings
  in place).
- The tolerate workflow mapped the full run beyond the walls, and the two surfaces the surviving paths asked
  for are served: ContentPresenter activates and the app's first real HTTP exchange goes through (2026-11-15).
  Four tolerated launches (RAVEN_TOLERATE_FAILFAST=1, fresh LocalState/LocalSettings each run, the state moved
  to /tmp/raven-shapr-probe/st80..st82-* after each), two without RAVEN_GUEST_DXVK_CONFIG (tol1) and two with
  it (tol2..tol4), agree on a deterministic sequence with ZERO fail-fasts and ZERO c0000005 — the codec-round
  marshaler wall (shapr3d.dll+2075fae) and the OnNavigatedTo IPageOverrides cast never fire in these runs
  because the app's flow varies and never reaches them; the exit is the natural status 3 after the full
  90-second window, preceded by an orderly teardown (EventUnregister/FlsFree/FreeLibrary from
  microsoft.ui.xaml.dll and tbb). The mapped sequence, each stage traced: (1) startup probes —
  AsyncCausalityTracer and StoreContext/CBSPackageDetector "no class" refusals (benign, the app catches);
  (2) the HTTP stack activates (HttpClient, HttpRequestMessage, HttpMethod, HttpBaseProtocolFilter served),
  and — NEW — the app builds its tracking POST body with Windows.Web.Http.HttpBufferContent and performs its
  FIRST real exchange: POST https://edge.api.flagsmith.com/api/v1/identities with a JSON body, sent through
  raven-web's WinINet transport and the response read (in tol1/tol2, without the surface, the body construction
  died 80040154 hresult_class_not_registered and NO exchange of any kind happened — HttpSendRequestW appears
  zero times; one tolerated 80072ee6 WININET_E_INVALID_URL on a side thread precedes it, caught by
  shapr.native.dll); (3) the first composition swapchain round (DXVK dummy window, vkd3d init with
  RAVEN_GUEST_DXVK_CONFIG, torn down unattached); (4) the ThemedImage activation — the app marshaler's
  NotImplementedException (the exception object now carries a "Defa…"-prefixed string, still the in-image
  marshaler gap) → raven reports activate failed 0x80004001, the app continues; (5) ContentPresenter activates
  (NEW, below) and is probed only by the known projected-type candidate-IID batch (IApplication 74b861a1
  refused — honest); (6) the two page-document XBF failures at 0x15df/0x163c, status 80004001 — the
  ThemedImage nodes, so the WelcomePage document still cannot complete and Frame.Navigate is NEVER called
  (the "raven-frame: page OnNavigatedTo" print, ungated, fires zero times in every run); (7) the second
  composition swapchain round, likewise created and torn down unattached; (8) SystemDiagnosticInfo "no class",
  teardown, exit 3. The goal markers remain unreached for the documented reason: SwapChainPanel activation is
  zero, ISwapChainPanelNative/f92f19d2 QIs zero, SetSwapChain zero, vkAcquireNextImageKHR/vkQueuePresentKHR
  calls zero — the panel cannot exist while the page documents fail at the ThemedImage nodes, and that wall
  is inside the app's .NET Native image (three prior rounds traced it; no raven-visible ask around the throw
  is refused). The surfaces implemented, both honest and kept: (a) Windows.UI.Xaml.Controls.ContentPresenter
  added to the runtime catalog (native/nt/registry.cpp → raven-xaml.dll) — the shell was refusing it "no
  class" while raven-xaml's elements factory (which the loader's own create_known path already uses for
  template presenters) accepts the name and builds the engine presenter; one catalog line closed the loop and
  the app's projection now constructs it; (b) Windows.Web.Http.HttpBufferContent in raven-web — a catalog
  entry (→ raven-web.dll) plus content.c's buffer_content_new (the existing content read machinery over a
  caller-owned byte slice: complete from creation, no exchange, Content-Type header of its own, E_BOUNDS past
  the buffer, its own winrt_class so GetRuntimeClassName answers the right name) and http.c's factory
  (IHttpBufferContentFactory bc20c193, the IID the trace showed the projection requesting, with
  CreateFromBuffer/CreateFromBufferWithOffset over the classes[] table; ActivateInstance answers E_NOTIMPL
  like the real parameterless-less class). Fixture suite green after both: 23/23 across the xaml
  application/window/controls/composition/render/dispatcher/input/style/core_window/queue set x64+x32 plus
  native_web_http_x64/x32 (the last exercising the changed raven-web directly). Nothing committed. The two
  removed diagnostics of earlier rounds stay removed; this round added none. Logs:
  /tmp/raven-shapr-probe/tol1.log (no guest DXVK config, composition refused 4x — recorded for contrast),
  tol2.log (config on, both surfaces still absent, same sequence), tol3.log (both surfaces served, full
  mapping, 80072ee6 + POST evidence), tol4.log (clean-state reproduction, identical sequence). App state:
  /tmp/raven-shapr-probe/st80-* (tol1/2's), st81-* (tol3), st82-* (tol4).
- The ThemedImage NotImplementedException is IDENTIFIED — it is the .NET Native marshalling-failure machinery,
  not a raven surface and not app code — and the existing RAVEN_TOLERATE_MISSING_INTEROP hook cracks it: the
  constructor completes, the two page-document XBF failures (0x15df/0x163c) resolve, and the launch moves to the
  struct-marshalling wall (2026-11-15). The identification came from a new TEMPORARY stack scan on the
  RoOriginateLanguageException path (RAVEN_DUMP_THROW_STACK, in native/nt/platform_api.cpp's handler, marked
  temporary for removal — the guest stack with pointer-like values resolved to modules) plus a sibling scan on
  the native e06d7363 path (RAVEN_DUMP_THROW_STACK in seh.cpp, unused this round because the managed throw never
  raises a native C++ exception). The scan of the ThemedImage round (th3) reads, innermost first:
  sharedlibrary+8feadc (the RoOriginateLanguageException call) → mrt100_app 4627f/50b20/3f3b5/3ee33 (the .NET
  throw helpers) → sharedlibrary+c236e9 (the throwing wrapper-build function; the call before it lands in
  sl+7d8200, a packed-table lookup, and the throw helper finally-cleanup shape around it) → marshaler glue
  7ea64d/826e76 → the CCW activation machinery 828b61/7d8214/7dd154 → app frames shapr3d 1260d48, 19fcd13,
  1bf42b4, 2739a6a/2739c69, 28312bd (the TitleBarControl-ctor region) → raven-xaml's activation chain (6b04e
  create_reference → raven-shell+ f72d → the factory). The last raven-visible asks before the throw are ALL
  SERVED — IControl, IFrameworkElement, IAgileObject (94ea2b94), IReferenceTracker (11d3b13a), the peer bridge
  (1791587a), the projected-type candidate batch (IControlTemplate/IGrid/IButton) — addref/release balanced,
  nothing refused, and the message string "The method or operation is not implemented." is created at
  sharedlibrary+848214, INSIDE the MissingInteropData exception factory (sl+847fd0, the helper the tolerate
  patch replaces). The "Defa…"-prefixed string of the previous entry is RETRACTED: the exception object's
  embedded UTF-16 fragments vary per run ("Defa…", "cken", empty in th1) — heap noise beside the object, not
  exception data. The crack, demonstrated on a clean-state launch (th4, RAVEN_TOLERATE_FAILFAST=1 plus
  RAVEN_TOLERATE_MISSING_INTEROP=1, fresh LocalState/LocalSettings): zero "activate ... failed" lines and ZERO
  XBF failures in the whole run — TitleBarControl and ThemedImage construct, every page document loads — and
  the app walks into the navigation-preparation round: DependencyProperty/PropertyMetadata activations, the
  "KeepFocusOnNavigate" string, then the struct-marshalling wall of the 2026-11-07 entry fires again at its
  exact known shape: the marshalled product (a runtime-created generic struct, EETypeRva 0x1b1c0) is zeroed by
  the stand-in, raven's DependencyProperty.Register reads the zeroed TypeName (WindowsGetStringRawBuffer of a
  null Name at raven-xaml+45af0) and answers E_INVALIDARG at xaml_styles.c:688 honestly, the marshaler formats
  "Windows.Foundation.IAsyncAction" strings right after, and RoOriginateLanguageException(80131534
  InvalidCastException, empty message) → RaiseFailFastException — tolerated by RAVEN_TOLERATE_FAILFAST, after
  which the launch idles to the timeout: one PeekMessageW, no dispatcher resume, one worker thread polling
  SleepConditionVariableSRW with a 64 ms timeout forever (the tolerated state is terminal for the launch, the
  same verdict as the 2026-11-07 round, now ~all-XBFs further into the flow). The fail-fast stack scan at the
  80131534 shows the marshal driver at shapr3d.dll+21790dd (the same driver the 21790c8 descriptor dump of the
  2026-11-06 entry examined) over sharedlibrary 889273-889295 glue — the codec-era address shapr3d.dll+2075fae
  appears on the stack as a stale frame, so the recurring fail-fasts at that address are one shared interop
  region, not one call site. Honest state: the ThemedImage wall is identified to the exact helper and the
  XBF/navigation-prep state is reachable, but only under the diagnostic patch whose zeroed struct product
  makes a valid launch impossible (raven cannot supply the app image's missing StructMarshallingStubMap, and
  the Windows-vs-raven branch divergence that creates the dynamic type instantiation remains unidentified);
  Frame.Navigate, OnNavigatedTo and the SwapChainPanel remain unreached. The kept changes are diagnostics only
  (both stack scans env-gated and marked TEMPORARY); no committed changes; the fixture suite passes 23/23
  (xaml application/window/controls/composition/render/dispatcher/input/style/core_window/queue x64+x32 plus
  native_web_http_x64/x32) with them. Logs: /tmp/raven-shapr-probe/th1.log (throw-stack diagnostic first run,
  the e06d7363 gate — no managed-throw hits), th2.log (the RoOriginateLanguageException scan, offsets unnamed),
  th3.log (named scan, the ThemedImage chain above), th4.log (the patched launch: zero XBF failures,
  KeepFocusOnNavigate, xaml_styles.c:688, 80131534, idle tail). App state moved to
  /tmp/raven-shapr-probe/st83-th1..th4-*.
- The synthetic-TypeName stand-in is built, its mechanism verified end to end, and its premise FALSIFIED: the
  zeroed TypeName the Register reads does not flow from the marshalling-failure helper's return value, so the
  "stronger tolerate" cannot reprogram the launch (2026-11-15). What was built (all in the RAVEN_TOLERATE_MISSING_INTEROP
  patch block of native/nt/providers.cpp, env-gated as RAVEN_SYNTHESIZE_TYPENAME, marked TEMPORARY): the shared
  marshalling-failure helper at sharedlibrary.dll+0x8621c0 is replaced (under a paused-threads window — every
  other guest thread is suspended via NativeThreads::suspend for the non-atomic cross-cache-line write, then
  resumed; the first builds' 1007/c0000602 fail-fast storms were the write racing concurrent helper calls) with
  register-preserving guest code that increments a counter word in a host-mapped guest page (R10/R11 saved and
  restored — the .NET runtime's internal ABI does not tolerate clobbered r10/r11; the first counter patch died
  in the GC init; also fixed en route: the lock inc needs REX.B, F0 41 FF 03, not F0 FF 03 which incremented
  [rbx] and faulted), and — when the call's return address is the failing round's site sl+0x86218c (identified
  by single-stepping the round, RAVEN_STEP='CoGetApartmentType:520:20000', plus a 60000-step walk that caught
  the sibling sites 90994c/90997a; a multi-site trampoline in a mapped executable page faulted its own fetch and
  was reverted) AND the count equals RAVEN_SYNTHESIZE_TYPENAME=<N> — returns a real guest HSTRING
  ("Windows.UI.Xaml.Controls.Control") built raven-side in a page pinned at 0x400000000 (a kernel-picked address
  lands beside the thread stacks; a guest-side allocation lands in the .NET GC's reserved ranges; both broke the
  startup). One en-route bug found and fixed: the occurrence-compare's jne displacement was off by one (0x12 for
  0x0f — the 5-byte mov rax,[rsp+16] was miscounted), so every non-matching call leaked RAX and R10 — the
  CoTaskMemAlloc argument trace showed the counter address riding into the guest's own calls. The host-side
  readouts: the originate scan prints the count (platform_api.cpp), and the DependencyProperty.Register refusal
  prints it at the xaml_styles.c:688 note itself (the OutputDebugStringA dispatch in providers.cpp).
  The measurements: the Register refusal fires at count 44 (the failing round is gated call #44; the counts at
  the 80004005/80131534 originates are 40/45). With the synthetic fired at #44 — and at #43, #44, #45 — the
  xaml_styles.c:688 refusal still fires: the TypeName{Name:null} the Register reads is NOT the helper's return
  value at that site (the return feeds a different slot whose consumers tolerate null; firing the synthetic at
  earlier occurrences breaks them differently — Bad optional access / hresult_error(E_FAIL, the synthetic as
  message) from shapr.native). The zeroed TypeName is the MCG marshaller's unwritten OUTPUT BUFFER, exactly as
  the 2026-11-07 entry's mechanism described: the marshalling round whose product would fill it is skipped by
  the null stand-in, and no value the helper returns lands in that buffer. With the corrected offsets the
  counter patch is register-clean and the launch reaches the documented th4 state unchanged (KeepFocusOnNavigate,
  the 688 refusal at count 44, 80131534 InvalidCastException, tolerated idle); the fixture suite passes 23/23.
  The honest verdict: the missing StructMarshallingStubMap data lives in the app's image, the helper-return
  channel is not the corrupted consumer, and the wall stands as raven-unfixable pending a working-Windows
  capture; the tolerate hook (with or without the synthetic) remains diagnostic-only. Nothing committed; the
  kept diagnostics are env-gated and marked TEMPORARY (RAVEN_SYNTHESIZE_TYPENAME, RAVEN_DUMP_THROW_STACK, the
  688-counter print). Logs: /tmp/raven-shapr-probe/sy1..sy22.log and step1..step3.log (the round single-step
  walks that identified the call site), app state st90-*. 
- The pre-commit cleanup pass trims the round-local diagnostics (2026-11-15). REMOVED, all high-chatter
  per-call prints whose removal cannot change behavior: the frame ledger notes in xaml_controls.c
  ("raven-frame: query" per QI in frame_query_any, "raven-frame: addref/release" in frame_addref/frame_release);
  the RAVEN_TRACE_FRAME_QI element-interface ask diagnosis in frame_query_any's refusal branch (the 5-GUID
  candidate table, the gate, and the __builtin_return_address frame walk — the refusal branch keeps its
  "frame has no interface ..." note and E_NOINTERFACE); the "raven-autoprops: Set*" note in
  automation_properties.c's TEXT_PROPERTY macro (the setter is now a straight `return text_set(...)`); the
  "raven-app: add_X reached" note in xaml_application.c's APPLICATION_EVENT macro; the "raven-tracker: query"
  note in reference_tracker.c's tracker_query; the per-call fall-through notes in xaml_metadata.c
  (provider_type_by_name's two "GetXamlType(...) answered no type" prints on the normal overload-fallback
  path, and provider_activate's success-path "xamltype ActivateInstance 0x..." print — the failure note
  "activate %ls failed" stays and now also covers the nameless case); and xaml_loader.c's dead
  `g_last_failure_site` static (declared, never used). KEPT, with the reasons: the env-gated GENERIC machinery
  — RAVEN_STEP / RAVEN_STEP_REGS / RAVEN_STEP_DUMP(2) in providers.cpp, RAVEN_DUMP_CELLS(+RAVEN_DUMP_CELLS_LIST,
  including the ":N" occurrence suffix), RAVEN_DUMP_THROW_STACK in both seh.cpp (the e06d7363 stack scan) and
  platform_api.cpp (the RoOriginateLanguageException scan), RAVEN_TOLERATE_FAILFAST and RAVEN_DUMP_FAILFAST_STACK
  in child_api.cpp, RAVEN_WATCH_EXCEPTION in seh.cpp — all generic, all off unless the env names them; the
  seh.cpp diagnostics inside the RAVEN_TRACE_EXCEPTIONS block (the dead-object dump, the null-IID ask print);
  the RAVEN_TRACE_FRAME_QI-adjacent gated TEMP-FE/TEMP-DP prints in xaml_controls.c and the gated
  RAVEN_TRACE_XBF_KEYS / RAVEN_XAML_SKIP_DEFERRED_CONTENT loader diagnostics (the load-failure notes and the
  "E_INVALIDARG at xaml_loader.c:NNN" contract notes stay); the short failure-path notes that state contract
  decisions (xaml_styles.c:688's refusal note, the frame "found no page" note, the peer-bridge/shim notes on
  the unobserved E_NOTIMPL paths, the "target bound by derived name" note); and the whole
  RAVEN_TOLERATE_MISSING_INTEROP / RAVEN_SYNTHESIZE_TYPENAME patch block in providers.cpp (falsified premise,
  but it is the documented mechanism evidence and stays gated-off). Rebuild: `ninja -C build` clean both widths
  (one pre-existing codec_query.c unused-variable warning, untouched). Suite: the effort's 58-fixture set
  (data_writer/storage/xml_dom/heap/automation/automation_properties/property_value/reference_tracker/
  suppress_transition/mspatcha/xaml_controls + the xaml application/window/controls/composition/render/
  dispatcher/input/style/core_window/queue set, x64+x32, plus web_http/webservices/cross_window/window/queue)
  passes 58/58 (one native_style_x32 flake under `-j8` load, green serially). The FULL 533-test suite has 24
  standing failures pre- AND post-trim — the Skia-less font/text family (dialog_font, rotated_text,
  toolbar_customize, font_families, dwrite, edit_text_contract, dc_attribute, locale_cli, mlang, ucrt,
  twinapi, error_info, load_component, clipboard, d3d10_1, vulkan_compute, socket_messages; the build
  directory's CMake cache has been Skia-less since 2026-11-14, so host GDI text providers do not exist) —
  plus native_system_cli and native_mrt_x64, both green serially (`-j8` load flakes), plus ONE genuine
  anomaly: native_screen_dc_cli passed in the pre-trim snapshot run and fails deterministically afterwards
  (`CreateFontW` unimplemented — the same Skia-less family; the fixture never loads raven-xaml.dll and the
  raven relink used byte-identical objects, so the trim cannot be the cause; its single pre-trim pass is
  unexplained and worth a fresh look with a Skia-provisioned configure). Launch verification with the trimmed
  build (recreated prefix: the rounds' /tmp/raven-shapr-probe/prefix no longer exists; it was rebuilt by
  copying components/shapr3d/prefix, fresh package state): with RAVEN_TOLERATE_FAILFAST=1 +
  RAVEN_TOLERATE_MISSING_INTEROP=1 + RAVEN_GUEST_DXVK_CONFIG the launch reaches the DOCUMENTED
  struct-marshalling wall exactly — RoOriginateLanguageException(80131534 InvalidCastException) from
  sharedlibrary.dll+8feadc → RaiseFailFastException from sharedlibrary.dll+7e38de, tolerated, then the
  terminal idle (one worker polling SleepConditionVariableSRW at 64 ms to the timeout), zero XBF failures in
  the whole run — the same endpoint as the 2026-11-15 th4/sy rounds. The tolerate-free traced launch shows the
  window (ShowWindow), the flags POST exchange, and then the SAME marshalling-failure origin site
  (sharedlibrary.dll+8feadc) with RoOriginateLanguageException(80040111 CLASS_E_CLASSNOTAVAILABLE) and teardown
  exit 3 — the site matches the documented wall, but the HRESULT differs from the th rounds' 80004001 and the
  run stops before the ContentPresenter/SystemDiagnosticInfo stages; given the trim is print-only (verified
  line by line) and run-to-run flow variance is documented for this app, this is recorded as launch-state
  variance under the reconstructed prefix, not a trim regression — the tolerate launch, which defines the
  effort's wall, is identical. Logs: /tmp/raven-shapr-probe/posttrim-{bare2,trace,tol}.log. Nothing committed.
- FINAL STATE of the Shapr3D effort (2026-11-15): the app shows its window and builds its XAML content on
  raven — the whole XBF/page/Style/ThemeResource pipeline runs, TitleBarControl and ThemedImage construct,
  every page document loads under the tolerate hook, the window is shown and receives input, renderer and
  state writing run. The remaining startup wall is INSIDE the app's .NET Native image: a reflection-blocked
  generic struct type (EETypeRva 0x1b1c0, name hash 0xd0e95d24, name deliberately stripped per the
  BlockReflectionTypeMap) whose marshalling data (StructMarshallingStubMap) is absent by design; the app's
  managed branch reaches an interop round needing that data on raven but not on Windows — the divergence is
  inside the app's managed decisions, not a raven surface, and every raven-visible ask around the throw is
  served. The tolerate-bypass family is exhausted with a definitive negative: the marshalled product is the
  MCG marshaller's unwritten OUTPUT BUFFER, not a helper return, so no stand-in return value can reprogram
  the launch (the RAVEN_SYNTHESIZE_TYPENAME mechanism was built, verified end to end, and its premise
  falsified). The routes forward are outside the binary-only investigation: (a) a working-Windows capture of
  the same startup to observe what the managed branch does when the data exists, or (b) the PDB /
  debug-metadata path if the package ever ships it (it does not today). The diagnostic machinery retained for
  future rounds: RAVEN_STEP/STEP_REGS/STEP_DUMP, RAVEN_DUMP_CELLS(+LIST,:N), RAVEN_DUMP_THROW_STACK (both
  copies), RAVEN_TOLERATE_FAILFAST + RAVEN_DUMP_FAILFAST_STACK, RAVEN_WATCH_EXCEPTION,
  RAVEN_TOLERATE_MISSING_INTEROP (+ the falsified RAVEN_SYNTHESIZE_TYPENAME block, gated off), the gated
  TEMP-FE/TEMP-DP/RAVEN_TRACE_XBF_KEYS/RAVEN_XAML_SKIP_DEFERRED_CONTENT prints, and the short failure-path
  notes — all env-gated, all off in a plain launch. Nothing committed.
- The ThemedImage E_NOTIMPL throw is now decoded to the exact managed method and the exact marshalled type:
  the failing managed method is **Windows.UI.Xaml.Controls.IControl__Impl.Stubs.add_IsEnabledChanged**, the
  marshal type it fails on is **Windows.UI.Xaml.DependencyPropertyChangedEventHandler** (the event-handler
  delegate), and the call it was marshalling for is **IControl.add_IsEnabledChanged (flattened vtable slot
  43 = 0x2b) on the raven element peer** (2026-11-15). The decode came from the app's own embedded
  stack-trace metadata — no launch, no raven change, pure offline work on
  components/shapr3d/prefix/drive_c/Program Files/Shapr3D/Shapr3D.dll. The tooling (all under
  /tmp/raven-stmeta/, uncommitted): walk_blobs.py (the blob-region walk), nativeformat.py (the .NET Native
  2.x NativeFormat metadata parser), resolve.py (code address → managed method name), plus the fetched
  corert/dotnet-runtime sources the conventions were verified against. What the formats turned out to be:
  (a) the blob region (PE RVA 0x8316c8 = file 0x830cc8; .rdata RVA 0x1000 = file 0x600) walks cleanly as
  {u32 reserved; u32 id; u32 size; data} tagged blobs — 27 blobs, ids 1-36, and blob **28
  (StackTraceLineNumbers) is NOT in this image** (0xc1fb61 is blob 13's data; the region ends after id 36)
  — line numbers do not exist for this module, only method names; (b) blob 27 is exactly
  corert's StackTraceMethodMappingNode output: flat {uint32 methodRVA (IMAGE_REL_BASED_ADDR32NB — the value
  IS the method entry RVA), uint32 metadata handle} pairs, 102056 entries; (c) the metadata handle is the
  2.x in-memory NativeFormat Handle = (HandleType << 24) | stream offset into blob 26 — high bytes seen:
  0x28 MemberReference (88743), 0x2b MethodInstantiation (13313); (d) blob 26 is a 0xDEADDFFD NativeFormat
  metadata stream (ScopeDefinition count 0 — the stack-trace emission writes no scopes; records are reached
  only through the handles) whose WIRE ENCODING is the corert-era one and NOT the current dotnet/runtime
  main one: handles are compressed-unsigned raw with type = raw &amp; 0xFF and offset = raw &gt;&gt; 8 (the
  HandleType enum differs from main too: 0x28 MemberReference, 0x29 Method, 0x2b MethodInstantiation,
  0x3c TypeDefinition, 0x3f TypeReference, 0x3a ScopeDefinition); and the decisive 2.x quirk: a field whose
  declared type is a SINGLE record type (e.g. a ConstantStringValue Name, a Method Signature) stores a BARE
  compressed offset with the type implied, while union fields (TypeDefOrRef, MethodDefOrRef) store
  (offset&lt;&lt;8)|type — verified three-for-three by decoding MemberReference records whose name offsets
  land exactly on length-prefixed strings ("get_Value", "GetVtbl", "get_Value__n"). Compressed integers,
  strings (compressed length + UTF-8), and collections (count + elements) match the published
  NativePrimitiveDecoder. The EEType name-hash convention was verified FOUR for FOUR at +0x14 with corert's
  ComputeNameHashCode: shapr3d+0x5d930 = 0xfdb30aca = Windows.UI.Xaml.DependencyPropertyChangedEventHandler,
  shapr3d+0x1ae80 = 0x1287af62 = Windows.UI.Xaml.Controls.IControl, and the two known RVAs 0xdfac0 = 0x42b522b5
  = Shapr3D.App.WelcomePage and 0x1e688 = 0x687c66a7 = Windows.UI.Xaml.Controls.IPageOverrides — so
  EETypeRVA → name is now available offline for any RVA, not just for stack-trace-mapped methods.
  The chain itself, resolved from the th3 originate scan's raw values through .pdata function containment
  (122759 functions; 20712 of them have NO stack-trace metadata entry — the emission policy excludes them,
  which is why the scan's 1260d48/7f9754/7f974d values are .rdata DATA pointers, not code) and blob 27:
  innermost __Interop.ComCallHelpers.Call (shapr3d+19fcc50, ret 19fcd13) ← the generated stub
  IControl__Impl.Stubs.add_IsEnabledChanged (shapr3d+1bf4260; its body loads the delegate EEType 0x5d930
  and interface EEType 0x1ae80 into .rdata handles, calls a marshal import cell with (handler, delegate
  handle), then calls ComCallHelpers.Call(this, ifaceHandle, slot 0x2b, marshalled-handler, out-token))
  ← a 15-byte trampoline (fn+2739c60) ← the generated event-wiring function (fn+27398c0, which also loads
  the handler body at 2739c70 and passes it as a function pointer — the compiled Connect()-style
  InitializeComponent wiring of ThemedImage's ctor path) ← an interface-dispatch glue stub (fn+2a48e10,
  the this->[0x60] interface-map walk) and a lazy-init getter (fn+2b1d130) ← IXamlType__Impl.Vtbl.
  ActivateInstance__n (shapr3d+1bf70a0) ← raven-xaml's activation chain (6b04e create_reference →
  raven-shell f72d). The sharedlibrary frames between the stub and the originate (7d8214/7dd154/828b61/
  826e76/7ea64d and the wrapper-build c236e9 over the sl+7d8200 packed-table lookup) are the argument
  marshal inside ComCallHelpers.Call: the throw happens while marshalling the HANDLER DELEGATE argument —
  BEFORE any call into the raven object's slot 43 — so raven's IControl face layout is not the trigger of
  THIS throw. What this changes for the next increment: (1) the 2026-11-14 "Dispatcher"-string reading is
  superseded — the wall is the IsEnabledChanged event subscription in the ThemedImage ctor, not the
  Dispatcher-typed DP; (2) the ctor consults a real raven EVENT surface: when the marshal succeeds (or
  under the tolerate hook) the call lands on flattened IControl slot 43/44 (add/remove_IsEnabledChanged),
  which raven's element faces should implement honestly (raven's current 43-50 event pairs are the
  IFrameworkElement table — a different interface's flattening); (3) the missing data is again in-image
  delegate-marshalling data for DependencyPropertyChangedEventHandler (the image has no
  DelegateMarshallingStubMap blob, per the 2026-11 blob decode), so the wall itself stands as
  raven-unfixable in the same sense as the struct-marshalling wall — but the exact type is now named, so a
  gated trace can check whether the marshal machinery makes any raven-visible ask for the delegate's
  interface before throwing, and a working-Windows capture can now be diffed by METHOD NAME instead of by
  raw offsets. Spot checks of the resolver across the image (System.Linq iterators, List`1.set_Item,
  Shapr.CommonAppWin.IMTModelEditor__Impl.Vtbl.remove_DrawingCreationInitiated__n, a
  KeyValuePair<...,ProjectStatusChangedDelegate>__Impl Vtbl record) all decode cleanly — the parser is
  general, not fitted to this one stack. Nothing committed; the tooling lives under /tmp/raven-stmeta/
  (walk_blobs.py, nativeformat.py, resolve.py, probe scripts, and the fetched corert/dotnet-runtime
  reference sources); the fixture suite was not run (no repo code changed); no launch was performed.
- The stub's slot 0x2b is reconciled against the winmd and raven's IControl face is aligned with an honest
  add/remove_IsEnabledChanged pair (2026-11-15). The slot arithmetic the 2026-11-15 decode left open — the
  stub calls flattened slot 43 but the assumed IFrameworkElement+IControl flattening puts add_IsEnabledChanged
  past 53 — resolves because **the assumed flattening does not exist**: in the Windows.UI.Xaml.winmd the app's
  MCG was built against (the wine-uwp System32/WinMetadata copy, parsed with dnfile), the UWP XAML interfaces
  carry **only their own members** after the six IInspectable methods — IControl's Extends column is null
  (0x0 in the raw TypeDef row; the dnfile-coded-index resolution is broken and answers the same wrong row for
  every interface, so the raw bytes are the evidence), IFrameworkElement's 47 own members (37 property
  accessors, the Loaded/Unloaded/SizeChanged/LayoutUpdated event pairs at 43-50, FindName, SetBinding) sit in
  the IFrameworkElement face alone, and IControl's own list is **41 members, not the 13 the previous entry
  assumed** (FontSize through Focus: 19 put/get pairs, get_FocusState, the IsEnabledChanged pair,
  ApplyTemplate, Focus). The convention was calibrated in-image from three independent stubs (the resolver's
  code→name map plus capstone over each stub's `mov $imm,%r8d` before the ComCallHelpers.Call):
  IAutomationPeerOverrides.GetPeerFromPointCore own-index 25 → slot 31, ICollectionView.LoadMoreItemsAsync
  own-index 16 → slot 22, IControl.add_IsEnabledChanged own-index 37 → slot 43 — every one is **6 +
  own-index**, zero divergence, so slot 43 = the stub's add_IsEnabledChanged exactly (remove = 44).
  The "current 43-50 event pairs are the IFrameworkElement table" worry from the previous entry dissolves
  with it: IFrameworkElement's own face legitimately has its four event pairs at 43-50; the same numbers in
  the IControl face are the IsEnabledChanged pair — different interfaces, no conflict. raven's
  windows_ui_xaml_control.h already declared this honest 47-entry table (a textual count of the struct
  confirms get_FontSize 6, get_FocusState 42, add_IsEnabledChanged 43, Focus 46) and both implemented faces
  follow it, so the layout needed no move; what was dishonest were the slot pair's bodies: the E_NOTIMPL
  stubs. Both are now real event storage over the shared machinery: `frame_control_add_IsEnabledChanged` /
  `_remove_IsEnabledChanged` in xaml_controls.c over a new `is_enabled_changed` raven_event_list on the frame
  struct (CS initialized with `navigated`'s, closed and deleted with it), and `control_is_enabled_changed_add`
  / `_remove` in xaml_elements.c over the same on the button struct (the element-peer IControl face); the
  handler arrives as the marshal's CCW pointer and is retained the way Click and Navigated retain theirs.
  New env-gated TEMPORARY diagnostics (clearly marked, off in a plain launch): `RAVEN_TRACE_FRAME_CONTROL=1`
  prints the frame's IControl QI and the add call's handler value; the suite (36/36 xaml/style/window/
  composition/render/dispatcher/input/queue + 24/24 core application/window/desktop/presentation + xaml
  application/window/controls/input, a superset of the 50/50+9/9+controls baseline) passes on the final
  build. The launch evidence is honest about the wall NOT moving: a fresh-state launch
  (LocalState/LocalSettings/LocalCache moved to /tmp, no tolerate hooks — the shape that reaches the page)
  reproduces the documented failure verbatim — XBF failures at nodes 0x15df and 0x163c with status **80004001**
  (the two control nodes), then `raven-frame: page OnNavigatedTo 0x80004003` — and with
  `RAVEN_TRACE_FRAME_CONTROL=1` **the gate never fires**: no IControl QI reaches raven's frame face (and with
  the tolerate hooks on, the whole ThemedImage phase is not even reached — see below). This is the first
  direct raven-side confirmation that the throw happens before ComCallHelpers.Call resolves the interface —
  the marshalled-handler argument fails and the slot pair is never entered, so the alignment's value is
  prospective: when the delegate-marshalling data question is resolved (the working-Windows capture diff, or
  a future tolerate that fills the handler buffer), the call will land on S_OK+S_OK storage, not E_NOTIMPL.
  The decode's own caveat stands: the DependencyPropertyChangedEventHandler marshalling data is absent from
  the image by design (no DelegateMarshallingStubMap blob), so raven cannot serve this round from the guest
  alone. Two launch-environment findings for the next round: (a) the tolerate hooks are now a regression, not
  a probe — with RAVEN_TOLERATE_MISSING_INTEROP the app's Application.Start callback takes the 80131534
  fail-fast at sharedlibrary+7e38de (the fail-fast-stack decode: IApplicationStatics__Impl.Stubs.Start →
  ComCallHelpers.Call → raven-xaml's statics_Start → dispatcher_process_events → the app's
  DispatchedHandler__Impl.Vtbl.Invoke__n → WinRTCallbacks.PostToWinRTDispatcher → the marshal failure) much
  earlier and in the tolerate-on runs the ThemedImage/XBF phase is never reached at all, while without the
  hooks the same build walks the catalog and builds the WelcomePage; (b) launches are NOT deterministic
  across state clears — the app's remote experiment config (app_welcome_redesign_test, app_compat, visible
  in the exception-context dump with "isRemote":true) branches the startup, and this session's runs
  consistently took a branch that never loads Shapr3D.UILibrary/ThemedImage.xbf, where the previous
  session's th4/sy launches did (57408 GetXamlType-string compares); first API-sequence divergence sits in
  the window-class registration count. Logs: /tmp/raven-shapr-probe/launch-ti1..ti11.log (ti11 = the
  no-tolerate reproduce with the silent gate; ti3 = the same shape without the gate; ti6 = tolerate-on
  divergence). Nothing committed.
- The remote-experiment-config hypothesis is CLOSED: the flagsmith exchange was captured end to end and the
  startup branch is byte-identical with and without the remote config, so the branch that reaches the
  ThemedImage/TitleBarControl XBF nodes is NOT driven by `app_welcome_redesign_test` (2026-11-15). The
  exchange, now fully traced: the app first POSTs a gzip-compressed Amplitude-style tracking event
  (`{"event_properties":{"reason":"new_install"},"tracking_id":<random-per-launch GUID>,...,"internet_connection":"none",...}`,
  443 bytes compressed) and then the flagsmith identities call —
  `POST https://edge.api.flagsmith.com/api/v1/identities` with body
  `{"identifier":<GUID>,"traits":[{"trait_key":"app_version","trait_value":"26.200.12220"},{"trait_key":"first_app_version","trait_value":""},{"trait_key":"platform","trait_value":"windows"},{"trait_key":"type","trait_value":"tracking"}]}`
  — through raven-web's WinINet transport; the response is one 13,703-byte JSON document (one `InternetReadFile`
  chunk; the "0x20000 + 0x1ca9b" two-call pattern in the older logs is one data read plus the end-of-body read,
  the second argument being the remaining buffer capacity, not the body size) listing **71 flags**, of which 37
  are `app_*`. The LIVE server (2026-11-15) returns for the five flags the app's native remote-configuration
  layer knows (strings + variant names `notEligible`/`control`/`treatment` live in Shapr.Native.dll's
  `Shapr::RemoteConfiguration` table): `app_compat` = 1 (Variant_1, "SPLIT; weight=100.0"), `app_new_tool_ux_test`
  = 0 (DEFAULT control), `app_non_trial_instant_trial_test` = 0 (STATIC, disabled),
  `app_instant_trial_feature_highlight_test` = 2 (Variant_2, weight=50), `app_welcome_redesign_test` = **2**
  (Variant_2, "SPLIT; weight=100.0") — exactly the values the sy-session launches' in-app parsed config showed
  (`{"app_compat":{"value":1,"isRemote":true},"app_welcome_redesign_test":{"value":2,"isRemote":true}}`), so the
  server has not drifted for these flags and the 100%-weight split makes the value deterministic per identity
  anyway. The instrument (TEMPORARY, env-gated, uncommitted, in `native/guest/raven-web/transport.c`): 
  `RAVEN_GUEST_RAVEN_WEB_EXCHANGE_DUMP=<win-dir>` (note the `RAVEN_GUEST_` pass-through — a bare host var never
  reaches the guest) appends every request body and response chunk to `<dir>/request-N.bin`/`response-N.bin`
  (the captured live response: `components/shapr3d/prefix/drive_c/dump/response-2.bin`), and
  `RAVEN_GUEST_RAVEN_WEB_REFUSE_HOST=<substring>` makes every request whose host contains the substring fail
  with WININET_E_CANNOT_CONNECT (80072efd) before any network touch — the offline machine. The verdict comes
  from four fresh-state launches (LocalState/LocalSettings cleared each time): traced online (fl1/fl2), traced
  offline (fl3) and untraced online/offline (fl4/fl5). The app catches the refused flagsmith POST
  (RoOriginateLanguageException 80072efd + FormatMessage from shapr.native.dll, its own error logging) and
  continues; the traced offline run takes the same branch as the traced online runs (22 RegisterClassW, the same
  GetXamlType catalog walk touching ThemedImage.xbf, the same two XBF failures at nodes 0x15df/0x163c status
  80004001, natural exit 3 at the 90-second window); and the untraced pair is **byte-identical** (diff of the
  two complete logs: zero lines): both fail-fast 80004003 at the documented marshaler stack
  (sharedlibrary+7e38de → shapr3d.dll+2075fae) as the very first event and exit 2147500035. This also resolves
  the 2026-11-15 "launches are not deterministic across state clears" finding (b): the divergence was an
  instrumentation artifact, not a config branch — the sy-session logs were RAVEN_TRACE_API-traced and the ti-session
  logs were not, so the compared "window-class registration counts" came from different tracing, and which wall
  appears (the ThemedImage-phase XBF failures with a slow traced run idling to exit 3, vs the immediate
  Application.Start dispatcher-callback fail-fast in a fast untraced run — the two race each other on different
  threads, and ti3/ti11 caught an interleaving where both fired) is a thread-timing matter. No pin is needed: a
  canned flagsmith response would change nothing, the offline path IS the online path, and both end at the
  app's in-image walls. In the traced runs the app reads the full response body into a buffer (read_more over
  the exchange) but the reduced in-app config JSON never surfaces through any traced HSTRING API in the current
  build (it did in sy3's), so whether the config is applied before the wall remains unobserved — moot for the
  branch question given the byte-identical offline/online pair. Suite state: the 43-test xaml/style/window/
  composition/render/dispatcher/input/queue + core application/window/desktop/presentation + web subset passes
  43/43 before and after the transport change. Nothing committed; logs /tmp/raven-shapr-probe/launch-fl1..fl5.log,
  ctest-baseline-fl.log and ctest-after-fl.log, cleared state under state-fl1..fl5, dumps under the prefix's
  drive_c/dump.
- The working-vs-failing delegate-marshalling asymmetry is DECODED to the helper chain, and the verdict is
  that hypothesis (a) is dead and (b) is confirmed in shape: the two marshal routes are ONE route and the
  split is data-driven (2026-11-15). The stub comparison: IControl__Impl.Stubs.add_IsEnabledChanged
  (shapr3d+1bf4260) and IApplication__Impl.Stubs.add_Suspending (shapr3d+1a754d0) are byte-for-byte the
  same shape — load the delegate EEType (0x5d930 DPCEH vs 0x5d320 TypedEventHandler<Application,
  SuspendingEventArgs>, hash-verified) and the interface EEType (0x1ae80 vs 0x5ad30), call the SAME import
  cell shapr3d+1366b88 with (handler, delegate EEType), then ComCallHelpers.Call (0x19fcc50) with slot
  0x2b/0xd. The import cell resolved LIVE (RAVEN_DUMP_CELLS anchored at RoOriginateLanguageException,
  launch-mr1/mr4) to sharedlibrary.dll+0x86a850 — the NEAT encoding is validated end to end: the file value
  0x8000000000000000|X indexes SharedLibrary's $NEAT$ export table (sl+0x1a2b20, 4-byte function RVAs) as
  X*4, resolved at module init by sl!InitializeModules; the sibling cells 1366b40/1366b50 resolve to
  sl+0x828e30/0x9e6680 the same way. The marshal chain: sl+0x86a850 → per-type cache lookup 0x86a8f0 over
  the context global [sl+0x55fb88] (built at init from static descriptor sl+0x79fc0, 9 entries; the hash
  table observed LIVE to be entirely empty — every bucket -1) → gate 0x86a8a0 → 0x8620c0, whose arg is the
  DELEGATE EEType and which special-cases System.Object (sl EEType 0xdfa8, name-hash verified) and throws
  the MissingInteropData helper 0x8621c0 (the sy round's site) when the type is System.String (0x11ed8) or
  equivalent → 0x863d20 (interface-map-needed gate: not-flag, not-flag, not-equivalent-to-sl-Delegate
  0x107e8) → cache lookup 0x826f30 over the SECOND context [sl+0x55fb98] (descriptor sl+0x7a270, 3
  entries) → 0x862270(handler, info) → the per-object descriptor via cell [0x55c140](CCW, sl-EEType
  0x1c8f8, hash 0x4d8885c2 unnamed) → the record walk 0x82d8d0 (PLAIN POINTER compare 0x807b00 against the
  record's EEType, equivalence fallback [0x55c160] only when neither handle is runtime-created per
  0x7ea670) and then the list walk 0x82d6e0 over the descriptor's +0x50 list (fetched by 0x82d870 through
  the descriptor's own +0x50 cache cell). What the images contain: SharedLibrary carries ~180 static
  descriptor objects of the 0x1c8f8 class (the framework's marshalling records — hypothesis (b)'s list,
  generalized); the APP image carries ZERO objects of that class. The app image DOES contain its own
  per-delegate marshalling records (app+0x396ea0 for DPCEH, +0x395dd0 for TH-inst: {EEType, flags
  0x00010002, EEType, NEAT thunk cell} pairs under a cache-descriptor header {magic 0x3004a40000, shared
  cell 0x136dd70, count word 0x0002000d, type hash, stub pointers}, plus hash-table-shaped {EEType, u32
  hash} arrays at 0x7aab38/0x7aaf98) — the MCG emitted the DPCEH stubs inline
  (DependencyPropertyChangedEventHandler__Impl.Vtbl.GetVtbl at shapr3d+1bf9840) — but nothing in the
  failing lookup consults them. The decisive asymmetry: both app delegate EETypes carry the SAME related-
  type cell (EEType+8 → 0x136dd70 → NEAT → sl+0x10738), and sl+0x10738 is the framework's DELEGATE-family
  descriptor keyed on sl System.Delegate 0x107e8 with hash field 0x348a5515 = System.MulticastDelegate and
  count 12 — the framework side has entries for the FRAMEWORK's delegate shapes only; a static PE cannot
  hold cross-module pointers, so no SharedLibrary descriptor can ever name the app's own non-generic
  delegate type. TypedEventHandler<Application,SuspendingEventArgs> marshals because generic
  instantiations ride the framework's generic-definition machinery with runtime type equivalence;
  DependencyPropertyChangedEventHandler is a non-generic custom delegate whose marshalling entry can only
  come from the app's own DelegateMarshallingStubMap (blob 17 — absent, verified) or the app's static
  records — both unreachable from the lookup that fails. The wall stands as raven-unfixable in the same
  sense as the struct-marshalling wall, and the divergence question (why Windows runs this round) needs
  the working-Windows capture; no raven-side patch is justified while the exact failing equality (record
  pointer vs requested handle inside the 0x82d8d0/0x82d6e0 walks) is not observed live. One more observed
  piece: the DispatchedHandler marshal inside Application.Start — the SAME chain — is flow-dependent on
  raven: most fresh-state launches fail-fast 80004003 at sl+7e38de through IApplicationStatics__Impl.
  Stubs.Start → ComCallHelpers.Call → raven statics_Start → DispatchedHandler__Impl.Vtbl.Invoke__n →
  WinRTCallbacks.PostToWinRTDispatcher (resolve.py names: 1a73b15, 19fcac5, 1becf39, 1b55c4d, 2075fae)
  BEFORE the ThemedImage phase, while the ti11/fl1 flows ran past it — so the ThemedImage E_NOTIMPL anchor
  fires only in some interleavings (0/4 untraced fresh-state runs, 0/3 traced 60-second runs; the traced
  runs idle after the tracking POST and tear down without reaching the XBF walk). Diagnostics kept (all
  env-gated, marked TEMPORARY): RAVEN_DUMP_MARSHAL_CONTEXT=1 (walks [sl+0x55fb88]'s context, hash table,
  table2/table3 and the app's four delegate/interface EETypes at the RoOriginateLanguageException anchor;
  also the [0x55f470]+0x20 registry read), and a stack-deref walk inside the RAVEN_DUMP_THROW_STACK block
  (dereferences each pointer-like stack value 0x60 bytes, 96 max) — the provider.cpp additions sit beside
  the existing cells dump. Fixture suite 51/51 for the xaml/window/queue/dispatcher/composition/render/
  input/style/controls/application set (one native_cross_window_x32 flake under -j4, green serially).
  Nothing committed. Logs: /tmp/raven-shapr-probe/launch-mr1..mr10-3.log; tooling /tmp/marshal-decode/
  (common.py PE+objdump helpers, sl_text.asm full SharedLibrary disassembly, MetadataBlob.cs the fetched
  ReflectionMapBlob enum) beside the prior /tmp/raven-stmeta/ tooling.
- The second trim pass removes the mr-round and ti-round diagnostics whose questions are answered (2026-11-15). REMOVED, all print-only, all off in a plain launch, none load-bearing: RAVEN_DUMP_MARSHAL_CONTEXT in providers.cpp (the whole ~115-line block — the walk of sharedlibrary's [sl+0x55fb88] marshalling context, its hash table, table2/table3 and the four hardcoded shapr3d.dll EEType offsets; Shapr3D-round-specific and NOT trivially foldable into the generic RAVEN_DUMP_CELLS machinery, whose purpose is cell resolution, not pointer chasing); the stack-deref walk inside platform_api.cpp's RAVEN_DUMP_THROW_STACK (the one-level dereference of up to 96 stack values, 0x60 bytes each — the base stack scan, the resolve() helper and the synthesize-counter read stay); the exception-object dump after the originate stack scan in platform_api.cpp's RoOriginateLanguageException (the 0x100-byte object dump with describe_text pointer chasing — the pre-existing RAVEN_TRACE_EXCEPTIONS message-only print stays; describe_text itself keeps its other callers); the rbx-object read in seh.cpp's c0000005 block (printed "rbx object len N" for a candidate that is usually NOT a nativeaot string — non-string garbage); the RAVEN_TRACE_FRAME_CONTROL machinery in xaml_controls.c (the control_qi_gate, both "TEMP-CONTROL" prints in frame_control_add_IsEnabledChanged and the IControl QI branch — the branch keeps a short contract note) and its element-peer twin in xaml_elements.c's control_is_enabled_changed_add; the TEMP-FE getter-trace machinery in xaml_controls.c (framework_trace_gate/RAVEN_TRACE_FRAME_FE and the per-get prints in the three FRAMEWORK_*_PROPERTY macros and the ActualWidth/ActualHeight getters — the "does the generated ctor read the extent getters through this face" question is answered). RENAMED to permanent, kept gated: the TEMP-DP GetValue tracing in xaml_controls.c is now the contract diagnostic RAVEN_TRACE_FRAME_DP (print prefix "raven-frame frame-dp:", comment no longer round-local — it shows which DP a generated ctor reads and whether raven answers metadata or none, the surface the Dispatcher-DP default contract rides); the TEMP-QI refusal print in the page inner's QI is now "raven-page: refused QI" under RAVEN_TRACE_PAGE_QI (minimal, prints only the refused IID — the refusal list is the page-face contract as generated callers see it). KEPT unchanged: the generic env-gated machinery (RAVEN_STEP/STEP_REGS/STEP_DUMP(+2), RAVEN_DUMP_CELLS(+LIST, the ":N" occurrence suffix), RAVEN_DUMP_THROW_STACK in both seh.cpp and platform_api.cpp — seh.cpp's comment no longer says "removed after the round", RAVEN_TOLERATE_FAILFAST/DUMP_FAILFAST_STACK, RAVEN_WATCH_EXCEPTION, RAVEN_TOLERATE_MISSING_INTEROP + the falsified RAVEN_SYNTHESIZE_TYPENAME block); seh.cpp's dead-object dump and null-IID ask print; xaml_loader.c's RAVEN_TRACE_XBF_KEYS (comment's stale "removed after the round" dropped) and RAVEN_XAML_SKIP_DEFERRED_CONTENT; transport.c's RAVEN_WEB_EXCHANGE_DUMP and RAVEN_WEB_REFUSE_HOST (stale "remove with the Shapr3D remote-config round" comments reworded to "kept for future protocol work", print prefixes lose TEMPORARY). Honest note: the task list also named an `if (false && !GetEnvironmentVariableA(...))` unconditional-diagnosis leftover in xaml_controls.c — no such block exists in the current tree (repo-wide grep over native/ for `if (false` returns nothing), so there was nothing to remove; the element-interface question it served was already answered and its prints evidently went with the first trim's RAVEN_TRACE_FRAME_QI removal. Rebuild: `ninja -C build` clean both widths (only pre-existing warnings: codec_query.c unused variable, loader unused statics); deploy refreshed components/shapr3d/prefix/drive_c/windows/system32/raven-xaml.dll + raven-web.dll and the syswow64 twins. Suite: the 44-fixture set (xaml application/window/controls/input, style, composition, render, dispatcher controller/idle/queue, core_window/core_application, application_view, cross_window, queue, timer_queue, window_cli/hooks, web_http; x64+x32 where both exist) passes 44/44 serially. Launch verification on the trimmed build under the running GL weston (wayland-99, --renderer=gl, PID 8424): (a) plain untraced fresh-state launch (RAVEN_PREFIX set, --timeout-ms=60000, no tolerate hooks) reproduces the fl4/fl5 endpoint verbatim — fail-fast 80004003 as the first event from the documented marshaler stack sharedlibrary.dll+7e38de → shapr3d.dll+2075fae, exit 2147500035; (b) the RAVEN_TRACE_DEBUG=1 launch takes the ThemedImage-phase interleaving and shows the ti-round signature exactly — the two XBF failures at nodes 0x15df and 0x163c with status 80004001 (the two control nodes) plus the same fail-fast 80004003 (the documented thread-timing interleaving where both fire); (c) the RAVEN_TRACE_API=1 traced run shows the traced shape (22 RegisterClassW, ShowWindow, offline 80072ee6 origins from shapr.native.dll, no fail-fast, natural exit 3). No "TEMP-" print fired in any run. Nothing committed. Logs: /tmp/raven-shapr-probe/launch-posttrim2.log, -traced.log, -debug.log; ctest /tmp/ctest-posttrim2.log.
- The delegate-marshalling data question is settled by a full offline decode, with one decisive correction: the DependencyPropertyChangedEventHandler marshalling data EXISTS in the app image — the "absent by design / no DelegateMarshallingStubMap blob" verdict of the two previous rounds is FALSIFIED for this toolchain, because .NET Native 2.x's MCG never emitted marshalling maps as blobs (blob 16/17) at all; it emitted per-type static DESCRIPTOR OBJECTS, and DPCEH's descriptor, records, CCW vtable and GetVtbl glue are all present (2026-11-15, pure offline work, no launch, no raven change). Tooling under /tmp/marshal-decode/ (common.py now also serves SharedLibrary via `-sl`; new scripts inline below). The round's findings, numbered:

  (0) TOOLING ARTIFACT, fixed: /tmp/marshal-decode/sl_text.asm had been disassembled with --adjust-vma=0x668000 instead of the real .text RVA 0x669000, so EVERY address in it is real-RVA-0x1000 (code and data alike: its 0x86a850 is real 0x86b850, its "0x55fb88 cache writer / 0x79fc0 descriptor" is the real 0x560b88-writes-from-0x7afc0 init function 0x6738e0). Any previous-round statement that mixed stale-asm addresses with live-stepped addresses is unreliable; the live single-stepped values remain valid. The file has been regenerated from the prefix's SharedLibrary.dll (md5 b505e1f8... — the system32 copy and dependencies/Microsoft.NET.Native.Framework.2.2/SharedLibrary.dll are byte-identical) and now anchors correctly (0x86a850 = push rbx/rdi; mov rdx->rbx, rcx->rdi; the marshal entry).

  (1) The 27 app blobs are now identified. Blob 13 = EmbeddedMetadata (head 0xDEADDFFD); 26/27 = the stack-trace pair (27 is exactly 102056*8 bytes); 30 = NativeLayoutInfo (contains the MCG's "InvokeRet..." helper-name strings); 31 = NativeReferences — a list of tagged RVAs (0x81xxxxxx form) that POINTS INTO the app's interop-record region: it references the shared cell 0x136dd70/0x136dd78 and, exactly once each, the two delegate descriptors 0x5d930 (DPCEH) and 0x5d320 (TypedEventHandler<Application,SuspendingEventArgs>) beside their sibling hash-table chunks (0x7aaa70..0x7aab98, 0x798a20..0x798b28) — this is the type-loader linkage data for runtime-created types, not a marshalling map; 32/34/35 = the three u32 hash/offset hashtables (Generics/StaticsInfo/GenericMethods per the modern enum), 33 = NativeStatics (rva pairs into the stub region), 36 = ExactMethodInstantiationsHashtable, blob 0 = a small u32 table, and 5/14/15/23 = small u32 offset/hash map tables (unclassified, all far too small and none marshalling-shaped). NO blob id 1-36 is a DelegateMarshallingStubMap under a renamed id; no blob 16/17 exists. Conclusion: the 2.x interop data lives in static objects, not blobs.

  (2) The descriptor object format (the MCG 2.x interop record): {qword0 magic 0x0000003x_xxxx0000 (kind flags), shared-cell pointer, {u32 subfield|count, u32 name-hash} at +0x10, 0, count stub pointers, tail cells/self-pointer}. This also explains the stmeta round's "+0x14 name hash" verification: descriptors carry the type-name hash at +0x14 exactly like EETypes do, so "EETypeRVA → name" worked — but the objects at app 0x5d930/0x5d320 named "the delegate EEType" in the previous entries are NOT EETypes; they are the delegate MARSHAL DESCRIPTORS (an EEType looks like sl System.Object 0xdfa8: {0x1801000000, 0, {3, hash}, 0, interface-map ptr}; app WelcomePage 0xdfac0: {size 0xc8|flags, related 0x1e720, {26, hash}, 0, stub-region ptrs}). The stub's "delegate EEType handle" argument IS the descriptor — the marshal machinery receives the app's own marshalling data directly.

  (3) What the DPCEH data consists of (all in-image): descriptor 0x5d930 = {magic 0x3000240000, cell 0x136dd70 -> NEAT[0xb706] -> sl+0x10738 (the framework delegate-family descriptor), {12, hash 0xfdb30aca}, 12 stub pointers: 0x182c62528/0x182c63858/0x182c63860 (shared), 0x181d3eed0 (the DPCEH-specific Invoke stub), 0x182c63868/0x182c63870, ...}; the Kind1+Kind2 record pair at 0x396ea0 ({0x18005d930, 0x10002, 0x18005d930, cell 0x136e2a0|1, 0x102, -0x20, 8, 1} + cache-descriptor {magic 0x3004a40000, cell 0x136dd70, {13, hash 0xc4de75ef}, 11 stubs, back-pointer, cells 0x136d928/0x136e248, tail {0x00fd62b1, 0xffffff2c}}); the static CCW vtable at 0x1374ff0 (entries 0x182bfe4f0..0x182bfe508) returned by DependencyPropertyChangedEventHandler__Impl.Vtbl.GetVtbl (app+0x1bf9840) through lazy-init cell 0x1366b70 -> NEAT[0x2fbf] -> sl+0x82d1d0 (which is literally `rax=*rdx; if(!rax){rax=rcx;*rdx=rcx}` — a get-or-init cell); and the data-only registration tables: hash chains {desc,hash,desc} at 0x7aab20 (DPCEH present) and 0x7aaf80 (TH present) and the ~628-row {desc, cell->sl+0x7edce8, desc, cell, name-hash} table at 0x7c8cc8, whose shared cell sl+0x7edce8 is the InvalidOperationException(0x80131500, "The method or operation is not implemented.") constructor glue. None of these tables is referenced by any code rip-relative — they are reached through data chains only.

  (4) SharedLibrary's side, fully decoded at real addresses: sl carries 103 strictly-shaped same-format records (magic family 0x0000003x_xxxx0000: 0x3000200000/0x3000300000/0x3001200000/0x3004200000/0x3004a00000/0x3005200000/0x3800200000; delegate-family cluster 0x10628-0x10aa0: descriptors 0x10738 {ref 0x107e8, {12, hash 0x348a5515}} -> 0x107e8 {ref System.Object 0xdfa8, {12, hash 0x39854ad9}} -> Object, plus per-shape delegate descriptors 0x10628/0x10960/0x10a08 with hashes 0x711f4e77/0xcc6986ef/0xb753c5f6). The marshal entry sl+0x86a850(handler, descriptor) -> 0x86a8f0(descriptor) -> 0x826f00 -> hash lookup over the cache object at [sl+0x55fb80] (buckets int[] at cache+0x28 via 0x826d50/0x826da0/0x826de0; entries array at cache+0x8; 0x827030 copies a 0x40-byte record out) — the cache is built at sl init from sl's OWN static records only (module registered with header .rdata 0x1000 at 0x68f5d0; cache-build cluster 0x6738e0 etc.; NO code path inserts app descriptors, and all 31 code references to the cache are readers or cctor inits) — so the lookup misses for ANY app descriptor, on Windows and raven alike. Then 0x86a8a0 (gate), 0x7e4640, 0x8620c0(handler, descriptor): System.Object (0xdfa8) and String (0x11ed8) special cases via 0x7ea670 runtime-created checks and mrt100 RhTypeCast_AreTypesEquivalent [0x55c160] -> gate 0x863d20 (helper 0x845700 = `testw $8,0x2(%rcx)` flag test, then 0x85c780(descriptor, 0x107e8) equivalence) -> for the delegate path 0x862270(handler, descriptor): sl statics [0x55f470+0x20] (the cctor descriptor 0x5672a8), equivalence checks, record walk 0x862f00, RhTypeCast calls -> 0x862322: RhTypeCast_IsInstanceOfClass [0x55c140](handler, sl CCW-class EEType 0x1c8f8) -> on an existing CCW: the CCW's interface-records array ([ccw+0x18]+0x8, 3-qword stride), 0x807b00 hash-lookup by descriptor, 0x82d8d0 per-record memo compare ({flag at +0x10, cached type at +8 == requested, AreTypesEquivalent fallback when neither handle is runtime-created}) and the 0x82d6e0 list walk, miss -> 0x828130 CREATES the interface record; on NO CCW (a fresh delegate object) the fallback 0x862460 -> 0x8244f0/0x8624b0/0x862550 creates the CCW and converts. The MissingInteropData exception object (HResult 0x80131622, compared at 0x8fe772) is built by 0x8621c0 (callers 0x862187 String-branch, 0x8fe7a2, 0x8fea91, 0x909947/0x909975 — the sy-round sites — and others), and the delegate wall's originate wrapper at sl+8feadc formats (hresult from [ctx+0x30], message, exception) into RoOriginateLanguageException.

  (5) The asymmetry this pins: TypedEventHandler<Application,SuspendingEventArgs>'s marshal through the SAME cell (0x1366b88 -> sl+0x86a850) SUCCEEDS on raven (verified live in the mr round) while DPCEH's fails — same descriptor shape, both app-image descriptors, both missing the sl-only first cache identically. The divergence is therefore NOT the cache, NOT the descriptor data (both complete), and NOT blob data — it is inside the per-object CCW conversion (0x862270 -> 0x862322/0x862460 -> 0x807b00/0x828130/0x862550), where the machinery must resolve the HANDLER CLASS's WinRT-interface (I(DPCEH)-with-Invoke) membership and build the CCW record from the descriptor's stub pointers. The difference between the two calls is the handler object's class and its interface-map reachability (the app's data-only tables 0x7aab20/0x7aaf80/0x7c8cc8 and/or the EEType interface maps). For a fresh delegate object the previous round's "exact failing equality inside the 0x82d8d0/0x82d6e0 walks" needs this correction: on a fresh object those walks run over an empty record list and the throw comes from the creation/lookup chain, not from a failed memo equality.

  (6) Verdict and next increment: the wall is no longer "raven-unfixable in-image-gap" — the data is in the image and the marshal machinery receives it as an argument; the missing piece is why the shared CCW conversion diverges for DPCEH on raven. The precise probe is now nameable: RAVEN_STEP anchored at sl+0x86a850, dumping per call (descriptor handle, 0x863d20 gate result, IsInstanceOfClass(obj,0x1c8f8) result, 0x807b00 hit/miss, 0x828130/0x862550 outcome) for one successful TH<App,Susp> marshal and one failing DPCEH marshal, and diffing — the first divergent branch names the raven surface to fix (or, if the machinery genuinely needs a Windows-only runtime state, the env-gated fallback that builds the CCW record from the descriptor's 12 stubs is the documented last resort). No raven-side change was made this round; nothing committed; no launch performed. Logs: none (offline). Tooling: /tmp/marshal-decode/ (regenerated sl_text.asm/sl_text.bin with the correct 0x669000 base; common.py -sl; the blob/record/EEType dump scripts in this entry are one-off heredocs, re-derivable from the RVAs above).

- The 2026-11-16 round retires the DPCEH-marshal locus and clears three walls (2026-11-16). (1) The documented RAVEN_STEP probe at sl+0x86a850 was run live and answered NEGATIVELY: no pass through the marshal entry ever carries the DPCEH descriptor (0 hits across 200k stepped instructions covering the whole throw; the app+0x5d930 descriptor appears in zero register dumps), and the marshal machinery itself WORKS — one pass per run (a sibling app-image delegate descriptor, name-hash 0x9827bc79) runs the full documented chain 86a850 → 862270 → 862550 and a CCW is created successfully; the originate wrapper's own NRE marshal also succeeds. The 80004003 wall was therefore mislabeled: the NRE is thrown by SharedLibrary's own managed code BEFORE any delegate marshal. Entry (3) of the previous round ("asymmetry this pins") is superseded by this evidence. (2) The follow-up probe pinned the real source: the app calls WinRT `CodecQuery.FindAllAsync(CodecKind, CodecCategory, String subCategory)` — the THREE-parameter overload whose retval rides the FIFTH (stack) ABI slot — and raven's codec face read `results` from r9 (the null subCategory, legitimate) and answered E_POINTER; the MCG maps E_POINTER to NullReferenceException, TPL captures it as ExceptionDispatchInfo (sl class 0x156c8, named via the recovered .NET Native ComputeNameHashCode — also named: 0x4e4f0 ResourceManager, 0x45060 NullReferenceException; the previous round's "settings-serialization region" attribution was wrong: sl+6c3a70 is List.RemoveAt, the ResourceManager machinery only builds the message string), and the EDI.Throw rethrow through the dispatched-handler chain fail-fasts. The per-run singleton NRE object and the twice-firing wall (first swallow, second fail-fast) are explained by this capture/rethrow. FIX (codec_query.c, all verified by ten launches): the five-parameter ABI `(This, kind, category, subCategory, results)` reading the retval from the stack slot; plus four live-forced follow-ons in the same chain — codec_operation owner init (QI/AddRef AV'd at NULL+0x10), the parameterized IAsyncOperation'1<IVectorView'1<CodecInfo>> QI (c1c59efc-71df-5534-b03f-e18a8fbe78bb), the operation split into parameterized (9-slot) + IAsyncInfo (11-slot) faces over one object (the flat 14-slot vtbl put GetResults on get_ErrorCode), and the IVectorView'1<CodecInfo> face (1fd24a93-c3da-513d-b216-0c4c7f412b88, extracted from the app's constant at Shapr3D.dll RVA 0x1512558) with empty-set semantics (size 0, GetAt E_BOUNDS, GetMany 0). RESULT: the whole FindAllAsync await chain completes and the app runs ~102 s, past the documented frontier (SM5_1 shader loads from C:\Program Files\Shapr3D\Renderer\SM5_1\, 990 vkCreateShaderModule, TitleBarControl.xbf scan, Studio_Dark_F IBL enumeration, Frame.Navigate → OnNavigatedTo). Fixtures green (xaml family + png_codec). (3) Two launch-environment fixes landed alongside: GetStdHandle now answers NULL (Windows behavior) for the UINT64_MAX "absent" process-parameters slot instead of the sentinel (the vkd3d logger had been issuing 5420 byte-at-a-time WriteFile calls to the bogus handle, error 6 — flood gone, startup ~5x faster, the same frontier reached in ~50 s; test extended in file_test.cpp); and a Windows.System.Diagnostics.SystemDiagnosticInfo WinRT stub (new windows_system_diagnostics.h + system_diagnostics.c in raven-xaml, catalog entry in registry.cpp, statics-first shape: ISystemDiagnosticInfoStatics d404ac21-fc7d-45f0-9a3f-39203aed9f7e = GetForCurrentSystem, Statics2, instance CpuUsage/MemoryUsage → GetReport with real host values from GetSystemTimes/GlobalMemoryStatusEx, every member traced) closes the 80040154 abort that ended some runs at ~50 s (REGDB_E_CLASSNOTREG → unhandled e06d7363 → thread stop → exit 3; note: exit 3 is ALSO the guest-abort path, not only the harness deadline — distinguish by the RoOriginateLanguageException/RaiseException tail). The diagnostics stub's class was never reached by the app (the codec wall fires earlier), but the fixture proves the full activation path in-guest both widths. (4) THE load_into WALL, diagnosed and fixed same-day: the failing call targets /Files/AppCommon/ConfirmDialog.xbf, whose root type is the dot-less app type ContentDialogBase, while the target's CCW runtime class is the documented Windows.UI.Xaml.IDependencyObject placeholder — raven's use_target root-type identity check refused a match that can never succeed for app-type roots (every earlier successful load_into binds through fully-qualified system roots). FIX (xaml_loader.c use_target): when the expected root is dot-less and the target's runtime class is the IDependencyObject placeholder, bind the target — the Page→IPage / UserControl→IUserControl identity-binding precedent generalized (Windows binds the document to the instance the caller created). EN ROUTE, a deterministic pre-wall c0000005 (3/3 strings runs, exit 0xFFFF7001): the app casts StorageFile to IRandomAccessStreamReference (winmd-guaranteed) and raven's file_query refused it, the app dereferenced the untouched NULL out — implemented the deferred storage-streams read stage in storage.c: ras_operation (IAsyncOperation'1<IRandomAccessStreamWithContentType> via the existing STORAGE_ASYNC macros), file_ras (WithContentType/IRandomAccessStream/IContentTypeProvider three faces over the file_input handle: Seek/Size/Position/SetFilePointer, ReadAsync reuse, GetInputStreamAt via DuplicateHandle, honest CanRead/CanWrite), and an IRandomAccessStreamReference sub-face on file_object (OpenReadAsync/OpenSequentialReadAsync). VERIFIED: 0 load_into failures, ConfirmDialog.xbf binds and applies, 888 GetFileSizeEx + 909 stream ReadFiles through the new faces, xaml fixtures green, both widths deployed. NEW FRONTIER: the final RoOriginateLanguageException(80070057) now has NO raven-xaml API call before it — a managed-thrown ArgumentException("Value does not fall within the expected range.") from app/framework code on thread 1 between dispatcher polls, ~12.5 s (pre-wall AV moved 8649 ms → 12516 ms); decode in progress. Launch logs: -loadcomp9.log (the decoded failing chain, via the deferred trace-switch watcher — full-strings runs die at the app's load watchdog before the ~86 s wall), -loadcomp10.log (verification). Logs for the earlier stages: -step0/-step1-fail/-step2-firstround (the probe), -null-confirm1, -diag* (SystemDiagnosticInfo round), -codecfix..-codecfix8 (the codec chain). Nothing committed.

- The node-decode and shapes rounds clear the last XBF activation walls (2026-11-16, later). (5) XBF node 0x409 was never a missing opcode: kind 19's Grid_RowDefinitions handling existed in xaml_loader.c but checked `length == 15` for a 14-character name ("RowDefinitions") at two sites, so every Grid row-definitions push fell into the unmatched-else E_NOTIMPL. Two-line fix; ConfirmDialog.xbf (the app's dialog-base document) then loads its Grid, all three RowDefinition children and its styled StackPanel/TextBlocks. The preceding `74b861a1` refusal was decoded as IApplication (Windows.UI.Xaml.IApplication) — a chronic, tolerated ask, no role in the wall. (6) The next node in the same document, 0x4a4 `PushScopeCreateTypeInit Rectangle` (stable 470), answered CLASS_E_CLASSNOTAVAILABLE: Rectangle now rides the exact Border/StackPanel/Image machinery — registry.cpp catalog line, xaml_metadata.c stable entry {470, Rectangle, Windows.UI.Xaml.Shapes.Rectangle}, xaml_elements.c rectangle factory + IGrid face (Grid.Row slots) + Fill/Stroke/StrokeThickness/RadiusX/RadiusY storage through grid_apply_setter/grid_store_stroke (brush / boxed Color / colour-name; stored, not rendered — same honesty as Border), plus a winrt_import_client extension activating Rectangle and driving Fill through the real catalog→factory chain both widths. VERIFIED (launch-rect-{1..5}): zero XBF failures in 5 launches beyond the two documented tolerated ProgressRing survivors (0x15df/0x163c); no fail-fast; runs end by the app's startup watchdog (exit 3) instead of dying at ~38k trace lines; the app reaches SM5_1 EdgeUberShader compilation, materials.mtl/gizmo asset streaming, 5 windows, ~1183 activations. The ConfirmDialog path did not re-fire in these runs (app-flow race variance — the n409 rounds themselves alternated between the 0x409 and 0x4a4 documents). (7) QUALITATIVE STATE: the run-enders are gone; the frontier moves from XBF activation walls to the renderer/main-window stage (swapchain/Present still never reached — the vkd3d presenter stops between format query and caps query, its error text on broken stderr) plus a one-off host-side teardown abort ("free(): invalid size" in raven itself after the guest stop, seen once in launch-rect-1). Tests: 24/24 (xaml/style/composition/render/dispatcher/winrt families, both widths). Logs: -loadcomp9/-loadcomp10 (load_into), -arg1..-arg3 (ArgumentException false lead + honest verification), -n409-1..5 (RowDefinitions), -rect-1..5 (Rectangle). Nothing committed.

- The steady-state and teardown rounds close the day (2026-11-16, evening). (8) STEADY STATE: the launch is now a ~22-second sprint (logs launch-steady-{1,2,3}; the 10-minute-watchdog premise is stale — the app self-terminates). Uniform sequence: XAML CoreWindow + XBF/WinRT activation bulk → 5 windows → render thread runs TWO full vkd3d rounds (vkCreateInstance → vkCreateDevice → semaphores → surface support → format query) each ending in the dummy-composition-swapchain abort, now PINNED: DXVK's own file log (via RAVEN_GUEST_DXVK_LOG_PATH; captured in /tmp/raven-shapr-probe/dxvklog2/Shapr3D_dxgi.log) shows the init dies at the Win32 display-config metadata query — `err: getMonitorDevicePath: GetDisplayConfigBufferSizes failed. ret: 50 (ERROR_NOT_SUPPORTED)` → `getMonitorEdid: Failed to get monitor device path` → `DXGI: Failed to parse display metadata + colorimetry info, using blank` — destroying the Vulkan surface right after the format query; capabilities/present modes are never queried, vkCreateSwapchainKHR is never reached. vkd3d is EXONERATED: its full stderr (recovered via raven's existing `--stdio=inherit` flag, native/main.cpp:207/726, which wires real guest std handles — use it for all future stderr captures) contains only `NtDelayExecution not found in ntdll` fixmes (x2, one per round) and VR/openxr warnings; note the one-byte WriteFile(0) flood PERSISTS (6-9.5k per run) because vkd3d keeps writing per char to the NULL handle — stderr content is unreadable through the trace, use --stdio=inherit. After the second swapchain round, `Windows.Graphics.Imaging.BitmapTransform` activation fails 80040154 (windows.graphics.imaging.dll et al. absent — LoadLibrary "library file not found"); the first failure is caught, the thread probes ~197 activation rounds, the second is unhandled → e06d7363 → thread stop → orderly teardown → exit 3. No idle phase, no welcome page. THE TWO PROCESS-KILLERS, in order: (a) implement/stub the CCD display-metadata APIs (GetDisplayConfigBufferSizes/GetDisplayConfigStringFromDevicePath/EDID) so DXVK's swapchain init passes and finally reaches vkCreateSwapchainKHR; (b) the BitmapTransform/imaging activation gap. A pipewire screenshot path WORKS (weston 15 headless needs `--renderer=pixman` for the pipewire backend — this supersedes "weston lacks screencopy"; capture via `gst-launch-1.0 pipewiresrc target-object=<pw-dump node> ! videoconvert ! pngenc`), but the frame is essentially black (no legible UI yet). (9) THE HOST TEARDOWN ABORT IS FIXED: `free(): invalid size` (~1 in 3-5 runs, always after a guest thread's fatal stop during teardown) was raven's own `NativeProcess::callback` (native/cpu/native_cpu.cpp:1791 catch block) popping the callbacks vector after `NativeThreads::check_pending`'s end-of-process delivery had already cleared it via end_call/abandon_call — pop_back underflowed an empty vector (size -1 in the core) and destroyed a NativeContext slot 240 bytes before the buffer; the guest teardown-time RaiseException on a raven-shell/shapr.native callback is the trigger, the race is the intermittency, and the return gate at native_cpu.cpp:2091 already had the guard the catch lacked. FIX: capture depth before push_back, pop/restore in the catch only `if (state->callbacks.size() > depth)`. Verified: 24 targeted tests pass, 3 fresh teardown launches clean (gdb-wrapped runs never reproduce — post-mortem the systemd-coredump core instead). Uncommitted. Logs: -steady-1..3, -core-1..5; old traces archived to /home/daalbu/raven-shapr-archive/logs/ (tmpfs was full).

- The CCD and imaging rounds (2026-11-17). (10) THE CCD DISPLAY-METADATA STAGE, implemented end to end against DXVK v3.1.1's actual calls (fetched source + dxgi.dll import table): GetDisplayConfigBufferSizes (Windows-style flag validation, 87 on bad structs) + QueryDisplayConfig (one active path: adapter LUID 1, 60000/1000 Hz, CEA 148.5 MHz target mode, 1920x1080@32bpp source mode from the desktop metrics) + DisplayConfigGetDeviceInfo (SOURCE_NAME \\\\.\\DISPLAY1, TARGET_NAME "Raven Monitor" / \\?\DISPLAY#RAVEN0000#… device path, ADAPTER_NAME, ADVANCED_COLOR_INFO, SDR_WHITE_LEVEL 1000 = 80 nits; header preserved on write-back, size mismatch 87, unknown type 50) in native/desktop/window_manager.cpp; a synthetic valid EDID 1.4 base block ("RVN" PnP id, sRGB chromaticity per libdisplay-info's encoding, CEA 1920x1080@60 preferred detailed timing, correct checksum) seeded as HKLM\SYSTEM\CurrentControlSet\Enum\DISPLAY\RAVEN0000\…\Device Parameters\EDID by register_monitor_edid in native/nt/registry.cpp; and — the key discovery — DXVK's setupapi imports are served by the GUEST PE component raven-setupapi (host traps never fire), so SetupDiEnumDeviceInterfaces / SetupDiGetDeviceInterfaceDetailW (packed-detail semantics) / SetupDiOpenDevRegKey were implemented in native/guest/raven-setupapi/devices.c with one monitor interface for GUID_DEVINTERFACE_MONITOR, their FAIL stubs removed. VERIFIED: the whole chain answers ERROR_SUCCESS in-trace (launch-ccd-3), every DXVK CCD error line gone, `native_display_enumeration` extended (checks 8–14). (11) THE IMAGING CHAIN: Windows.Graphics.Imaging.BitmapTransform (IBitmapTransform ae755344-…, the ONLY interface per pinned windows-rs 58217a69 — vtable: ScaledWidth/ScaledHeight/InterpolationMode/Flip/Rotation/Bounds get/put pairs; NO GetTransformedSize exists) implemented in imaging.c + windows_graphics_imaging.h mirroring system_diagnostics.c, then BitmapDecoder statics (IBitmapDecoderStatics 438ccb26-… vtable: 7 decoder-id getters with REAL WIC CLSIDs from mingw wincodec.h, GetDecoderInformationEnumerator — a SYNC IVectorView<BitmapCodecInformation> with parameterized IID 97dfde96-… computed as uuid5 over the pinterface namespace, honest empty; CreateAsync/CreateWithIdAsync E_NOTIMPL — no decode fabrication) + Statics2 (Heif/Webp ids); catalog + dispatch + fixtures bitmap_transform_client/bitmap_decoder_client (both widths), 16/16 tests. VERIFIED by launch: both classes served (`winrt: activation … served`); the app's ACTUAL usage: BitmapTransform = ActivateInstance + put_Flip; BitmapDecoder = CreateAsync(stream) only. The render thread's killer moved: CreateAsync's honest E_NOTIMPL → 80004001 double-throw → exit 3; the WIC-backed decode round (windowscodecs.dll is present in the guest) is the follow-up. (12) STATE AFTER THESE ROUNDS: the render thread survives past the metadata stage; the composition swapchain proceeds through surface + format negotiation but vkd3d-proton's IDXGIVkSwapChain::CreateSwapChain HALTS SILENTLY between formats and capabilities on both adapter attempts (vkGetPhysicalDeviceSurfaceCapabilitiesKHR never called, no error anywhere) — decode in progress (prime suspect: raven's synthetic vulkan-1.dll proc table vs vkd3d's post-format queries; earlier rounds traced DXVK probing "unavailable instance/device function vkGetPhysicalDeviceSurfacePresentModes2EXT" etc.). Also noted: `Windows.Devices.Display.DisplayMonitor` statics (IDisplayMonitorStatics 6eae698f-…) is a caught-but-real gap in Shapr.Native.dll's path. Logs: launch-ccd-1..3, launch-baseline-trace, launch-imaging-1..2, launch-bmpdec-1..2. Nothing committed.

- The refusal-trace round (2026-11-17). (13) THE 0xFFFF7001 TERMINATE IS NOT THE DECODE — two real walls named by the deployed refusal traces (`raven-xaml: <face>.refuse <iid>` in imaging.c, `winrt: activation … query failed` in raven-shell). IMPORTANT: the guest trace strings only print under RAVEN_TRACE_DWRITE=1 (font_service.cpp RavenTrace gate) — the first probe launch without it was blind. Run-to-run variance is heavy (threads race): the app may die before OR after BitmapDecoder.CreateAsync, all four probe logs reach the same 92-shader stage. (a) WALL 1 — the Button factory: the app's `new Button()` (the .NET Native projection constructor) asks the Button activation factory for the REAL `Windows.UI.Xaml.Controls.IButtonFactory` (80a13c19-843a-451c-8cf5-44c701b0e216; GUID resolved through the SDK metadata's CustomAttribute blob on the IButtonFactory TypeDef in Windows.Foundation.UniversalApiContract.winmd — downloaded from nuget microsoft.windows.sdk.contracts 10.0.19041.2 because the guest's own Windows.winmd is trimmed), the projection maps the refusal to a managed exception (first 80004003 NRE swallowed, then a 80004002 E_NOINTERFACE rethrow that RaiseFailFastExceptions through sharedlibrary.dll+7e38de from a raven-xaml dispatcher callback, stack shapr3d+1becf39 → raven-xaml+2eaf/+12e60 dispatcher pump → …), and shapr.native.dll TerminateProcess(0xFFFF7001)s — the exact exit every earlier run showed. Windows serves this interface, so per the honesty rule it is served: FIX (xaml_elements.c) — hand-declared IButtonFactory vtbl over the existing element factory machinery; element_factory_query answers the new IID for element_button, the face's CreateInstance mirrors control_create's contract (outer&& !inner → E_POINTER; creates the button + engine default style; a non-null outer answers E_NOINTERFACE honestly — the button object cannot aggregate yet) and IButtonFactory.CreateInstance has the exact 2-param ABI of IControlFactory.CreateInstance (verified in the metadata, sig 200212c0…). VERIFIED live: launch-refuse-4 activation `Windows.UI.Xaml.Controls.Button served` for iid 80a13c19. (b) WALL 2 — the decoder is not a frame: after a SUCCEEDED CreateAsync the app casts the decoder result to `Windows.Graphics.Imaging.IBitmapFrame` (72a49a1c-8081-438d-91bc-94ecfc8185c6, named by `raven-xaml: BitmapDecoder.refuse 72a49a1c-8081-438d` in launch-refuse-3 immediately before the same TerminateProcess) and raven's decoder face refused it. Windows' own contract (learn.microsoft.com BitmapDecoder): "The BitmapDecoder class implements IBitmapFrame … data from the first frame" — so the decoder must serve IBitmapFrame AND IBitmapFrameWithSoftwareBitmap (fe287c9a). FIX (imaging.c) — create_decoder_object now holds `frame0` (a real create_frame_object over IWICBitmapDecoder_GetFrame(wic, 0)), decoder_query serves both frame IIDs by delegating to frame_query (real frame data behind every member), decoder_release releases it. Fixtures green (bitmap_transform + bitmap_decoder + xaml_controls, both widths), raven-xaml.dll deployed sys32+syswow64 cmp-verified. (c) REMAINING: the swallowed 80004003 NRE from sharedlibrary+8feadc on thread 1 still fires every run (message literal 53 chars = "Object reference not set…" region; source unidentified — NOT CodecQuery.FindAllAsync, which the previous round fixed) and its 80004002 rethrow fail-fasts non-deterministically BEFORE the render thread can reach the decode (launch-refuse-2/4 died there; launch-refuse-1/3/decode-2 got past to the TerminateProcess). What the app calls on the decoder after the IBitmapFrame cast now succeeds (GetPixelDataAsync? OrientedPixelWidth? GetSoftwareBitmapAsync?) is the open question for the next launch that survives the thread-1 race. Logs: launch-refuse-1 (blind, no RAVEN_TRACE_DWRITE), -2 (E_NOINTERFACE fail-fast wall), -3 (IBitmapFrame refusal pinned), -4 (Button fix verified live; thread-1 fail-fast by race). Nothing committed.

- The refuse round names two walls and fixes both (2026-11-17). (13) METHODOLOGY: guest trace strings (all "winrt:"/"raven-xaml:" lines) print ONLY under RAVEN_TRACE_DWRITE=1 (the RavenTrace host gate at native/nt/font_service.cpp:520) — a launch without it is blind; put it in every diagnostic launch. Run variance is heavy (threads race): some runs die before BitmapDecoder.CreateAsync, some after. (14) WALL 1, the Button factory: the app's `new Button()` (.NET Native projection ctor) asks the Button activation factory for the REAL IButtonFactory (80a13c19-843a-451c-8cf5-44c701b0e216, resolved authoritatively from the CustomAttribute blob on the IButtonFactory TypeDef in the nuget microsoft.windows.sdk.contracts winmd — the guest's own Windows.winmd is trimmed); raven refused → NRE-swallow → 80004002 rethrow → fail-fast → TerminateProcess(0xFFFF7001) (the exit several earlier rounds saw). FIX (xaml_elements.c): hand-declared IButtonFactory vtbl, element_factory_query answers it for element_button, button_create_part/button_create_instance mirror control_create's contract (non-null outer → E_NOINTERFACE honestly). LIVE-VERIFIED: `Windows.UI.Xaml.Controls.Button served` for that IID in launch-refuse-4, app progresses (PropertyValue/Uri/DataWriter activations). (15) WALL 2, the decoder is not a frame: the app's await continuation GetResults→cast expects BitmapDecoder to implement IBitmapFrame (72a49a1c-…) — Microsoft's contract says the decoder exposes first-frame data — and raven refused; refusal trace `raven-xaml: BitmapDecoder.refuse 72a49a1c-…` fired 2 lines before TerminateProcess. FIX (imaging.c): decoder_object holds frame0 (a real create_frame_object over IWICBitmapDecoder_GetFrame(wic,0)), decoder_query serves IBitmapFrame/IBitmapFrameWithSoftwareBitmap by delegating to frame_query — real frame data behind every member. FIXTURE-VERIFIED (10/10 imaging+controls); app-exercise pending a run that survives the thread-1 fail-fast race. (16) NEXT: the thread-1 swallowed-80004003 → 80004002 fail-fast race (same dispatcher-pump stack shape as the mislabeled ancestors, but a DIFFERENT instance — neither CodecQuery nor DPCEH) preempts the decode path non-deterministically; decode in progress. Logs: launch-refuse-1..4. Nothing committed.

- The thread-1 wall decode (2026-11-17). (17) PINNED, two managed exceptions in one app callback: after Button serves (IButtonFactory round), thread 1's generated ctor/panel code sets Background twice (named-color table: Black/Gray/…/Orange, 12 entries), paint round, then sharedlibrary handles L"Windows.Foundation.IAsyncAction" (obtained WITHOUT any traced WindowsCreateString — a leak: some raven surface hands out raw pooled-static pointers as string data; unpinned) → CoGetApartmentType burst ending (160, 87) → native c0000005 reading 0 at shapr3d.dll+2b1cbb0 → managed NRE 80004003 (swallowed) → castclass `Button`→`EETypeRva:0x0001AFA0` fails → 80004002 ("Unable to cast object of type 'EETypeRva:0x00066938(Windows.UI.Xaml.Controls.Button)' to type 'EETypeRva:0x0001AFA0'.") unhandled → fail-fast, exit 2147500034. PROVEN the cast never touches raven's button COM faces (RavenTrace-relayed refusal print in button_query_any never fired — note: OutputDebugStringA relay is mute by design unless RAVEN_TRACE_DEBUG). The cast is decided from managed-side metadata via the SharedLibrary cast walk consuming IInspectable::GetIids — which every raven-xaml object answers dishonestly (count=0, iids=NULL, inspectable_iids xaml_elements.c:98). (18) FE-FAMILY FIX (real contract work, not the key): the Button instance now serves the FE2/FE3/FE4/FE6 continuation family (extern'd winmd IIDs, hand-declared vtbls, RequestedTheme/AllowFocusOnInteraction defaults, focus-visual storage, DataContextChanged/Loading/ActualThemeChanged events) + a `raven-element: refused QI <iid>` diagnostic under RAVEN_TRACE_ELEMENT_QI; 8/8 xaml fixtures; deployed. Wall unchanged (identical shape). (19) NEXT (offline-derivable): name EETypeRva 0x1afa0 from Shapr3D.dll's EEType table (known neighbor 0x1af60 = IFrameworkElement3; Button's own EEType = 0x66938) and make GetIids honest for the button/element family — watch the pinned GetIids out-pointer convention trap (docs ~2803). Secondary: audit raven's IAsyncAction faces for a wrong-slot/success-null shape (the null-read follows the IAsyncAction handling). The render thread never reaches the BitmapDecoder decode in any launch (thread 1 dies first); the IBitmapFrame decoder fix remains fixture-verified, app-unexercised. Logs: launch-t1-1..3. Nothing committed.

- The EEType round closes the castclass wall (2026-11-17). (20) EETypeRva 0x1afa0 = Windows.UI.Xaml.IFrameworkElement — the BASE interface, IID a391d09b-4a99-4b7c-9d8d-6fa5d01f6fbf = raven's own IID_frame_framework (xaml_controls.c:756). Fully offline decode: EETypeRva is the direct image RVA (pinned by the FE3 neighbor); .NET Native EEType layout +0x00 flags/+0x04 baseSize/+0x08 related-type/+0x10 numVtableSlots u16/+0x12 numInterfaces u16/+0x14 name hash/interface map at +0x18+8*numVtableSlots; name hashes via corert's ComputeNameHashCode (seed 0x6DA3B944, rotl-5 pairs, dotnet/runtime TypeHashingAlgorithms.cs), verified against System.Object 0xb738b9c1 and DPCEH 0xfdb30aca; the IID extracted from the app's own .data interface record {EEType ptr, rva, 0, GUID} — the method validated by recovering FE2/3/4's GUIDs the same way and matching raven's extern'd constants. Button EEType 0x66938 (hash 0xc61511e1, 25 vtable slots, 43 interfaces): map = [0..2] dispatch cells, [3..4] the stripped runtime-created generics (0x1b1c0/0x1b1a0), [5..13] IUIElement family (no 6), [20] IFrameworkElement, [21..24] FE2/3/4/6, [25] 0x1af00 unnamed, [29..33] IControl family, [41] Button's 2nd own interface 0x66bb8, [42] IButton 0x66b98; NO IButtonBase-family interface is projected anywhere in the app image. (21) FIXES: (a) honest GetIids on the shared element path — element_family_iids[] + element_honest_iids() probing the object's own QI; rewired ui_iids/GRID_PART/textblock_iids/BUTTON_PART/BUTTON_FE_BOILERPLATE (previously ALL answered count=0/iids=NULL); (b) the real gap: the button served FE2/3/4/6 but refused the base IFrameworkElement — button_framework_part (47-slot vtbl in winmd slot order, storage-backed Resources/Tag/DataContext/Style/Name/extents NaN-auto/0/+inf/Stretch/Margin/alignments/FlowDirection + Loaded/Unloaded/SizeChanged/LayoutUpdated) now served for IID_frame_framework; (c) B2 audit: xaml.c/dispatcher_queue.c action vtables correct, but data_writer.c flush_table had only 8 slots — the 9th IAsyncAction slot (GetResults) was NULL, matching the launch's null-read; fixed with flush_results returning the recorded error. VERIFIED: castclass message zero occurrences in both launches (was 1/1); thread 1 runs ~30% further (paint rounds, sustained sqlite streams, file enumeration); 46/46 xaml-family tests. NEW WALL: managed KeyNotFoundException (80131577, message "key") on thread 1 in a dispatched callback (unwind raven-xaml+2eaf → shapr3d+19fcac5), unhandled → exit 2148734327, both launches identical; the swallowed NRE remains co-located before it. Logs: launch-eetype-1..2. Nothing committed.
- The key-not-found round decodes both thread-1 exceptions and closes the Frame.Navigate parameter gap (2026-11-17). (17) FORENSICS (launches launch-key-1/2/3, RAVEN_TRACE_STRINGS=1 is what decodes the guest string arguments — put it in every string-shaped diagnosis): (a) the swallowed 80004003 NRE is the page lifecycle — raven-frame traces `page OnNavigatedTo 0x80004003` (xaml_controls.c frame_navigated_deliver) in the traced run: the app's OnNavigatedTo override dereferences a raven-served null. (b) the 80131577 "key" KeyNotFoundException is NOT a raven face misbehaving on its own: the message HSTRING is the fixed BCL literal "key" (Shapr3D.dll RVA 0x13cae74 = base 0x180010000 + 0x13cbe74 — see the module-table caveat below), marshaled by sharedlibrary.dll+848214 immediately before the originate with NO Win32 api call in between — the MCG's IDictionary-adapter mapping of an E_BOUNDS from an IMap`2 Lookup on a raven-served collection. Right before it the app reads back its remote-configuration JSON (settings container 9c9fec88508fc60c, key `com.shapr3d.shapr.userDefaults.remoteConfiguration.configurationValues`, type-12 record verified byte-faithful on disk — raven's settings round-trip is NOT the corruption) and re-inserts it; the throw lands in the follow-up dispatched callback. (c) MODULE-TABLE CAVEAT, applies to every Shapr3D frame offset logged this week: raven's module table records shapr3d.dll at 0x180000000 but the loader actually maps it at 0x180010000 (64 KB up; the literal-pool string VA decode proves it) — every logged shapr3d.dll+offset is 0x10000 TOO HIGH; subtract 0x10000 before mapping a logged offset to a file RVA. raven-xaml.dll's own trace offsets carry a similar (0xfff-shaped) skew (its WindowsCreateString callsite for settings_decode sits at +0x5a1fe but the trace says +0x5b203). (18) FIX (xaml_controls.c): frame_navigate dropped the `parameter` argument in all three overloads (IFrame.Navigate(Type,Object), Navigate2, NavigateToType) and navigation_args_parameter answered constant NULL — Windows delivers the Navigate parameter through NavigationEventArgs.Parameter, and an app that dereferences it inside OnNavigatedTo NREs exactly as traced. The frame now holds `navigation_parameter` (AddRef'd, replaced per Navigate, released with the frame), navigation_args_create copies it into the args and get_Parameter returns it (null stays null with S_OK for the 1-arg overload). Fixtures green (xaml family + ieframe, both widths), raven-xaml.dll deployed sys32+syswow64 cmp-verified. (19) VERIFICATION launch-key-3: no 80004003, no 80131577, no fail-fast; the run reached the settings/language stage (requestedTheme, Localizable.strings marshals present, "key" marshal ABSENT) and ended at the 120 s deadline (exit 3, thread stops, XAML unwind) — heavy run variance again: navigation never completed in this run, so the parameter fix is fixture-verified, not live-verified against the KeyNotFound. (20) NEXT: name the exact IMap face the adapter called when the "key" throw recurs — a temporary E_BOUNDS trace note (key text) in xaml_dictionary.c map_lookup/view_lookup, settings_view_lookup and the other keyed faces turns the next reproduce into an answer; and decode the launch-key-3 UI-thread shutdown (microsoft.ui.xaml.dll EventUnregister → tbb FreeLibrary → exit 3) that ended the run before navigation. Nothing committed.
- The module-table caveat is withdrawn and the navigation parameter is live-verified; the KeyNotFound's keyed face is named (2026-11-17). (21) CAVEAT (17c) WITHDRAWN — raven's trace offsets were never wrong, do NOT subtract 0x10000 from logged shapr3d.dll offsets. Forensics on the shipped image: (a) shapr3d.dll's PE preferred ImageBase is 0x180000000 and raven maps it at 0x180010000 (64 KB up, overlap resolution), and both the PEB loader table and every trace print use loaded.base() = 0x180010000 = the real mapping — a logged offset IS the image RVA (raw 0x182b2cbb0 → +2b1cbb0 sits inside a .pdata function range; all API-caller offsets 1a7ce77/21796d9/19fcac5/2b1fd9a/2178a6d/1a2687c likewise). (b) The literal-pool proof the caveat cited, redone exactly: the BCL "key" string object (EEType 0x…800c1978, length 3, "key") sits at Shapr3D.dll file RVA 0x13bbe74; the traced marshal pointer was 0x1813cbe74 = 0x180010000 + 0x13bbe74 EXACTLY. The caveat's RVA 0x13cae74 was a 0x1000 RVA↔file-offset slip (that RVA holds "Foun…" of a longer literal), mislabeled into a 0x10000 skew; its own equation was internally inconsistent. (c) The raven-xaml "+0x5b203 logged vs +0x5a1fe real" pair is binary drift, not base skew: the traced run used the pre-parameter-fix deployment, the offline decode used the post-fix rebuild; in the current image 0x5a1fe is a call return address (`ff 15 aa 6f 0d 00` ending at +0x5a1fe) and 0x5b203 is not a call site. Fresh launches launch-nav-1/2 re-log the same offsets as launch-key (e.g. shapr3d.dll+2b1fd9a, still a .pdata member) — no offset shift across the rebuild. (22) NAVIGATION-PARAMETER FIX LIVE-VERIFIED (launch-nav-2, RAVEN_TRACE_DEBUG=1 so the OutputDebugStringA relay prints): `raven-frame: page OnNavigatedTo 0x00000000` — the app's override ran to completion with the frame-held parameter (launch-nav-1 and launch-nav-2 contain zero 80004003 lines; the swallowed NRE is gone). (23) The E_BOUNDS trace note (key text) went into the keyed faces — xaml_dictionary.c map_read/map_lookup and view_lookup (string keys print their text, non-string keys print the pointer) and storage_settings.inc settings_lookup (prints status + key; the 8000000b misses it prints on a fresh profile are legitimate first-run absences: inputDevice.usedPointerDevices, device.id, settings.requestedTheme, appstate.numberOfAppStarts, appversion.firstOpenedVersion, com.shapr3d.shapr.userDefaults.localizationAdminOption, tools.measureMode, sketchPens.*, viewNavigation.currentPreset, …). Fixtures: native_xaml_{application,window,controls,input}+resource_dictionary+storage, x64+x32, green; deployed sys32+syswow64 cmp-verified. (24) THE KEYED FACE IS NOW NAMED (launch-nav-2 line ~763520): the fatal 80131577 "key" is preceded by exactly one resource miss — `raven-xaml: map Lookup E_BOUNDS key=<AccentButtonStyle>` from raven-xaml.dll+449a6 (the ResourceDictionary IMap.Lookup → map_read path; 0x449a6 is the return address of its import-style call, verified in the image), followed by GetRestrictedErrorInfo → FormatMessageW(8000000b) → the IAsyncAction/CoGetApartmentType (160, 87) dispatch burst → the dispatched-callback RoOriginateLanguageException(80131577 "key") → fail-fast, exit 2148734327 — the same stack shape both launches (shapr3d.dll+2075fae … raven-xaml.dll+2eaf/12e60 → shapr3d.dll+19fcac5). On Windows this lookup succeeds, so raven's ResourceDictionary must resolve framework styles ("AccentButtonStyle" lives in WinUI's theme/generic resources, nowhere in raven's served resources today): the NEXT fix is either (a) fall IMap.Lookup through to raven's framework theme dictionary, or (b) define AccentButtonStyle in raven's served theme resources — NOT an E_BOUNDS-contract change; the adapter's KeyNotFound is then correct behavior for a dictionary that genuinely lacks the key. Secondary observations: launch-nav-2 progress: settings/language stage crossed, the page navigated (ProgressRing peer bridge, frame QIs) before the throw; both launches fail-fast well before the 180 s deadline (exit 119 wrapper / process status 2148734327). Logs: /tmp/raven-shapr-probe/launch-nav-1.log, launch-nav-2.log (fresh state preserved at st-nav-1-*/st-nav-2-*). Nothing committed.

- The navigation and skew rounds (2026-11-17, later). (22) The module-base "skew" caveat from the key round is WITHDRAWN — there is no skew: every trace print computes caller − LoadedImage::base() and that base is the exact mmap address (checked against guest registration); the decisive check puts the BCL "key" string object at file RVA 0x13bbe74 and the traced marshal pointer at 0x1813cbe74 = 0x180010000 + 0x13bbe74, exact — the key round's "0x13cae74 near the literal pool" was an RVA↔file-offset slip, and the raven-xaml "+0x5b203 vs +0x5a1fe" pair was pre/post-fix binary drift. DO NOT subtract 0x10000 from logged Shapr3D offsets. (23) The Frame.Navigate parameter fix is LIVE-VERIFIED: `raven-frame: page OnNavigatedTo 0x00000000` in launch-nav-2 (the 0x80004003 NRE: zero occurrences across both launches); the E_BOUNDS key-text notes added to xaml_dictionary.c (map_read/view_lookup) and storage_settings.inc (settings_lookup) name the first-run misses as legitimate fresh-profile absences. (24) THE KEYNOTFOUND FACE, named: `raven-xaml: map Lookup E_BOUNDS key=<AccentButtonStyle>` (ResourceDictionary IMap.Lookup, raven-xaml.dll+449a6 verified as the call return) — the app's dispatched callback resolves a WinUI framework style raven's dictionaries do not carry; on Windows it lives in WinUI's theme/generic resources. The KeyNotFoundException("key") is .NET Native's IDictionary-adapter rendering of the E_BOUNDS Lookup miss — CORRECT adapter behavior for a dictionary genuinely lacking the key; the fix is framework-theme serving in raven's ResourceDictionary (theme-dictionary fall-through or real style definitions — round in flight). Logs: launch-nav-1..2. Nothing committed.

- The framework-theme round closes the KeyNotFoundException wall live (2026-11-17, later). (25) DESIGN: a process-wide framework-theme FALL-THROUGH in xaml_dictionary.c — `map_read`'s string-key miss path (and map_has_key) consults a lazily-built framework dictionary built from the app's own shipped WinUI 2.8 package (deps/Microsoft.UI.Xaml.2.8/resources.pri → 21h1_themeresources.xbf 783 KB + 21h1_generic.xbf 194 KB; kept only when the framework package actually contributed content; builder-thread reentrancy guard against the build's own Source-load deadlock), theme-aware via dictionary_lookup_theme_depth, then raven_xaml_system_color, then raven_xaml_platform_resource — mirroring resolve_resource's tail; xaml_loader.c's resolve_resource consults it below Application.Resources. Honest bounds: serves only what the shipped framework documents; genuine misses still answer E_BOUNDS (miss notes now print size=/merged=). Probe evidence: AccentButtonStyle/DefaultButtonStyle/SystemAccentColor resolve through every merged member incl. the app-shaped chain. VERIFIED (launch-res-1, ~1.02M lines): ZERO E_BOUNDS notes, no 80131577, no fail-fast; the app reaches page navigation + ProgressRing peer bridge, settings/language, userDefaults.lastUsedD3D12BackendVersion, dxgi/vulkan swapchain setup (dummy composition swapchain active, two xdg surfaces), ContentDialog activation, framework styles realized (BasedOn chains firing), a cryptography/license stage — then an orderly .NET shutdown, exit 3. 44/44 xaml-family tests both widths. (26) NEXT WALLS (characterized): (a) thread 1: `raven-xaml: XBF failure at 0006af93, status 80070057` ×2 — inside the SetCustomRuntimeData blob at [0x691d8] of 21h1_themeresources.xbf (substream 4, the WinUI theme/runtime-data form), a deferred framework-entry realization refusing E_INVALIDARG; (b) thread 186: the app's own LoadLibraryW fails module-not-found from shapr.native.dll, 80040154 + e06d7363, retries, then its own exit 3 — a module-loading frontier (candidate: the known Windows.Devices.Display.DisplayMonitor statics gap, docs 12). Both rounds in flight. Logs: launch-res-1. Nothing committed.

- SESSION END (2026-11-17, quota wrap-up). Tree left GREEN: full ninja clean, 19/19 tests in the xaml/bitmap/imaging/display/winrt families, raven-xaml.dll deployed to both prefix widths cmp-verified. The two in-flight rounds (SetCustomRuntimeData blob at 21h1_themeresources 0x6af93; the thread-186 LoadLibrary/module frontier — its half-written Windows.Devices.Display.DisplayMonitor component: display_monitor.c, windows_devices_display.h, display_monitor_client.c) were STOPPED mid-work and QUARANTINED intact to /tmp/unfinished-displaymonitor/ (wiring reverted: xaml.c dispatch, xaml_internal.h declaration, native/CMakeLists.txt source/dep/foreach references; two 30-second fixes were applied before quarantine — missing <stddef.h> and a missing WINAPI on operation_query — so a resume starts from a compiling file with these notes). RESUME LIST, in order: (a) the SetCustomRuntimeData blob decode (docs 26a); (b) the DisplayMonitor statics component (docs 26b — candidates: IDisplayMonitorStatics 6eae698f-… from Shapr.Native.dll's path; restore the quarantined files + wiring, fix the header's duplicated struct typedef at lines 49/68 and the fixture's statics1/IInspectableVtbl references); (c) vkd3d presenter: swapchain creation is lazy at first Present — the render thread must survive to Present (docs 12, 25); (d) once Present flows, DXVK sizes the dummy composition swapchain 1×1 from GetWindowRect(NULL)=error 1400 — raven's caps-extent override is the intended save. Nothing committed; all work uncommitted in the tree as the user prefers. Full session narrative: entries (1)–(26) of this file's 2026-11-16/17 rounds.

- The DisplayMonitor round (2026-11-19). (27) CORRECTION to the session-end note: the "quarantined" DisplayMonitor work was never removed — the tree still carried it intact (display_monitor.c 1218 lines, windows_devices_display.h, catalog registry.cpp:305, dispatch xaml.c:2150, fixture, CMake wiring) and a reboot did not lose it; the resume note's two "30-second fixes" were also already applied. The round verified the whole surface against the pinned windows-rs revision (IDisplayMonitorStatics 6eae698f declares exactly GetDeviceSelector / FromIdAsync / FromInterfaceIdAsync — GetDefaultAsync does NOT exist; IDisplayMonitor 1f6b15d4 = 20 members, no refresh-rate properties; IDisplayMonitor2 023018e6 = IsDolbyVisionSupportInHdrMode; parameterized IAsyncOperation'1<DisplayMonitor> = 28c0e623 verified by reproducing the imaging component's known values; Shapr.Native.dll is a C++/WinRT consumer whose only statics use is GetDeviceSelector) and fixed TWO REAL DEFECTS: (a) monitor_query_any was declared WITHOUT WINAPI while cast to raven_weak_query_fn — on 32-bit the cdecl-vs-stdcall stack mismatch shifted slots (fixture faulted at resolved=0xffffff00) and had been permanently masked by an earlier fixture death at the EDID check; (b) stale per-test prefixes (build/native/test-prefixes) held pre-fix EDIDs that register_monitor_edid never heals (it skips existing keys) — deleted, fresh seeding verified. 21/21 tests both widths; deployed both widths (previous copies were Nov 17). LAUNCHES (launch-disp-1/2/3): the `winrt: no class Windows.Devices.Display.DisplayMonitor` line is gone in ~2.3M lines; the app never queries the class in-trace (fixture proves the surface); the PREVIOUS module frontier (thread-186 LoadLibrary → 80040154 → exit 3) is GONE — shapr.native.dll now loads a raven component and activates ApplicationData successfully. NEW STATE, first-ever healthy timeout: the app loads its CAD engine (td_gs_25.12src_17.dll, usd_arch.dll, tbb.dll), ~9 threads settle into waits, and thread 1 grinds raven-xaml string/container loops (RVAs 0x45741–0x45937 + 0x4d692; the storage_settings container scan building \LocalSettings\containers\… paths) for the ENTIRE 180 s budget — no crash, no exception, no surface stage. NEXT FRONTIER: measure and fix the grind (round in flight). Logs: launch-disp-1..3. Nothing committed.

- The thread-1 engine-init grind round: measured, dominant cost fixed, the app crosses into the graphics stage (2026-11-20). (28) MEASUREMENT (launch-perf-1, fresh profile, RAVEN_TRACE_API=1, 120 s budget, thread-1 time attributed by u-deltas between traced calls — u is microseconds): the entire thread-1 budget (119.7 s traced span) was string-op cycles at four raven-xaml sites, RVAs mapped through .pdata function ranges (objdump -t/-d labels are fine, but .pdata is the authoritative range source): key_string_hash 45671×, key_matches 15208× (string path), create_boxed_string_length 15752×, pri_lookup_blob 32757 CompareStringOrdinal. The API names on those sites (WindowsDuplicateString/WindowsDeleteString/WindowsGetStringRawBuffer) are the inlined boxed-string get_Value copies, not separate call sites. THE DRIVER: per-probe atom [1 WindowsCreateString → 3 key_string_hash → 1 key_matches] ×14882 (exact: 14882·3+680+146·2+33+10·2 = 45671 hashes; 14882+292+33+1 = 15208 matches) — that is raven_xaml_dictionary_defer's old shape: boxed probe → map_insert's store_find (hash 1) → insert re-hash at line 1021 (hash 2) → explicit store_find of the just-inserted entry (hash 3 + the visible match). The dictionaries defer ~15k distinct entries (framework/theme build); the PRI scan is the sibling of the known manifest grind, ~5800-entry linear pass per blob lookup (11 lookups, matches at entries 5773–5780, one 4221-deep miss at EOF). Total: dictionary grind ≈ 108 s (90 %), PRI ≈ 11 s (9 %); ZERO traced API calls from d3d12/dxgi/d3d11/libegl/usd_arch/td_gs/microsoft.ui.xaml — the app never left engine init. (29) FIX (xaml_dictionary.c + xaml_styles.c + xaml_internal.h, semantics identical, call counts strictly down): (a) map_insert_entry — one insert pass that computes the key hash once, scans with the same filter store_find uses, replaces or appends, and HANDS BACK the entry; the public map_insert wraps it; raven_xaml_dictionary_defer sets the deferred fields directly on the returned entry — the second find (and its full-store scan) is gone, so a defer costs 1 hash instead of 3; (b) raven_xaml_string_box_handle (xaml_styles.c) — a non-owning read of a string box's stored HSTRING, gated on the box's own reference-vtable identity so any other Reference`1<HSTRING> (property_value.c's boxed_value) answers E_NOINTERFACE; key_string_hash hashes the borrowed handle's buffer (1 API instead of 3: dup+rawbuf+del) and key_matches compares the two borrowed handles through the same WindowsCompareStringOrdinal (1 API instead of 5) with the exact old copy path as fallback; (c) entry_read takes the caller's already-found entry (store_lookup/map_read/view_lookup all held it) — the redundant re-find is gone, the post-realize re-find stays. WindowsDuplicateString/WindowsDeleteString at the old grind sites: zero occurrences after the fix. Fixtures: native_resource_dictionary/storage/mrt/mrt_manager/mrt_extended green both widths; native_style_x64/x32 RED — pre-existing (verified by rebuilding the component with the fix reverted: identical failure — the style client needs ~12 s vs raven's 5 s default entry timeout; with the fix it is ~1.5 s faster, and its remaining cost is RavenFont* + GetPixel polling, not dictionary work). Deployed both widths (md5 cmp). (30) VERIFICATION (launch-perf-2, fresh profile, same 120 s budget): the defer path now costs 25661 probes × 1 create + 1 rawbuf + 1 release, 700 visible compares (matches only fire for real finds/collisions now); thread-1 distribution: PRI scan 22.5 s (61040 compares — grew because the app progresses further), probe create ≈ 21 s, store-scan tail ≈ 20 s attributed to the release sites (the remaining O(N) scan per insert into a ~25k-entry store — the NEXT dominant cost, a hash-bucketed store is the candidate fix), hash rawbuf 9 s. THE WALL IS CROSSED: dxgi.dll and d3d12.dll load at t=34.3 s, libegl 34.4 s, usd_arch 34.7 s (calling APIs), raven-web 42.1 s, microsoft.ui.xaml 55.8 s, d3d12core 60.7 s; td_gs_25.12src_17.dll executes; settings reads appear (726 ReadFile); 72 threads; the run ends in quiet multi-thread waits (no crash, no exception) — the former grind phase dropped from the whole budget to ~34 s. NEXT, in cost order: (a) the PRI blob lookup's linear scan (61040 compares, 22.5 s — index the entries at parse time); (b) the defer path's remaining O(N) store scan (~20 s at 25k entries); (c) whatever the graphics stage surfaces next (d3d12core active but no CreateSwapChain yet). Logs: launch-perf-1.log (pre-fix), launch-perf-2.log (post-fix), style-hang.log/style-fixed-trace.log (the style-fixture forensics), all in /tmp/raven-shapr-probe/. Nothing committed.

- The index round (2026-11-20). (31) PRI PATH INDEX (mrt_manager.c): struct pri_path_index — per-state bucket heads/tails with one next-slot per schema entry (no strings owned), built lazily on the first lookup searching a state (pri_lookup_blob/map_find_entry) with the ensure_pri interlocked once-idiom; FNV-1a over the entry's full path (the exact needle entry_path builds and CompareStringOrdinal compares) with ASCII folding; ALL-OR-NOTHING (any non-ASCII unit or failed allocation sets no_index and keeps the linear scan verbatim; a bucket miss falls through to the linear loop) — entries CSO-equal to the needle all land in its bucket in entry order, so the first walk hit is the first linear hit; the blob-resolution body factored into pri_blob_from_entry shared by walk and fallback. (32) DICTIONARY STORE BUCKETS (xaml_dictionary.c): dictionary_entry.next chain + store bucket_heads/tails/bucket_count; the flat array remains storage and iteration order — buckets only accelerate find; store_find_hash is the single scan shared by store_find and map_insert_entry (hash once per probe); buckets walk only the probe's bucket with an exact-hash filter before key_matches (ESSENTIAL: without it launch 1 showed 32781 compares/14.4s from mask collisions; with it 845); built at the 64th append (64 pow2 buckets), rehashed at 2× load, O(1) append, index moves with entries in store_adopt, dropped (rebuild whole) by map_remove/map_clear/store_clear; stored hash 0 = non-string key never matches hash-carrying probes. One regression found in test: the first bucket build's link branch ran with bucket_count==0 (mask 0xFFFFFFFF) — gated on an existing table. 27/27 fixtures (mrt/resource/dictionary/storage/bitmap/display/imaging families, both widths); native_style_x64/x32 remain RED pre-existing (12s client vs 5s timeout). Deployed both widths cmp-verified. VERIFICATION (launch-perf3-{1,2}): PRI lookups 61040 → 20 confirms; the ~20s defer-store scan tail gone; engine init 34.3s → 22.3s; microsoft.ui.xaml +41.0s (was 55.8), d3d12core +45.1s (was 60.7) with 10214 d3d12core calls incl. 92 vkCreateShaderModule — shader compilation active; dxgi reaches the CCD stage (7 EnumDisplayMonitors from dxgi thread 202); 71 threads; no E_BOUNDS misses. NEW WALL: the documented MCG rethrow fail-fast at +74.1s — RoOriginateLanguageException(80004002) from sharedlibrary+8feadc (the old swallowed-NRE site, now E_NOINTERFACE) → RaiseFailFastException +7e38de → exit 2147500034, through the dispatcher pump (shapr3d+2075fae … raven-xaml+2eaf/12ea0) — IID unnamed (DWRITE tracing needed); naming round in flight. NEXT COST (quantified): the boxed-string plumbing per defer probe, ~19.8s (rawbuf 10.6s × 25.7k + create 6.9s × 23.7k + delete 2.3s). Logs: launch-perf-1..3, launch-perf2-*, launch-perf3-1..2. Nothing committed.
- The ContentDialog cast-wall round: the refused face named and served, the dialog pipeline runs to ShowAsync (2026-11-22). (33) THE WALL NAMED (launch-wall-1, string traces ON from the start — RAVEN_TRACE_DEBUG=1 relays the OutputDebugString notes and the originate's message HSTRING prints verbatim): the MCG rethrow at +74 s (u-time 1186 Mµs in this run; heavy variance again) is `RoOriginateLanguageException(80004002 …) from sharedlibrary.dll+8feadc` with message `Unable to cast object of type 'EETypeRva:0x000EC670(Shapr3D.AppCommon.ConfirmDialog)' to type 'EETypeRva:0x00020888'` — a plain managed InvalidCastException (its standard HResult IS E_NOINTERFACE/80004002, no QI semantics required), raised from the marshaler stubs the dispatched-handler pump drives (stack shapr3d+2075fae/1b8a9c6/1b55c4d/1becf39 are .NET Native shared-fixup dispatch thunks, the callsite's target a 5-byte jmp stub table all pointing at sharedlibrary's per-return-address dispatcher). (34) NAMING EVIDENCE: the target EEType 0x20888 (Shapr3D.dll .rdata, flags word 0x0002 = interface) is unnamed because WinRT-projected interfaces carry no managed names; the owner is the app's ConfirmDialog composed over raven's ContentDialog inner. The decisive trace fired EARLIER in the same run — `raven-xaml: frame has no interface 38dc4404-d24e-40d8-9415-349464c1afdc` (launch-wall-1 line 275913, just before `page OnNavigatedTo 0x00000000`) — and 38dc4404-d24e-40d8-9415-349464c1afdc IS IContentDialog (verified against public generated metadata; IContentDialog2 = 2f93eb45-ee43-4303-9b38-3fe1a111ecbf; both winmds ship unnamed for reflection — Shapr3D.dll carries the app-managed ConfirmDialog only). The refused IButton/IGrid/ITextBlock QIs around it are the tolerated XBF element-kind probes. NEGATIVE RESULTS pinned so nobody re-chases them: ALL SIX QI-refusal gates (CORE_QI/PAGE_QI/ELEMENT_QI/ACT_REFUSAL/FRAME_DP/XBF_KEYS) stayed silent at the fatal moment (the fatal QI never landed on a traced raven face); RoGetMetaDataFile is NEVER called in any of the three runs (the flat-E_FAIL stub at platform_api.cpp:1560 is NOT in this path); the 80040111 register echoes in GetStringRawBuffer traces are a stale register value from the log start, not a fresh HRESULT. (35) FIX (windows_ui_xaml_contentdialog.h + xaml_controls.c): the two dialog faces join the standalone ContentDialog frame exactly as the header's old note anticipated — IContentDialog (40 slots: Title/TitleTemplate objects, FullSizeDesired, Primary/SecondaryButtonText HSTRINGs, Commands+Parameters, IsPrimary/SecondaryButtonEnabled bools, the five Closing/Closed/Opened/PrimaryButtonClick/SecondaryButtonClick pairs through raven_event_add/remove, Hide and ShowAsync answering E_NOTIMPL with a naming note) and IContentDialog2 (22 slots: CloseButtonText/Command/Parameter, three button Styles, DefaultButton, CloseButtonClick pair). Honest storage on new frame fields, default button-enabled flags true; part init in create_frame, event-list CS init in contentdialog_create (the flag arrives after create_frame), teardown in frame_release under standalone_contentdialog. Fixtures: xaml application/window/controls/input + resource_dictionary/storage/mrt/mrt_manager/mrt_extended/bitmap_transform/bitmap_decoder/display_monitor = 24/24 both widths. Deployed sys32+syswow64 cmp-verified. (36) VERIFICATION (launch-wall-3, profile reused — the wall reproduces on it): THE CAST WALL IS GONE — no InvalidCastException, no 80004002, no 2147500034. The dialog pipeline runs end to end on the served faces: ContentDialog activation → the ~35 k trace lines of the dialog document walk (407 ContentDialog mentions) → the app's dispatched handler → `raven-xaml: ContentDialog ShowAsync E_NOTIMPL (no presentation path yet)` — the trace note names the next wall from inside the fix. The unhandled NotImplementedException (80004001, "The method or operation is not implemented.") fail-fasts at the same pump stack (exit 2147500033) at u-time 3294 Mµs — 2.8× further than launch-wall-1, with 95 vkCreateShaderModule (up from 92), 7 EnumDisplayMonitors (CCD), 3 OnNavigatedTo, 67 threads alongside. (37) NEXT FRONTIER (characterized): ContentDialog.ShowAsync must return an honest IAsyncOperation<ContentDialogResult> — the codec_query.c codec_operation and the display_monitor FromIdAsync precedents are the patterns; completing the operation (None, or after a presentation path raises Opened and the button events) is the design decision the next round opens with, plus what the app does with a completed dialog. Logs: /tmp/raven-shapr-probe/launch-wall-1.log, launch-wall-2.log, launch-wall-3.log (fresh state st-wall-1-*). Nothing committed.

- The ContentDialog cast round (2026-11-22). (38) THE +74s WALL NAMED AND FIXED: a plain managed InvalidCastException (E_NOINTERFACE 80004002) — `Unable to cast object of type 'EETypeRva:0x000EC670(Shapr3D.AppCommon.ConfirmDialog)' to type 'EETypeRva:0x00020888'` — where 0x20888 = **IContentDialog {38DC4404-D24E-40D8-9415-349464C1AFDC}** (verified against public generated metadata; IContentDialog2 = 2F93EB45-…; raven's header comment had anticipated exactly this join). The owner: the app's ConfirmDialog composed over raven's ContentDialog inner (the standalone_contentdialog frame); trace evidence launch-wall-1:275913 `raven-xaml: frame has no interface 38dc4404-…`. Negative results pinned: all six QI-refusal gates silent at the fatal moment; RoGetMetaDataFile never called; the 80040111 register echoes are stale values, not fresh HRESULTs. FIX (windows_ui_xaml_contentdialog.h + xaml_controls.c, mirroring the UserControl/Page precedents): IContentDialog (40 slots) + IContentDialog2 (22 slots) with honest storage (Title/TitleTemplate, FullSizeDesired, button texts as HSTRINGs, Commands+Parameters, Styles, DefaultButton, enabled flags defaulting true), the six dialog events through raven_event_add/remove, Hide/ShowAsync E_NOTIMPL with a naming note; QI under standalone_contentdialog, part init in create_frame, teardown in frame_release. 24/24 fixtures both widths; deployed. VERIFIED (launch-wall-3): the cast wall GONE (zero 80004002/exit 2147500034); the dialog pipeline runs end to end (ContentDialog activation → ~35k dialog-document trace lines → dispatched handler → the fix's own note names the next wall: `ContentDialog ShowAsync E_NOTIMPL (no presentation path yet)`) → unhandled NotImplementedException (80004001, exit 2147500033), 95 vkCreateShaderModule, 7 EnumDisplayMonitors, 3 OnNavigatedTo, 67 threads. (39) NEXT: ShowAsync must return an honest completed IAsyncOperation<ContentDialogResult> — round in flight; the completion-timing decision gates everything downstream of a confirmed dialog. Logs: launch-wall-1..3, fresh state st-wall-1-*. Nothing committed.
- The ContentDialog.ShowAsync round: the honest completed operation, the await continues, the dialog wall is passed (2026-11-22). (40) THE DESIGN DECISION: without a presentation path there is no user interaction to wait for, and a pending-forever operation would hang the app's await silently — so ShowAsync completes the operation BEFORE the call returns, with ContentDialogResult None, the documented value for a dialog taken down without a button click ("No button was tapped", MS Learn winrt reference; None=0/Primary=1/Secondary=2 verified). Windows' own semantics complete ShowAsync when the dialog closes; a hidden dialog closes without a button click, so immediate-None is the honest minimal reading, and the loud trace note (`ContentDialog.ShowAsync -> completed None (no presentation path)`) names what happened on every launch. Hide() moved from E_NOTIMPL to S_OK with its own note (the dialog is already not visible; the app may call Hide between ShowAsync and the continuation). (41) THE GUID VERIFICATION (this round's decisive fact): the parameterized instance GUID algorithm is uuid5 (SHA-1) over namespace {11f47ad5-7b73-42c0-abae-878b1e16adee} with the signature `pinterface({<template>};<arg>)`, IAsyncOperation`1's template being 9fc2b0bb-e446-44e2-aa61-9cab8f636af2 (wine's windows.foundation.collections.idl names both) — cross-verified against EIGHT independent instances (mingw's precomputed IAsyncOperation`1<HSTRING> 3e1fe603, <Uri> 641cb9dd, <UserConsentVerificationResult> fd596ffd, <UserConsentVerifierAvailability> ddd384f3, raven's display_monitor 28c0e623, imaging's IIterable`1<BitmapCodecInformation> 97dfde96, and the app's own carried aa94d8e9 BitmapDecoder instance). The ARGUMENT ENCODING FOR AN ENUM IS enum(<type>;i4), NOT rc(): the task's rc-form hypothesis was wrong per this verification, and the rc-form value the task prescribed (needs an IContentDialogResult IID) is UNCOMPUTABLE because the metadata declares no IContentDialogResult — WinRT enums carry no default interface (wine idl + winmd shape). The operation therefore serves TWO faces: 1f23bdd1-06dc-5be9-9a60-0b4d94d4d72c (enum-form — the one Shapr3D.dll CARRIES as its ShowAsync projection constant, found by byte search at 0x14e1798, with the completed-handler instance 45c7a306-… adjacent, both reproduced by this computation) and d5476170-5fa4-5660-abec-08afbc016858 (the rc-form alternate computed over the instance GUID as the only face the metadata names for the enum). The imaging stamp's 8eb6ba3f "mingw variant" does NOT reproduce under 216 encoding/template/namespace combinations and is now identified as an earlier round's value of unexplained provenance — the app carries aa94d8e9, the verified computation. (42) THE FIX (windows_ui_xaml_contentdialog.h + xaml_controls.c): a stamp macro (RAVEN_CONTENTDIALOG_ASYNC_OPERATION, the display-header shape, GetResults carrying INT32* by value) plus a dialog_operation in xaml_controls.c with the storage.c/display_monitor.c sync-completion shape — parameterized face + separate IAsyncInfo face + IWeakReferenceSource + IAgileObject, put_Completed firing synchronously post-completion, finish-before-return, refusal traced by name. The x64 controls fixture deadlocked on first run and root-caused a REAL pre-existing bug: the factory_activate ContentDialog path (ActivateInstance) set standalone_contentdialog WITHOUT initializing the six dialog event critical sections (only the composable contentdialog_create path did), so frame_release's raven_event_close entered garbage — fixed by initializing the lists (+ the enabled-flag defaults) on that path too. Fixture: the dialog block in xaml_controls_client.c (exit codes 7800-7813: activation → IContentDialog QI → button-text roundtrip → ShowAsync → both parameterized IIDs answer → IAsyncInfo Status Completed → put_Completed handler invoked inside the call with GetResults 0 → GetResults 0 → Hide S_OK). 32/32 xaml-family tests both widths; deployed system32+syswow64 cmp-verified. (43) LAUNCH VERIFICATION (launch-show-3, the wall-3 recipe PLUS RAVEN_TRACE_DEBUG=1 — see (44)): the dialog pipeline runs THROUGH the wall — ContentDialog activation → the ~34k dialog-document walk (zero 38dc4404 refusals) → DialogManager.TryShowAsync (same call shape as wall-3: ConfirmDialog string 0x1f → DialogManager.cs 0x6b → TryShowAsync 0xc) → `ContentDialog.ShowAsync -> completed None` → the .NET runtime's ONE internal probe 11d3b13a-180e-4789-a8be-7712882893e6 refused by name and TOLERATED (a SharedLibrary.dll runtime GUID at 0x55f410, absent from the app's own MCG constants; the await takes the generic path) → **page OnNavigatedTo fired** (the await's continuation ran; wall-3's unhandled NotImplementedException 80004001/exit 2147500033 is GONE — the only 80004001s left are the two tolerated XBF-skip notes) → the app continued (element creation, the comctl32 window-class registrations) → exit 3 at guest thread 206 (the RENDER-THREAD abort wall, the same killer show-1/show-2 died at earlier). ShowAsync verdict: HONEST COMPLETION VERIFIED; the dialog pipeline no longer gates the app. (44) LAUNCH RECIPE discovered en route: the app's startup has a device-creation race — both no-debug-tracing launches (show-1/show-2) diverged BEFORE the dialog pipeline entirely (D3D init on thread 1, zero ConfirmDialog/TryShowAsync/XBF activity, dxgi config never read, render-thread abort at exit 3), while every launch with RAVEN_TRACE_DEBUG=1 (wall-1..3, show-3) walks the dialog pipeline. RAVEN_TRACE_DEBUG=1 is part of the reproducible-dialog launch command. (45) NEXT FRONTIER (characterized): the render-thread abort → exit 3 (thread does GetStdHandle/time-formatting then stops; in show-3 it fires ~40k lines AFTER the dialog completed, right after comctl32 window-class setup) — the app's own abort path, the same wall (11)/(12) named; with the dialog pipeline passed, this is now the earliest wall standing between raven and a living Shapr3D window. The two dialog probe IIDs the await tolerated (11d3b13a; fd104460/280335ae/ae2d9271 are the known element-kind probes) are named in the trace if the next round needs them. Logs: /tmp/raven-shapr-probe/launch-show-1..3.log (show-3 is the verified one), fresh state st-show-1-*. Nothing committed.

- The ShowAsync round (2026-11-22). (40) ShowAsync returns an honest, already-completed IAsyncOperation'1<ContentDialogResult> finishing with ContentDialogResult::None (the documented no-button value; None=0/Primary=1/Secondary=2 verified against MS Learn) + a loud trace note; Hide() moved E_NOTIMPL → S_OK (nothing visible to hide); the dialog_operation mirrors the storage.c/display_monitor.c sync-completion shape (parameterized face + separate IAsyncInfo face + weak source + agile marker, put_Completed fires synchronously post-completion, refusals traced by name). (41) GUID VERIFICATION, decisive: the instance GUID is uuid5(SHA-1) over namespace {11f47ad5-7b73-42c0-abae-878b1e16adee} of pinterface({9fc2b0bb-…};<arg>) — cross-verified on 8 independent instances; FOR AN ENUM the argument encoding is `enum(...;i4)`, NOT rc() (and the prescribed rc-form is uncomputable: WinRT enums carry no default interface — there is no IContentDialogResult). The op serves BOTH faces: 1f23bdd1-06dc-5be9-9a60-0b4d94d4d72c (enum-form — byte-search-verified as the constant Shapr3D.dll itself carries for ShowAsync, handler 45c7a306 adjacent) and d5476170-… (rc-form alternate). SIDE FINDING: imaging's stamp 8eb6ba3f does not reproduce under 216 encoding variants — unexplained provenance; the app carries aa94d8e9 (the rc-form of IAsyncOperation'1<BitmapDecoder>). (42) BONUS BUG: the factory_activate ContentDialog path (ActivateInstance) set standalone_contentdialog without initializing the six dialog event critical sections — deadlocked frame_release on x64; fixed (CS init + enabled-flag defaults). 32/32 xaml-family fixtures both widths (new dialog block, exit codes 7800-7813, in xaml_controls_client.c). Deployed both widths. (43) LAUNCH RECIPE DISCOVERY: RAVEN_TRACE_DEBUG=1 is part of the reproducible-dialog launch (show-1/2 without it diverged before the dialog pipeline — D3D init on thread 1, zero ConfirmDialog activity). (44) VERIFIED (show-3, the wall-3 recipe): THE WALL IS PASSED — ContentDialog activation → ~34k dialog-document walk (zero 38dc4404 refusals) → DialogManager.TryShowAsync → `ContentDialog.ShowAsync -> completed None` → the runtime's single internal probe 11d3b13a-180e-4789-a8be-7712882893e6 refused by name and TOLERATED (a SharedLibrary runtime GUID) → page OnNavigatedTo FIRED (the await's continuation ran where the NotImplementedException fail-fast stood) → app continued (element creation, comctl32 window-class registration) → exit 3 at guest thread 206. (45) NEXT FRONTIER: the RENDER-THREAD ABORT (thread does GetStdHandle + time-formatting then stops) — now the earliest wall between raven and a living Shapr3D window; decode round in flight. Logs: launch-show-1..3. Nothing committed.

- The render-thread abort decode round (2026-11-22). (46) THE MECHANISM, pinned end to end: the wall was never a raven stop — the render thread calls the CRT's abort() and raven's ucrtbase serves it as ExitProcess(3) SILENTLY (the served-exit check in providers.cpp's api loop precedes the RAVEN_TRACE_API trace block, so no api line names it; the ProcessExit thrown inside a WORKER thread's invoke propagates through thread_main's catch(...) — printed as the misleading "ended with a stop" — is stored in the scheduler's pending, rethrown by the main thread at check_pending into run_native_process's ProcessExit handler, whose exit_result runs terminate_others + the DLL detach notifications on thread 1 and answers NativeResult{exited,3} — exactly the observed "Native process exited with status 3" with the EventUnregister/FlsFree/TlsFree/FreeLibrary tail on thread 1; raven's ExitProcess serving from any thread is untraced BY DESIGN order, and ucrtbase.c's `CRT void abort(void) { ExitProcess(3); }` is the only ExitProcess(3) the CRT's fail paths reach: abort (line 605), _assert (606), _invalid_parameter_noinfo_noreturn (642)). (47) THE DECODE LADDER, three changes shipped and each verified in its own launch: (a) NativeThreads::thread_main's catch now decodes what it caught — ProcessExit prints "guest thread N ended the process with exit code X (the guest's own exit call, last API Y)" and a NativeResult prints its stop kind (fault+signal+instruction, syscall, api, exception, exit code) — the first launch under it turned the bare "ended with a stop" into the wall's name; (b) raven's ucrtbase abort()/_assert()/_invalid_parameter_noinfo_noreturn() say which fail path ran through OutputDebugStringA ("raven-crt: abort at 0x…", the failing function's return address via __builtin_return_address, the note kept free of CRT machinery so a broken runtime cannot re-enter it) — launch-render-2 fired it ("debug: raven-crt: abort at 0x184619339") and pinned the exit to abort() itself (not assert, not invalid-parameter); (c) the served ExitProcess/RtlExitUserProcess now prints the guest caller chain through KernelContext::describe_stack ("raven: ExitProcess(3) from ucrtbase.dll+9160 shapr.native.dll+a39339 … module+offset each, 14 frames") before abandoning the call — the launch-3 decode named everything. (48) WHAT THE RUN REACHES BEFORE THE ABORT (render-1/2/3, --stdio=inherit added to the recipe so the guest's stderr lands in the log — GetStdHandle answered from launch->standard_fds, and the vkd3d-proton log became readable in place): dialog pipeline → ContentDialog.ShowAsync completed None → page OnNavigatedTo fired → TWO full vkd3d-proton device inits (SM 6.6, DXR 1.1, descriptor buffers, pipeline-library disk cache) → TWO composition swapchains ("Creating swapchain (1 x 1), BufferCount = 3", frame-latency 3 with KHR_present_wait) → 94 vkCreateShaderModule → 2504 vkGetPhysicalDeviceImageFormatProperties2 + 421 vkGetPhysicalDeviceFormatProperties2, ALL returning VK_SUCCESS → 4 MB mapped buffers, worker threads spawned (CreateThread bursts, DLL_THREAD_ATTACH walks) → ZERO vkQueueSubmit / vkQueuePresentKHR / vkCreateSwapchainKHR across every run (vkd3d defers the real swapchain to first Present, and no frame is ever submitted) → at ~58 s the render thread parks on the shared pool condvar (msvcp140_app+12085, the std::condition_variable wrapper), wakes, fetches stderr twice and formats two timestamps (msvcp140_app+12e3f precise-time helper + ucrtbase _localtime64_s) with NO write ever reaching WriteFile, and aborts. (49) THE FATAL CHECK, named by the launch-3 chain (shapr.native.dll+0xa39339's IAT resolved to api-ms-win-crt-runtime!abort; the caller .pdata-function + strings): Shapr.Native.dll's material-system parser factory at +0xa36d1e calls its fatal reporter (+0xa392b0, which formats the message and ends with `call abort; int3`) with the format "the material was not built for the %s backend and %s shader language\n" at source line 62 of "createParser", where the %s pair comes from the material object's backend tag (1=OpenGL, 2=Vulkan, 3=Metal, 4=Noop, 0=Unknown) and shader-language tag ('ESSL 1.0', 'ESSL 3.0', 'SPIR-V', 'MSL'; expected-language 4 branches away as a handled case) — the report's two timestamped lines were BUILT in memory and never written (fputs never ran), which is why the app's own message was invisible until the chain decode. THE ABORT IS THE APP'S OWN DECISION: a material reached the parser whose (backend, shader-language) pair the parser does not serve. (50) FIXES SERVED THIS ROUND: (a) "NtDelayExecution" added to the ntdll export catalog (providers.cpp nt_names, with the vkd3d-proton comment — its swapchain setup does GetProcAddress("NtDelayExecution") at run time and got the "NtDelayExecution not found in ntdll" fixme; the serving existed at thread_api.cpp all along) — the fixme is GONE in render-2/3, verified against render-1's message capture; (b) the raven-crt loud fail paths (47b); (c) the thread_main stop decode (47a); (d) the ExitProcess caller chain (47c). ucrtbase.dll rebuilt from source both widths through the canonical builders (scripts/provision-controls.py crt target for x64 — the component load line now shows 234883 bytes vs the old 215401, making which binary a run used self-evident — and the CMake ucrtbase32 command for x86, 292980 bytes), raven rebuilt with the three native-side changes; both deployed by the build into build/native (the load path the app's run actually uses, "component ucrtbase.dll from …/build/native/ucrtbase.dll"). TESTS: native_waits_and_alerts extended with check 13 (GetProcAddress on the synthetic ntdll resolves "NtDelayExecution" and the pointer sleeps like the import — the exact vkd3d path) — passes; native_xaml family 8/8; ucrt family 7/7; core/dispatcher/storage/controls/dialog/setting 26/28 where the two native_dialog_font_x failures are the PRE-EXISTING x64 GetTextMetricsW/GetTextFaceW gap (testdata/native_loader/dialog_font_client.c, commit e84d2149 needs those gdi32 exports served on x64; unrelated to this round). (51) NEXT FRONTIER, now precise: WHY the material's (backend, shader-language) tag mismatches the parser under raven — the material objects and their tags come from the app's own assets/pipeline (the parser expects the D3D12 renderer's pair, which its own enum names "Vulkan"+SPIR-V), so the next round names the pair the fatal was about (the two %s values, capturable by tracing the fatal reporter's string args or a checkpoint at +0xa36d1e) and works back to which raven-served answer (device/driver/adapter query or asset parse) steered the material to the wrong pair; behind it stands the frameless GPU pipeline (zero submits) that the material pipeline gates. Logs: /tmp/raven-shapr-probe/launch-render-1..3.log (render-3 is the decoded one), message captures render-1/2/3-messages.txt, fresh state st-render-1..3-*. Nothing committed.

- The render-thread abort decode round (2026-11-22). (46) THE WALL IS NOT A RAVEN STOP: the render thread calls the CRT's abort() — raven's ucrtbase serves it as ExitProcess(3) silently (untraced served-exit order), the ProcessExit from a worker thread reaches thread_main's catch(...) as the misleading "ended with a stop", and the main thread's ProcessExit handler runs terminate_others + detach-on-thread-1 and answers status 3 — every observed detail of the wall explained. (47) DECODE LADDER SHIPPED: thread_main's catch decodes ProcessExit/NativeResult by kind; raven's ucrtbase abort/_assert/_invalid_parameter print "raven-crt: abort at 0x…" via OutputDebugStringA before exiting; the served ExitProcess prints the full guest caller chain via describe_stack. All three fired and agree across render-2/3. (48) RUN REACH: dialogs → OnNavigatedTo → 2 vkd3d devices → 2 composition swapchains → 94 shaders → 2925 format probes (all VK_SUCCESS) → buffers + workers → ZERO submits/Presents → ~58 s in, the render thread wakes from the pool condvar, builds two timestamped fatal lines that never reach WriteFile, and aborts. (49) THE FATAL NAMED: Shapr.Native.dll's material parser factory, "createParser: the material was not built for the %s backend and %s shader language" (backend OpenGL/Vulkan/Metal/Noop/Unknown × language ESSL 1.0/ESSL 3.0/SPIR-V/MSL, line 62) — the app aborted on its own material/backend pair check. (50) FIXES: NtDelayExecution in the ntdll export catalog (vkd3d's runtime lookup; fixme gone) + native_waits fixture check 13; raven-crt loud fail paths; the stop decode; the ExitProcess chain. ucrtbase rebuilt both widths (234883/292980 bytes — the component load line names the binary). Tests: waits 1/1, xaml 8/8, ucrt 7/7, core/controls/dialog 26/28 (two pre-existing GetTextMetricsW x64 gaps, unrelated). (51) NEXT: name the (backend, language) pair the fatal was about and trace which raven-served answer steered the material to the wrong pair; behind it, the zero-submit GPU pipeline the material pipeline gates. Logs: launch-render-1..3. Nothing committed.

- The material-fatal decode round (2026-11-22). (46) THE "RENDER-THREAD ABORT" WAS NEVER A RAVEN STOP: the app's own abort() — Shapr.Native.dll's material-parser factory calls its fatal reporter (IAT api-ms-win-crt-runtime!abort; sites near +0xa39339/+0xa36d23; source "createParser", line 62) with the format **"the material was not built for the %s backend and %s shader language\n"** (backend enum {Unknown, Noop, OpenGL, Vulkan, Metal}; shader language {ESSL 1.0, ESSL 3.0, SPIR-V, MSL}) into two timestamped stderr lines that were built in memory but whose fputs never ran (the GetStdHandle+time-formatting tail) — then abort() → raven's ucrtbase `CRT void abort(void){ExitProcess(3)}` → the served ExitProcess runs BEFORE the trace block (hence no api line) → ProcessExit{3} in a worker thread's invoke → the misleading "guest thread N ended with a stop" → the scheduler rethrows on the main thread → terminate_others + DLL detaches → `Native process exited with status 3`. Every artifact of the old wall explained. (47) THREE DECODE CHANGES SHIPPED, all fired and agreed across launches: thread_main's catch now NAMES what it caught (ProcessExit as "ended the process with exit code N (the guest's own exit call, last API X)"; NativeResult by stop kind/signal/instruction) in native/nt/threads.cpp; ucrtbase abort/_assert/_invalid_parameter_noinfo_noreturn print `raven-crt: abort at 0x…` (return address) before exiting (raven-crt/ucrtbase.c, rebuilt both widths via provision-controls.py crt + CMake ucrtbase32); the served exit prints describe_stack() — `raven: ExitProcess(3) from <module+off>×14`. (48) REAL SERVED-SURFACE GAP FIXED: **NtDelayExecution added to the ntdll export catalog** (providers.cpp nt_names) — the serving existed (thread_api.cpp:504), the export didn't; vkd3d-proton's swapchain setup does GetProcAddress("NtDelayExecution") at run time (the long-standing "NtDelayExecution not found" fixme is gone); fixture native_wait_client.c check 13 covers both the export resolution and the sleep. Tests: waits 1/1, xaml 8/8, ucrt 7/7; the two native_dialog_font_x failures are the pre-existing GetTextMetricsW/GetTextFaceW gap, unrelated. (49) REACH NOW: dialogs → ShowAsync completed None → OnNavigatedTo → TWO full vkd3d device inits → two composition swapchains ("Creating swapchain (1 x 1), BufferCount = 3", KHR_present_wait) → 94 vkCreateShaderModule → 2925 format probes VK_SUCCESS → 4 MB mapped buffers → ZERO vkQueueSubmit/vkQueuePresent/vkCreateSwapchainKHR (vkd3d defers the real swapchain to first Present — no frame is ever submitted) → the material fatal at ~58 s. (50) NEXT: name the (backend, shader-language) pair the fatal names and the raven-served answer that steered the selection — round in flight. Logs: launch-render-1..3 + messages captures. Nothing committed.
- The material-fatal steering round (2026-11-22). (52) THE PAIR, NAMED AND TRACED END TO END: the fatal "the material was not built for the %s backend and %s shader language" (Shapr.Native.dll+0xa36d1e, line 62 of createParser) fired for **(OpenGL, ESSL 1.0)** — the only reachable failing pair: the engine factory (0x180a33250) defaults the backend byte to 1=OpenGL and returns NULL ("Selected backend not supported in this build") for Vulkan/Metal (2/3), Noop (4) skips the check, and ≥5 derives language tag 1; the language tag derives from a renderer virtual (vtable+0x180 = read of renderer+0x13d9, the shader-version level): tag 0 = ESSL 1.0 wants chunks "MAT_ESS1"+"DIC_TEXT", tag 1 = ESSL 3.0 wants "MAT_GLSL"+"DIC_TEXT" — and the material packages (36 embedded in Shapr.Native.dll's blob at 0x182c79540 + the 12 Assets/VisualizationAssets/Materials/opengl/*.bin) contain EXACTLY ONE language chunk each, MAT_GLSL+DIC_TEXT (ESSL 3.0), nothing else — so tag 0 has no chunk anywhere and the parser aborts. (53) THE STEERING, DECODED TO THE RAVEN-SERVED BYTE: the shader-version flag (renderer+0x13d9, written by the caps-init 0x180a9d3d0 at +0x13d9, default 1) is forced to 0 when sscanf(glGetString(GL_VERSION), "OpenGL ES %d.%d") yields major==2 — ANGLE delivered an ES2-class context because the app creates its ANGLE device via **D3D11On12CreateDevice(pDevice, 0x28, pFeatureLevels=NULL, 0, ...)** (Shapr.Native.dll call at 0x182665337) and DXVK's D3D11On12CreateDevice, for a NULL feature-level list, asks the D3D12 device (vkd3d) for its MAX supported level and answers **D3D_FEATURE_LEVEL_12_1** (dxvk d3d11_main.cpp: defaultFeatureLevels = {11_0, 11_1, 12_0, 12_1} → CheckFeatureSupport → 12_1), while Windows' D3D11-on-12 caps at **11_1** (D3D11 feature levels end at 11_1; the documented NULL default is 11_0) — and ANGLE's D3D11 caps logic (exact-match tables: shader-model map {9_3,10_0,10_1}→SM4, {11_0,11_1}→SM5, default SM0 at libGLESv2+0x1c1250; caps copy gated on FL==0xb000 at +0x1a5492) treats 12_x as unknown and the renderer lands on the ES2 path. (54) THE FIX (honesty rule: answer what Windows-on-this-hardware answers): the runtime's DXVK d3d11.dll default feature-level array (pe64 file offset 0x60d980, pe32 0x6665e0 — right after the "D3D11On12CreateDevice" log strings) patched {11_0, 11_1, 12_0, 12_1} → {11_0, 11_1, 11_1, 11_1}, applied to components/graphics/dxvk/{pe64,pe32}/d3d11.dll and the deployed build/{native,merge}/native/d3d11.dll, sha256 entries in components/graphics/installed.json updated; no raven source change. (55) VERIFIED IN ONE LAUNCH (launch-mat-4, full recipe incl. dxgi.enableDummyCompositionSwapchain=True): the D3D11On12 log line now reads "Chosen feature level: D3D_FEATURE_LEVEL_11_1" (was 12_1); the material fatal is GONE (no abort, no ExitProcess, zero raven-crt hits through the whole 180 s timeout); the app's new reach: the two vkd3d composition swapchains still create, then the GPU pipeline goes live — 94 vkCreateShaderModule, **49 vkCreateGraphicsPipelines + 68 vkCreateComputePipelines (zero before — the first pipelines of the whole porting effort)**, 48 semaphores, images bound, render-loop threads alive with vkWaitForFences succeeding on thread 242 — the run then ended at ~the timeout boundary on a NEW, later wall (see 57). (56) SIDE FINDINGS: (a) launch-mat-3 accidentally ran WITHOUT RAVEN_GUEST_DXVK_CONFIG and showed the dummy-composition-swapchain config is load-bearing: DXVK's DXGI answers "DxgiFactory::CreateSwapChainForComposition: Not implemented" (Windows implements it; UWP composition hosting requires it), the app tears down both D3D12 devices, retries with standalone D3D11CreateDevice (0x180973cab), fails and aborts at shapr.native+0x973b62 (its own assert in the fallback path) — keep the config in every Shapr3D launch recipe; (b) the fatal reporter's two timestamped stderr lines never reach WriteFile because the emit path (0x180a391c0 → logger 0x18342eee8 → 0x180a382e0/38260/38340 → _strdup into the logger's sink) buffers the line and drops it on abort — raven's CRT stdio is write-through, so the drop is in the app's own logger; (c) ANGLE (libEGL/libGLESv2) is the app's own copy and runs over the app's D3D11On12 device, so ANGLE's context version is steered entirely through the FL answer. (57) NEXT FRONTIER: the run's end is now a NEW wall past the material parse — on the render thread the app loads the Win2D DLLs (Microsoft.Graphics.Canvas.Text.dll, Microsoft.Graphics.Canvas.dll via shapr.native+0xf1c6c), then throws winrt::hresult_error **0x800401F8 (CO_E_CLASS_NOT_AVAILABLE)** (RoOriginateLanguageException from shapr.native+0xb64ed, followed by RaiseException e06d7363 → uncaught → terminate → abort; chain vcruntime140_1_app+1ab1) — Win2D's WinRT class activation is not served yet (route the Microsoft.Graphics.Canvas.* activation factory to the DLL's DllGetActivationFactory, or fix its dependency load); behind it, the first vkQueueSubmit/vkQueuePresentKHR (vkd3d's present path + the D3D11/ANGLE swapchain) and XAML composition hosting of the presented frames. Logs: /tmp/raven-shapr-probe/launch-mat-1..4.log (mat-4 = the verified run), fresh state st-mat-1..4-*. Nothing committed.

- The material-pair round (2026-11-22). (52) THE PAIR NAMED: (OpenGL, ESSL 1.0) — createParser (Shapr.Native.dll+0xa36d1e, "createParser" line 62) derives it from engine+0xc (backend byte) + renderer virtual vtable+0x180 (shader-version level): tag 0 = ESSL 1.0 needs chunks MAT_ESS1+DIC_TEXT, tag 1 = ESSL 3.0 needs MAT_GLSL+DIC_TEXT; backend byte 1 (the engine factory 0x180a33250 returns NULL for Vulkan/Metal — "Selected backend not supported in this build" — so Noop/Unknown can't fatal), flag 0 (the fatal fired). The flag mechanics: caps-init 0x180a9d3d0 runs sscanf(glGetString(GL_VERSION), "OpenGL ES %d.%d") and forces flag=0 exactly when major==2. (53) ASSET INVENTORY: every package holds exactly one language chunk — MAT_GLSL+DIC_TEXT (ESSL 3.0): 36 packages in Shapr.Native.dll's blob at 0x182c79540 ("Filament Default Material"…) plus 12 Assets/VisualizationAssets/Materials/opengl/*.bin; ZERO MAT_ESS1/MAT_SPIR/MAT_METL — ES2-class context is fatal by design. (54) THE STEERING INPUT: the app calls **D3D11On12CreateDevice(pDevice, 0x28, pFeatureLevels=NULL, 0, …)** (Shapr.Native 0x182665337) to create ANGLE's D3D11 device over its vkd3d D3D12 device; DXVK 3.1.1's NULL-list default array is {11_0,11_1,12_0,12_1} answered with the D3D12 device's max → **FL 12_1**; Windows' D3D11-on-12 caps at 11_1 (D3D11 never reports 12_x); ANGLE's exact-match FL table ({9_3,10_0,10_1}→SM4, {11_0,11_1}→SM5, default SM0) treats 12_1 as unknown → ES2 path → flag 0 → the parser wants the nonexistent ESSL 1.0 chunks → abort. FIX (Windows-honesty): DXVK's default feature-level array clamped ≤11_1 ({11_0,11_1,11_1,11_1}) in components/graphics/dxvk/pe64/d3d11.dll (file offset 0x60d980) + pe32 (0x6665e0) + build/native + build-merge copies, installed.json updated. SIDE FINDING (launch-mat-3 control): RAVEN_GUEST_DXVK_CONFIG='dxgi.enableDummyCompositionSwapchain=True' is load-bearing — without it DXVK refuses CreateSwapChainForComposition and the app aborts at shapr.native+0x973b62. (55) VERIFIED (launch-mat-4): "Chosen feature level: D3D_FEATURE_LEVEL_11_1" (was 12_1); the material fatal GONE (no abort through the full 180 s); the GPU pipeline went LIVE — 94 shader modules, **49 vkCreateGraphicsPipelines + 68 vkCreateComputePipelines (the first pipelines of the whole effort)**, images/semaphores bound, render-loop threads alive with vkWaitForFences succeeding. (56) NEXT FRONTIER: the first vkQueueSubmit/vkQueuePresentKHR (vkd3d present path + the new D3D11/ANGLE swapchain), then XAML composition hosting of presented frames. Logs: launch-mat-1..4. Nothing committed.
- The Win2D load-chain round (2026-11-22). (58) WHAT ACTUALLY SITS BETWEEN "PIPELINES BUILT" AND THE FIRST SUBMIT: not a missing raven signal — the app is still in one-time startup initialization, and four Windows-servable walls fell in sequence, each killing the process before the next. The render threads' waits during the ~117 s of pipeline work are the app's own: vkWaitForFences from d3d12core.dll+9799f on thread 242 succeeds against CPU-signaled vkd3d fences (no GPU work has ever been queued — zero vkQueueSubmit), thread 254 polls the frame-latency-family handles (0x1d4c/0x173c) at zero timeout from shapr.native.dll+267d9dd/+267b794, thread 18 sleeps 100 ms on a pool condvar — all live, none starved. The chain the app dies along: Win2D activation → render-target creation → SVG interop, i.e. **the material pipeline's UI surface, not the frame loop**. (59) WALL 1 — Win2D LOADS NOT: the app activated Microsoft.Graphics.Canvas.Text.CanvasTextFormat (~125 s, thread 202) and microsoft.graphics.canvas.dll failed to load on three missing imports — **d2d1.dll!#13**, **d3d11.dll!CreateDirect3D11DeviceFromDXGIDevice**, **d3d11.dll!CreateDirect3D11SurfaceFromDXGISurface** — then RoOriginateLanguageException(800401f8) → winrt::hresult_error → abort → ExitProcess(3). Two raven fixes: (a) **raven-d2d1's ordinal table was Windows-wrong** — the .def assigned Windows' exports alphabetically (1 CreateFactory, 5 CreateDeviceContext), but the system d2d1.dll's true table is 1 CreateFactory, 2 MakeRotateMatrix, 3 MakeSkewMatrix, 4 IsMatrixInvertible, **5 InvertMatrix**, 6 ConvertColorSpace, 7 CreateDevice, 8 CreateDeviceContext, 9 SinCos, 10 Tan, 11 Vec3Length, 12 ComputeMaximumScaleFactor, **13 GetGradientMeshInteriorPointsFromCoonsPatch** (strontic xcyclopedia + wine's spec agree for 1..12; Win2D imports exactly #1/#5/#13) — the .def now pins every ordinal explicitly and D2D1GetGradientMeshInteriorPointsFromCoonsPatch is implemented (the interior tensor points of the Coons patch, derived from the mixed-partial matching and verified against the bilinear patch: point11 = p10+p1−p0+q0+r0−corner/9 with q the bottom-minus-top edge tangents, r the right-minus-left, corner the bilinear corner surface's curvature; point12/21 step along each axis, point22 closes); (b) **the two Direct3D-WinRT interop constructors** — Windows' d3d11.dll exports them, DXVK's build does not, so raven serves them from a new component `raven-d3d11interop.dll` (native/guest/raven-d3d11interop, provision-controls.py target): each wraps the COM pointer in the Windows.Graphics.DirectX.Direct3D11 runtime object (IInspectable + IDirect3DDevice/IDirect3DSurface + IDirect3DDxgiInterfaceAccess::GetInterface unwrapping; IDirect3DSurface::Description is the traced E_NOTIMPL). The loader serves a guest's import of them from the component when the Direct3D library lacks them: `ModuleSet::serving_module` (native/pe/module_set) answers "raven-d3d11interop.dll" for the two names, ModuleSet::resolve falls through to the component when it is loaded, and providers.cpp's import loop starts the component on the missing-export report. No DXVK binary touched. (60) WALL 2 — WindowsStringHasEmbeddedNull: Win2D's factory path called it the moment it loaded; raven's catalog had the name without a handler. Implemented in platform_api.cpp beside the other HSTRING APIs per the contract (HRESULT; S_OK, *hasEmbedNull true only when a null code unit lies before the counted length; E_INVALIDARG on a null out). (61) WALL 3 — IDWriteTextLayout3: CanvasTextLayout's creation QI'd the text layout for **{07ddcd52-020e-4de8-ac33-6c953d83f92d} = IDWriteTextLayout3** (not TextLayout1, whose GUID raven already answered) and got E_NOINTERFACE → 80004002 → abort. raven-dwrite's text layout now carries the derived interface table slots and answers TextLayout2/3 on a layout, TextFormat2/3 on a format: TextLayout2 = GetMetrics(DWRITE_TEXT_METRICS1) + the eight Format1 methods (raven's f1_* already implemented them on the shared object), TextLayout3 = InvalidateLayout + the DWRITE_LINE_SPACING struct set/get + GetLineMetrics(DWRITE_LINE_METRICS1) (leading fields answer zero — the layout tracks no uniform-spacing distribution), TextFormat2 = the line-spacing struct, TextFormat3 = font-axis values (stored, ≤32) + automatic axes. (62) WALL 4 — the canvas service is Skia-gated: the app then created a **CanvasRenderTarget (100×100)** and raven-d2d1's bitmap creation called RavenCanvasCreate — registered only under RAVEN_SKIA, and this build has none (RAVEN_SKIA_LIBRARY empty; upstream/skia is a stub). Without Skia the canvas service now still serves the surfaces themselves (native/nt/canvas_service.cpp #else branch): real BGRA buffers (create/destroy/size/pixels with row-copied bounds checks), and drawing commands are **dropped loudly by name** ("raven: canvas draw dropped, no Skia rasterizer in this build", throttled to the first 20 then every 1000th), the batch answering 0 failed so the queue keeps going. A Skia provision (scripts/provision-text.py) replaces the drops with rasterization. (63) WALL 5 — CreateStreamOverRandomAccessStream: the app then activated **Microsoft.Graphics.Canvas.Svg.CanvasSvgDocument** and died on the shcore interop (api-ms-win-shcore-stream-winrt-l1-1-0 → apiset shcore.dll, catalogued, unserved). Implemented as a real guest export of raven-xaml.dll (native/upstream/raven-xaml/stream_over.c): the **IStream over the IRandomAccessStream** the function builds (direction per shcore docs: an IStream around the WinRT stream, for IStream consumers such as D2D's CreateSvgDocument) — Read through GetInputStreamAt(position) + one ReadAsync into a raven Buffer (shared raven_xaml_buffer_create helper from data_writer.c's stream_buffer_create) with the bytes copied out, Seek/SetSize/Stat/Clone/CopyTo real, Write the traced gap; apiset.cpp maps the stream-winrt contract to raven-xaml.dll ahead of the generic shcore entry. Verified by build + export table + the ctest families (34 loader/pe/xaml/ucrt tests green; native_edit_text_contract and native_dwrite fail on the pristine tree too — the known GetTextMetricsW x64 gap). **NOT verified by launch — the launch budget (4) was spent reaching this point; the next run starts past the canvas service and dies (if at all) at raven-d2d1's ID2D1DeviceContext5::CreateSvgDocument stub, the named next wall (an SVG document model, behind it Skia for the D2D pixels).** REACH after four fixes (launch-present-1..4, logs in /tmp/raven-shapr-probe): Win2D loads (launch-present-2 mapped microsoft.graphics.canvas.dll from the app dir), CanvasTextFormat + CanvasTextLayout create (launch-present-3, past the TextLayout3 QI), CanvasRenderTarget creates and canvas batches flow (launch-present-4, "canvas batch dropped" lines), then the SVG interop. Still ZERO vkQueueSubmit/vkQueuePresentKHR — the app has never left UI construction; the frame loop's first pass waits on the app, not on a raven signal. Nothing committed.
- The SVG round (2026-11-22). (64) THE PREVIOUS ROUND'S REPORT SAID THE SVG FIX WAS "already in tree but unverified" — it was not: grep found raven-d2d1's ID2D1DeviceContext5::CreateSvgDocument still the bare stub (slot 115, unimplemented("ID2D1DeviceContext5", "CreateSvgDocument")); the only in-tree piece was the stream_over.c fix (63), itself launch-unverified. The whole SVG document model was written this round: d2d1_tables.h grows stub tables for the eight d2d1svg.h interfaces (ID2D1SvgDocument 15 slots, ID2D1SvgElement 34, ID2D1SvgAttribute 6, ID2D1SvgPaint 13, ID2D1SvgStrokeDashArray 12, ID2D1SvgPointCollection 10, ID2D1SvgPathData 15, ID2D1SvgGlyphStyle 9 — the member order and GUIDs taken from the system's d2d1svg.h, included with D2D_USE_C_DEFINITIONS so the enums and IIDs come with it; the attribute-derived interfaces inherit GetElement/Clone at slots 4/5, and the UpdateDashes/GetDashes and Set/GetAttributeValue overloads get _1/_1_1 slot-name suffixes); d2d1.c gains the model (a ~1500-line section before the method tables): the XML of the stream parsed into an element tree (tags, text-content children with empty tags, attributes in document order; entities &amp;/&#NN;/&#xHH;, CDATA, comments, declarations, DOCTYPE with internal subset skipped; UTF-8/UTF-16LE/UTF-16BE with byte order marks via svg_read_stream; whitespace-only text runs skipped; >1 top-level element or a mismatched close tag is the parse error CreateSvgDocument answers E_INVALIDARG to, as Windows does), attributes stored as parsed text or as the typed POD value or as the attribute object the program set/asked for (POD get parses the text per type: FLOAT/COLOR with #rgb #rrggbb #rrggbbaa rgb()/rgba()/the common palette/MATRIX/LENGTH with %-units/the enum families/PRESERVE_ASPECT_RATIO/VIEWBOX; POD set formats back for a text read; GetAttributeValue(REFIID) materializes an ID2D1SvgPaint for fill/stroke (incl. url(#id) forms), a stroke-dasharray or a points collection from the parsed text — the d attribute's ID2D1SvgPathData is NOT materialized (E_NOINTERFACE, traced: the SVG path grammar is its own work, the trace names the ask)), CreateChild appends (Win2D's CreateAndAppend* rely on it), tree mutation moves elements within their document (cross-document answers D2DERR_WRONG_RESOURCE_DOMAIN), Serialize writes the tree back to XML in UTF-8 through the caller's IStream (declaration line included, text/attribute escaping, attribute objects rendered by their paint state), Deserialize parses a fragment kept apart from the document, FindElementById walks for the id attribute, GetViewportSize is the one value-return member (D2D1_SIZE_F, 8 bytes → RAX per the x64 ABI), ID2D1SvgGlyphStyle is a real object (fill brush ref, stroke width/dashes/offset), and DrawSvgDocument joins the canvas service's loud drop ("dropped: no SVG rasterizer in this build", throttled 20/1000) — no pixels faked. set_context wires CreateSvgDocument/DrawSvgDocument/CreateSvgGlyphStyle; set_target (plain render targets) deliberately does not. (65) WALL — Windows.System.Threading.ThreadPool: launch-svg-1 died BEFORE any d2d1 trace: Win2D's AsyncOperation::StartThreadPoolDelegate (AsyncOperation.h:181) calls GetActivationFactory("Windows.System.Threading.ThreadPool", IThreadPoolStatics b6bf67dd-84bd-44f8-ac1c-93ebcb9dba91) the moment CanvasSvgDocumentStatics.LoadAsync starts, the catalog had no class → 80040154 → hresult_class_not_registered → abort → exit 3. Implemented as a real raven-xaml module (native/upstream/raven-xaml/thread_pool.c, in the factory chain beside dispatcher_queue, registry.cpp runtime_classes entry): the statics factory (IActivationFactory + IThreadPoolStatics, ActivateInstance E_ILLEGAL_METHOD_CALL) whose RunAsync/RunWithPriorityAsync/RunWithPriorityAndOptionsAsync each create the shared async-action shape (IAsyncAction + IAsyncInfo, completion raised after the handler returns) and run the IWorkItemHandler once on its own thread. FIRST BUG of the round found by launch-svg-2's crash: the work thread took a raw action pointer without its own reference — Win2D releases the returned IAsyncAction immediately, the action died under the running handler, and the heap corruption killed the awaiting thread minutes later (c0000005 read 0x2000000c0, no module → a freed heap block, under microsoft.graphics.canvas.dll+d9c5b) → TerminateProcess(0xFFFF7001). The work thread's own reference is the fix. (66) THE SVG WALL IS DOWN: launch-svg-2/3/4 all print `d2d1: ID2D1DeviceContext5::CreateSvgDocument: <svg>, 3 elements, viewport 1x1` — the app's material-pipeline SVG (Win2D CanvasSvgDocumentStatics.LoadAsync on the thread pool, the stream from (63)) parses. The app never walks the tree: ZERO ID2D1SvgElement member traces across all launches — its use is load → immediately SaveAsync → ID2D1SvgDocument::Serialize → 0x1b4 bytes through the raven-storage.dll HGLOBAL-backed IStream → Win2D reads the XML back into an HSTRING via WindowsPreallocateStringBuffer. (67) WALL — WindowsPreallocateStringBuffer/PromoteStringBuffer: implemented in platform_api.cpp beside the other HSTRING APIs (preallocate: allocate length+1 wide chars in their own block, an opaque handle answers, an empty preallocation hands a null buffer; promote: copy up to the terminator into the ordinary string header+characters layout and release the buffer block). launch-svg-3 stopped on the missing API; launch-svg-4 hung INSIDE the first handler — two bugs of the same commit: the length was read from argument 1 (the trace's four fields are length/charBuffer/bufferHandle/padding, so length is arg 0) and store_pointer wrote the handle over argument 0, plus the handle counter was a stack local captured by reference ([&] into a long-lived registry lambda — dangling). Fixed: shared_ptr state, correct indices. NOT verified by launch — the 4-launch budget is spent (launch-svg-1..4, logs in /tmp/raven-shapr-probe); the next run starts past the read-back and the next wall will name itself there. Build + ctest: component rebuilt through scripts/provision-controls.py --components d2d1 (0 warnings), raven-xaml.dll and the runtime rebuilt by ninja; 18 loader/pe/xaml/ucrt tests green (native_dwrite + native_edit_text_contract still the pre-existing RED). Still ZERO vkQueueSubmit/vkQueuePresentKHR — the app is still in one-time startup (material pipeline), the frame loop has never been reached. Nothing committed.

- The SVG round (2026-11-22). (64) THE PREVIOUS ROUND'S "FIX ALREADY IN TREE" CLAIM WAS FALSE — CreateSvgDocument was still the bare stub (slot 115). Added the whole model: eight stub tables in native/guest/raven-d2d1/d2d1_tables.h (ID2D1SvgDocument 15, ID2D1SvgElement 34, ID2D1SvgAttribute 6, ID2D1SvgPaint 13, ID2D1SvgStrokeDashArray 12, ID2D1SvgPointCollection 10, ID2D1SvgPathData 15, ID2D1SvgGlyphStyle 9 — member order/GUIDs from the system d2d1svg.h with D2D_USE_C_DEFINITIONS) + a ~1500-line section in d2d1.c: XML parser (entities/CDATA/comments/DOCTYPE, UTF-8/16 BOMs), element tree with document-order attributes, text/typed-POD/attribute-object value storage, tree mutation, real Serialize/Deserialize, FindElementById, GetViewportSize (8-byte struct → RAX), CreateSvgGlyphStyle as a real object, DrawSvgDocument joining the canvas service's loud no-rasterizer drop. Plus per the traces: native/upstream/raven-xaml/thread_pool.c (Windows.System.Threading.ThreadPool IThreadPoolStatics RunAsync×3, each work item its own thread, shared async-action shape), a registry.cpp class entry, factory-chain wiring, and WindowsPreallocateStringBuffer/WindowsPromoteStringBuffer in platform_api.cpp (writable guest chars + opaque handle; promote copies to the terminator). (65) SELF-CAUGHT BUGS, each by a launch: launch-2's pool refcount bug (Win2D releases the returned IAsyncAction immediately — the action died under the running handler, heap corruption killed the awaiting thread; fixed by handing the work thread its own action reference); launch-4's handler arg-index/dangling-capture bug (fixed in tree, launch-unverified). (66) VERIFIED (launches 2/3): THE SVG WALL IS DOWN — `d2d1: ID2D1DeviceContext5::CreateSvgDocument: <svg>, 3 elements, viewport 1x1`; full load → ID2D1SvgDocument::Serialize (0x1b4 bytes through raven-storage's HGLOBAL stream) → died on the missing Preallocate → (fix landed). THE APP NEVER WALKS THE TREE — zero ID2D1SvgElement member traces; its SVG use is LoadAsync → immediate SaveAsync read-back; the traces, not assumptions, set the scope. Still ZERO vkQueueSubmit/vkQueuePresentKHR (one-time startup; only vkEnumeratePhysicalDevices/vkCreateDevice). 18 loader/pe/xaml/ucrt ctests green (dwrite + edit_text remain the documented pre-existing RED). Deployed to build/native. (67) NEXT: verify the string-buffer path, then the next wall names itself (past the SaveAsync read-back). Logs: launch-svg-1..4. Nothing committed.

- The string-buffer verification round (2026-11-22). (68) THE PREALLOCATE/PROMOTE FIX IS CORRECT IN TREE AND LAUNCH-VERIFIED. In-tree judgment first: the registry signature is `WindowsPreallocateStringBuffer(3)` = (length, charBuffer*, bufferHandle*) per the real ABI — arg0 is the length (launch-svg-4's bug: it read arg1, a pointer, as the length), the characters go to arg1 and the opaque handle to arg2 (bug two: the handle was written over arg0), and the handle counter/map are `shared_ptr`s captured by value, not `[&]` over stack locals (bug three, the dangling capture); the promote copies to the terminator into the same header layout WindowsCreateStringReference builds (flags=0 so DeleteString releases it — right for a promoted heap string), releases the preallocation block, and an empty promote answers a null HSTRING. launch-str-1 was a 180s timeout, not a wall: thread 186 spent the budget inside DXVK's device-init banner — 27,694 traced WriteFile chunks of ~40 bytes to `drive_c/Shapr3D_d3d11.log` (1.18 MB of capability/extension listing; the dxgi.log sibling sits beside it; DXVK opens them OPEN_ALWAYS|GENERIC_WRITE next to the exe path, one flush per log line) plus .NET assembly probing — the API-trace overhead of the banner alone pushed startup past the budget. Quieted by `RAVEN_GUEST_DXVK_CONFIG='dxgi.enableDummyCompositionSwapchain=True;dxvk.logLevel=None'` (RAVEN_GUEST_<NAME> reaches the guest as <NAME>, system_api.cpp:156; DXVK_CONFIG takes semicolon-separated entries; DXVK honors dxvk.logLevel=None by not writing the banner) and a 600 s budget. The rest of launch-str-1 matched launch-svg-4's phase structure line for line (same vkCreatePipelineLayout/canvas-drop counts, same EnumDisplayMonitors and CoreApplication activations) — no divergence, just time. (69) launch-str-2 VERIFIED the string-buffer path: `WindowsPreallocateStringBuffer( 1b5 … ) → WindowsPromoteStringBuffer( 21 … ) → WindowsStringHasEmbeddedNull → WindowsDuplicateString → WindowsDeleteString`, all S_OK, three load→save cycles — launch-svg-4's inside-the-handler hang is gone. Then the wall named itself: the third `CreateSvgDocument` answered "the XML did not parse" → the app threw HResultException(E_INVALIDARG 0x80070057) → abort → ExitProcess(3). Diagnostics added to raven-d2d1 for that question (svg_parse grows start/fail_at; failures trace the byte offset and the chunked escaped input; Serialize traces its first four outputs) — rebuilt through provision-controls.py --output components/controls64 --components d2d1. (70) launch-str-3 NAMED THE WALL: the failing input is **BOM-less UTF-16LE** (`<\0?\0x\0m\0l\0…`) — Win2D writes the read-back string into the stream it hands LoadAsync without a byte order mark, and svg_read_stream only sniffed BOMs, so the UTF-16 bytes went through MultiByteToWideChar(CP_UTF8) and every second wide char came out NUL; the parser died inside the first tag (offset 27). Fixed in svg_read_stream: the XML encoding sniff — a leading pair with exactly one zero byte is BOM-less UTF-16, endianness from which byte is zero (a UTF-8 XML document can never begin with NUL, so the heuristic is safe), folded into the same code path as the marked LE/BE branches. The serialize dumps also proved raven's serialization byte-honest: `<?xml version="1.0" encoding="utf-8"?>\n<svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg"><path fill-rule="evenodd" clip-rule="evenodd" d="M16.2803 …"/></svg>` with the app's `&gt;&#13;&#10;` text child preserved (the '?' glyphs in the dump are the LFs). (71) launch-str-4: THE ROUND TRIP PARSES — `CreateSvgDocument: <svg>, 3 elements, viewport 1x1` for the app's own re-written stream; the SVG read-back wall is down. THE NEXT WALL NAMED ITSELF: `winrt: no class Windows.UI.ColorHelper` (`winrt: requested 8504dbea-fb6a-4144-a6c2-33499c9284f5` = IColorHelperStatics) → REGDB_E_CLASSNOTREG 80040154 → RoOriginateLanguageException → abort → exit 3 at ~180 s. Implemented per the metadata, which both the mingw windows.ui.h and the shipped windows.ui.winmd agree on: **IColorHelperStatics = FromArgb only; IColorHelperStatics2 (24d9af02-6eb0-4b94-855c-fcf0818d9a16) = ToDisplayName only — the .NET `ColorHelper.ToColor` static has no ABI interface in the metadata**, so nothing is guessed: the new raven-xaml module `color_helper.c` serves the factory (activation chain beside colors.c, registry.cpp runtime-class entry `Windows.UI.ColorHelper → raven-xaml.dll`) with FromArgb real and ToDisplayName answering the real palette names from colors.inc (the empty string for colors outside the palette); if the app's next step asks for a further interface (a ToColor-bearing one from a newer contract), the next launch's trace will name that GUID and the answer extends then. NOT launch-verified — the 4-launch budget is spent (launch-str-1..4, logs in /tmp/raven-shapr-probe). Build + ctest: 17 xaml/loader/pe/ucrt tests green (native_dwrite + native_edit_text_contract untouched, the documented pre-existing RED); deployed to build/native (raven with the registry entry, raven-xaml.dll with color_helper.c, raven-d2d1.dll with the sniff fix + diagnostics). REACH: three SVG documents parse (two app-authored, one raven-serialized round trip), serialize→promote→read-back real, the material pipeline loads CAD-kernel DLLs (usd_*, td_*, pskernel) on worker threads, d3d12 fences alive. Still ZERO vkQueueSubmit/vkQueuePresentKHR — one-time startup, the frame loop never reached. Nothing committed.

- The string-buffer and UTF-16 round (2026-11-22). (68) THE STRING-BUFFER FIX IS CORRECT IN TREE AND LAUNCH-VERIFIED (launch-str-2:761887): preallocate → promote → has-embedded-null → duplicate → delete, all S_OK, three load→save cycles; the implementation (platform_api.cpp:1385-1439) reads length from arg 0, stores chars/handle to args 1/2, holds the counter/map in by-value shared_ptrs, and the promote builds the WindowsCreateStringReference header layout with flags=0 (DeleteString releases the promoted heap string); empty promote → null HSTRING. (69) PROCESS: launch-str-1 was a 180s timeout, not a wall — DXVK's device-init banner wrote 27,694 traced WriteFile chunks (1.18 MB) to drive_c/Shapr3D_d3d11.log; quieted with `dxvk.logLevel=None` in RAVEN_GUEST_DXVK_CONFIG and a 600 s budget. (70) THE NEXT WALL: the SVG ROUND-TRIP re-parse failed E_INVALIDARG ("a tag that does not parse") — diagnostics added to raven-d2d1 named it: the failing input is **BOM-LESS UTF-16LE** (Win2D writes the read-back string without a BOM; svg_read_stream only sniffed BOMs) — fixed with the XML encoding sniff (leading pair with exactly one zero byte = UTF-16, endianness from which byte is zero); the Serialize dumps proved raven's serialization byte-honest. VERIFIED: the app re-parses raven's own serialized XML ("3 elements, viewport 1x1") — the full SaveAsync round trip is down. (71) **Windows.UI.ColorHelper** implemented metadata-honestly (mingw windows.ui.h + the shipped windows.ui.winmd agree): IColorHelperStatics 8504dbea-… = FromArgb only; IColorHelperStatics2 24d9af02-… = ToDisplayName only (real palette names from colors.inc); the .NET ToColor static has no ABI interface — nothing guessed. New module color_helper.c, wired into xaml.c's activation chain + registry.cpp. LAUNCH-UNVERIFIED (budget spent). 17 xaml/loader/pe/ucrt tests green; dwrite + edit_text remain the documented pre-existing RED. (72) REACH: SVG load → Serialize → string read-back → re-parse; the material pipeline loads CAD-kernel DLLs (usd_*, td_*, pskernel) on workers; d3d12 queue threads alive in vkWaitForFences; **vkQueueSubmit/vkQueuePresentKHR still ZERO** — one-time startup only. Logs: launch-str-1..4. Nothing committed.

- The ColorHelper verification and the graft round (2026-11-22). (73) DEPLOYMENT WAS STALE — the previous round's "deployed raven-xaml.dll" claim was false for the final builds: build/native/raven-xaml.dll (16:41, 1610699 bytes) and build/native/x86/raven-xaml.dll (16:42) both postdated the prefix copies (15:37). Redeployed (build/native → system32, x86 → syswow64), cmp-verified both; raven-d2d1.dll in build/native cmp-matches controls64. The launch then VERIFIED ColorHelper: `winrt: activation iid 8504dbea` → `winrt: activation Windows.UI.ColorHelper served` in EVERY launch, the app proceeding past it each time — member-level calls are not traced (the factory answers silently), so the activation serving is the launch evidence. (74) THE WALL launch-color-1 NAMED: `ID2D1SvgElement::AppendChild: an element of another document` → D2DERR_WRONG_RESOURCE_DOMAIN 0x88990015 → Win2D HResultException (Microsoft.Graphics.Canvas.Svg.h) → abort → ExitProcess(3) at ~181.5 s — the app's material pipeline grafts an SVG element between two documents raven parsed. The refusal was over-strict: the Windows resource domain is the DEVICE (factory), not the document — Win2D's own AppendChild (winrt/lib/svg/CanvasSvgElement.cpp) enforces only VerifyDeviceBoundary (device-level) and then calls D2D directly, so same-device cross-document grafts are legal; and the real ID2D1SvgDocument has 15 methods exactly as raven's table (Microsoft's docs list no CreateElement/CreateGlyphStyle; mingw's order FindElementById=8, Serialize=9, Deserialize=10 confirmed by the app calling Serialize at slot 9 successfully). FIXED in d2d1.c: svg_element_admit_child now refuses only a different factory (D2DERR_WRONG_RESOURCE_DOMAIN stays Windows-honest for a foreign domain) and ADOPTS a sibling-document element — svg_retag_subtree/svg_adopt_subtree retag the moved subtree to the destination document and adjust both documents' element tallies, with a trace naming the tag and moved count; an ancestor-cycle guard (E_INVALIDARG "an element into its own subtree") and SetRoot relaxed the same way; InsertChildBefore validates the reference child before mutating (admission must not happen for a call that will fail) and re-lookups the reference index after the admission may have detached the child from the same list. (75) launch-color-3 AND launch-color-4 VERIFIED THE FIX: `ID2D1SvgElement::AppendChild: 'path' adopted from another document (1)` — the app's graft works. THE NEXT WALL NAMED ITSELF (deterministic, both launches): right after the graft, during Win2D's device-probe QI storm (~22 refused interfaces: ID2D1Device1/DeviceContext1/Bitmap1/CommandList/StrokeStyle1/ImageBrush/Geometry, IDXGISwapChain1 …), the crash registers hold the serialized XML's own opening bytes ("?xml", "versio") and the AV lands in raven-rtl's heap bin_remove (rtl.dll+392b, `mov %rax,(%rcx)` with a zeroed free-links `previous`) on the material worker thread (182/186) at ~181-187 s — SOME write of the XML runs past its buffer into the rtl heap's free-list links; the thread dies, the app's remaining workers poll forever, the 600 s budget expires (exit 1). Audited on paper and sound: d2d1's serialize bytes buffer (doubling GROW, fits), raven-storage's block_write (resize before every memcpy, exact fit), the string-buffer preallocate/promote, Win2D's GetXml chain against raven's GlobalSize (requested size, zeroed regrowth). PRIME SUSPECT: raven-storage's shared_block has NO serialization — memory_stream_Clone shares one block and the material phase runs SEVERAL SVG workers concurrently (threads 182/186/190 all in the SVG phase; launch-color-2, pre-fix, crashed with the sibling signature: an app-side NULL read at shapr.native+94d2e8 of a freed-object field, no append reached) — a concurrent write/resize race on one HGlobal block would overflow exactly like this; second suspect: GlobalSize/heap.block_size slack semantics vs Win2D's GetXml reading GlobalSize bytes. NEXT ROUND: serialize the shared block (or per-stream positions) + relaunch; the heap write that overflows will be caught at the clone/lock boundary. (76) REACH: four SVG documents parse (three app-authored, one adopted-into), serialize → string read-back → re-parse all real, the first cross-document tree graft real, ColorHelper served. Build + ctest: d2d1 rebuilt through provision-controls.py (0 warnings), ninja clean, 20 loader/pe/ucrt/xaml tests green (dwrite + edit_text_contract remain the documented pre-existing RED). **vkQueueSubmit/vkQueuePresentKHR still ZERO across all four launches** — one-time startup only, the frame loop never reached. Logs: launch-color-1..4. Nothing committed.

- The ColorHelper and graft round (2026-11-22). (73) DEPLOYMENT WAS STALE — the previous round's "deployed raven-xaml.dll" claim was false for the final builds (build copies postdated the prefix copies); redeployed both widths cmp-verified. LAUNCH-VERIFIED: `winrt: activation Windows.UI.ColorHelper served` every launch (member calls invisible — raven-xaml factories answer silently; color_helper.c is static-only, metadata-honest). (74) THE CROSS-DOCUMENT APPENDCHILD WALL: `ID2D1SvgElement::AppendChild: an element of another document` → D2DERR_WRONG_RESOURCE_DOMAIN → Win2D HResultException → abort. THE WINDOWS SEMANTICS, verified against Win2D's own source (CanvasSvgElement.cpp AppendChild enforces only device boundary): the resource domain is the DEVICE (factory), not the document — foreign factory → D2DERR_WRONG_RESOURCE_DOMAIN kept (honest refusal); sibling-document element → ADOPTED (subtree retagged to the destination document, tallies adjusted, trace names tag+count); ancestor-cycle guard; SetRoot relaxed identically; InsertChildBefore validates the reference before mutating. Verified: `AppendChild: 'path' adopted from another document (1)` deterministically. 20 loader/pe/ucrt/xaml tests green; d2d1 deployed cmp-verified, raven-xaml pair cmp-verified. (75) NEW WALL — A DETERMINISTIC HEAP OVERFLOW: right after the graft, during Win2D's device-probe QI storm, the crash registers hold the serialized XML's own opening bytes and the AV lands in raven-rtl heap bin_remove (rtl+392b, zeroed free-links) on a material worker thread — an XML write overruns into the rtl heap free list; worker dies, app hangs to the 600s timeout. Paper audit points at raven-storage's shared_block having NO SERIALIZATION (memory_stream_Clone shares one block while material workers 182/186/190 run the SVG phase concurrently — a write/resize race overflows exactly like this); second suspect: GlobalSize/heap.block_size slack vs Win2D's GetXml reading GlobalSize bytes. Fix round in flight. (76) vkQueueSubmit/vkQueuePresentKHR still ZERO — one-time startup continues, but the app now walks and mutates the SVG tree it previously only round-tripped. Logs: launch-color-1..4. Nothing committed.

- SESSION END (2026-11-22, wrap-up). Tree left GREEN: full ninja completes, 36/36 tests pass in the storage/pe/loader/xaml families. THE RACE ROUND IS MID-FLIGHT (agent stopped at wrap-up, launch-race-{base,1-freeze,2,4}.log preserved): the audit landed (the shared-block write/resize race is real; the fix adds locking to raven-storage's shared block), the ORIGINAL OVERFLOW IS GONE in launch-race-4 (zero bin_remove/free():invalid hits — the locking works), but the lock placement triggers a NEW failure: `native callback exceeds initial nesting limit (last API RtlAcquireSRWLockExclusive)` — the SRW lock is acquired inside a native callback already at raven's nesting limit (threads.cpp's nesting guard). RESUME: move the lock acquisition out of the callback path (take it in the API-dispatch layer before entering guest code, or use a non-callback primitive — a host-side atomic flag/futex outside raven's callback nesting accounting), rebuild, relaunch; the expected sequence after the race fix: the material pipeline's SVG phase completes → UI construction completes → the frame loop starts → FIRST vkQueueSubmit/vkQueuePresentKHR (still zero in every launch) → pipewire capture (--renderer=pixman restart) for the first visual frame. Full session narrative: docs entries (1)–(76); ~25 walls cleared across codec ABI, XBF loader, imaging chain (real WIC decode), display metadata/EDID, ContentDialog faces + ShowAsync, Win2D load chain, SVG model, engine-init performance (120s→22.3s), and the material-pipeline fatal. All work uncommitted per the user's preference. Logs: /tmp/raven-shapr-probe/.
