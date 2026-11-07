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
