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
- Application limits: one view; no `RunWithActivationFactories`, process property
  set or suspend/resume event delivery (`E_NOTIMPL`). There is no package
  identity yet: `Id` forwards the host's no-package error. Direct view-source
  startup has empty `Load` entry point, launch arguments and tile ID, and no
  splash screen. Manifest-driven activation and additional activation-argument
  interfaces remain for stage 5.
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
  standalone DispatcherQueueController creation, shutdown draining and deferrals
  remain unsupported (`GetDeferral` returns `E_NOTIMPL`).
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
  sample; `GetIntermediatePoints` stays unimplemented — the desktop input
  has no history buffer, so the snapshot carries exactly one point.
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
  completes; a failing `ShutdownStarting` handler never stalls the shutdown,
  and `GetDeferral` stays `E_NOTIMPL` per the plan. Post-shutdown queues
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

`scripts/generate-winrt-header.py` reads a winmd and emits the C declarations an
interface needs: the IID from the `Guid` attribute, the vtable in metadata order
after the IInspectable methods, and one slot per method. The ABI slot count is
the declared parameter count plus one when the logical return type is not void
(property getters and factory-created objects return through an out parameter);
a repeated method name in one interface is suffixed in the C member name while
the order stays unchanged. It requires `dnfile`:

```sh
python3 -m pip install --target /tmp/winmd-lib dnfile
PYTHONPATH=/tmp/winmd-lib python3 scripts/generate-winrt-header.py \
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
  (`scripts/generate-winrt-header.py`, types `IXamlMetadataProvider`,
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
- Acceptance: a Direct3D texture composites in a XAML surface without readback.

### Stage 7 — Shapr3D

- Run the app against the implementation; fix gaps driven by its traces.
- Not a stage with an independent contract: it is the stress test for 0–6.

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
   Done: `scripts/generate-winrt-header.py`, the generated header,
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
   Next: apply the remaining node property forms against the element classes,
   and give the tree its layout and rendering path (composition stage).
4. Extend DispatcherQueue with standalone controllers and deferred shutdown when
   required by app traces.
5. Finish pointer history/static access, cursor objects and touch/pen delivery as
   native input sources permit.
6. Extend application startup with manifest identity/entry-point data and
   `RunWithActivationFactories` as the app's startup path requires.
