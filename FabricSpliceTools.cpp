
#include <xsi_application.h>
#include <xsi_desktop.h>
#include <xsi_context.h>
#include <xsi_status.h>

#include <xsi_command.h>
#include <xsi_factory.h>
#include <xsi_longarray.h>
#include <xsi_doublearray.h>
#include <xsi_math.h>
#include <xsi_customproperty.h>
#include <xsi_ppglayout.h>
#include <xsi_menu.h>
#include <xsi_selection.h>
#include <xsi_argument.h>
#include <xsi_comapihandler.h>
#include <xsi_project.h>
#include <xsi_toolcontext.h>
#include <xsi_camera.h>
#include <xsi_primitive.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>

#include <xsi_graphicsequencer.h>
#include <xsi_graphicsequencercontext.h>

// project includes
#include "FabricSplicePlugin.h"
#include "FabricSpliceRenderPass.h"
#include <FabricSplice.h>
#include <vector>
#include "FabricSpliceBaseInterface.h"

using namespace XSI;

// The following IDs are taken from ManipulationConstants.kl in the 'Manipulation' extension.
// The IDs cover all of the event types propagated by Qt.
// A given DCC will not support all event types, but the IDs must match between
// all DCC, (and standalone tools built using Qt, PyQt, PySide or toher frameworks).

typedef unsigned int EventType;

const EventType Event_None =  0;  //  Not an event.
const EventType Event_ActionAdded = 114;  //  A new action has been added (QActionEvent).
const EventType Event_ActionChanged = 113;  //  An action has been changed (QActionEvent).
const EventType Event_ActionRemoved = 115;  //  An action has been removed (QActionEvent).
const EventType Event_ActivationChange =  99; //  A widget's top-level window activation state has changed.
const EventType Event_ApplicationActivate = 121;  //  This enum has been deprecated. Use ApplicationStateChange instead.
const EventType Event_ApplicationDeactivate = 122;  //  This enum has been deprecated. Use ApplicationStateChange instead.
const EventType Event_ApplicationFontChange = 36; //  The default application font has changed.
const EventType Event_ApplicationLayoutDirectionChange =  37; //  The default application layout direction has changed.
const EventType Event_ApplicationPaletteChange =  38; //  The default application palette has changed.
const EventType Event_ApplicationStateChange =  214;  //  The state of the application has changed.
const EventType Event_ApplicationWindowIconChange = 35; //  The application's icon has changed.
const EventType Event_ChildAdded =  68; //  An object gets a child (QChildEvent).
const EventType Event_ChildPolished = 69; //  A widget child gets polished (QChildEvent).
const EventType Event_ChildRemoved =  71; //  An object loses a child (QChildEvent).
const EventType Event_Clipboard = 40; //  The clipboard contents have changed (QClipboardEvent).
const EventType Event_Close = 19; //  Widget was closed (QCloseEvent).
const EventType Event_CloseSoftwareInputPanel = 200;  //  A widget wants to close the software input panel (SIP).
const EventType Event_ContentsRectChange =  178;  //  The margins of the widget's content rect changed.
const EventType Event_ContextMenu = 82; //  Context popup menu (QContextMenuEvent).
const EventType Event_CursorChange =  183;  //  The widget's cursor has changed.
const EventType Event_DeferredDelete =  52; //  The object will be deleted after it has cleaned up (QDeferredDeleteEvent).
const EventType Event_DragEnter = 60; //  The cursor enters a widget during a drag and drop operation (QDragEnterEvent).
const EventType Event_DragLeave = 62; //  The cursor leaves a widget during a drag and drop operation (QDragLeaveEvent).
const EventType Event_DragMove =  61; //  A drag and drop operation is in progress (QDragMoveEvent).
const EventType Event_Drop =  63; //  A drag and drop operation is completed (QDropEvent).
const EventType Event_DynamicPropertyChange = 170;  //  A dynamic property was added, changed, or removed from the object.
const EventType Event_EnabledChange = 98; //  Widget's enabled state has changed.
const EventType Event_Enter = 10; //  Mouse enters widget's boundaries (QEnterEvent).
const EventType Event_EnterEditFocus =  150;  //  An editor widget gains focus for editing. QT_KEYPAD_NAVIGATION must be defined.
const EventType Event_EnterWhatsThisMode =  124;  //  Send to toplevel widgets when the application enters "What's This?" mode.
const EventType Event_Expose =  206;  //  Sent to a window when its on-screen contents are invalidated and need to be flushed from the backing store.
const EventType Event_FileOpen =  116;  //  File open request (QFileOpenEvent).
const EventType Event_FocusIn = 8;  //  Widget or Window gains keyboard focus (QFocusEvent).
const EventType Event_FocusOut =  9;  //  Widget or Window loses keyboard focus (QFocusEvent).
const EventType Event_FocusAboutToChange =  23; //  Widget or Window focus is about to change (QFocusEvent)
const EventType Event_FontChange =  97; //  Widget's font has changed.
const EventType Event_Gesture = 198;  //  A gesture was triggered (QGestureEvent).
const EventType Event_GestureOverride = 202;  //  A gesture override was triggered (QGestureEvent).
const EventType Event_GrabKeyboard =  188;  //  Item gains keyboard grab (QGraphicsItem only).
const EventType Event_GrabMouse = 186;  //  Item gains mouse grab (QGraphicsItem only).
const EventType Event_GraphicsSceneContextMenu =  159;  //  Context popup menu over a graphics scene (QGraphicsSceneContextMenuEvent).
const EventType Event_GraphicsSceneDragEnter =  164;  //  The cursor enters a graphics scene during a drag and drop operation (QGraphicsSceneDragDropEvent).
const EventType Event_GraphicsSceneDragLeave =  166;  //  The cursor leaves a graphics scene during a drag and drop operation (QGraphicsSceneDragDropEvent).
const EventType Event_GraphicsSceneDragMove = 165;  //  A drag and drop operation is in progress over a scene (QGraphicsSceneDragDropEvent).
const EventType Event_GraphicsSceneDrop = 167;  //  A drag and drop operation is completed over a scene (QGraphicsSceneDragDropEvent).
const EventType Event_GraphicsSceneHelp = 163;  //  The user requests help for a graphics scene (QHelpEvent).
const EventType Event_GraphicsSceneHoverEnter = 160;  //  The mouse cursor enters a hover item in a graphics scene (QGraphicsSceneHoverEvent).
const EventType Event_GraphicsSceneHoverLeave = 162;  //  The mouse cursor leaves a hover item in a graphics scene (QGraphicsSceneHoverEvent).
const EventType Event_GraphicsSceneHoverMove =  161;  //  The mouse cursor moves inside a hover item in a graphics scene (QGraphicsSceneHoverEvent).
const EventType Event_GraphicsSceneMouseDoubleClick = 158;  //  Mouse press again (double click) in a graphics scene (QGraphicsSceneMouseEvent).
const EventType Event_GraphicsSceneMouseMove =  155;  //  Move mouse in a graphics scene (QGraphicsSceneMouseEvent).
const EventType Event_GraphicsSceneMousePress = 156;  //  Mouse press in a graphics scene (QGraphicsSceneMouseEvent).
const EventType Event_GraphicsSceneMouseRelease = 157;  //  Mouse release in a graphics scene (QGraphicsSceneMouseEvent).
const EventType Event_GraphicsSceneMove = 182;  //  Widget was moved (QGraphicsSceneMoveEvent).
const EventType Event_GraphicsSceneResize = 181;  //  Widget was resized (QGraphicsSceneResizeEvent).
const EventType Event_GraphicsSceneWheel =  168;  //  Mouse wheel rolled in a graphics scene (QGraphicsSceneWheelEvent).
const EventType Event_Hide =  18; //  Widget was hidden (QHideEvent).
const EventType Event_HideToParent =  27; //  A child widget has been hidden.
const EventType Event_HoverEnter =  127;  //  The mouse cursor enters a hover widget (QHoverEvent).
const EventType Event_HoverLeave =  128;  //  The mouse cursor leaves a hover widget (QHoverEvent).
const EventType Event_HoverMove = 129;  //  The mouse cursor moves inside a hover widget (QHoverEvent).
const EventType Event_IconDrag =  96; //  The main icon of a window has been dragged away (QIconDragEvent).
const EventType Event_IconTextChange =  101;  //  Widget's icon text has been changed.
const EventType Event_InputMethod = 83; //  An input method is being used (QInputMethodEvent).
const EventType Event_InputMethodQuery =  207;  //  A input method query event (QInputMethodQueryEvent)
const EventType Event_KeyboardLayoutChange =  169;  //  The keyboard layout has changed.
const EventType Event_KeyPress =  6;  //  Key press (QKeyEvent).
const EventType Event_KeyRelease =  7;  //  Key release (QKeyEvent).
const EventType Event_LanguageChange =  89; //  The application translation changed.
const EventType Event_LayoutDirectionChange = 90; //  The direction of layouts changed.
const EventType Event_LayoutRequest = 76; //  Widget layout needs to be redone.
const EventType Event_Leave = 11; //  Mouse leaves widget's boundaries.
const EventType Event_LeaveEditFocus =  151;  //  An editor widget loses focus for editing. QT_KEYPAD_NAVIGATION must be defined.
const EventType Event_LeaveWhatsThisMode =  125;  //  Send to toplevel widgets when the application leaves "What's This?" mode.
const EventType Event_LocaleChange =  88; //  The system locale has changed.
const EventType Event_NonClientAreaMouseButtonDblClick =  176;  //  A mouse double click occurred outside the client area.
const EventType Event_NonClientAreaMouseButtonPress = 174;  //  A mouse button press occurred outside the client area.
const EventType Event_NonClientAreaMouseButtonRelease = 175;  //  A mouse button release occurred outside the client area.
const EventType Event_NonClientAreaMouseMove =  173;  //  A mouse move occurred outside the client area.
const EventType Event_MacSizeChange = 177;  //  The user changed his widget sizes (Mac OS X only).
const EventType Event_MetaCall =  43; //  An asynchronous method invocation via QMetaObject::invokeMethod().
const EventType Event_ModifiedChange =  102;  //  Widgets modification state has been changed.
const EventType Event_MouseButtonDblClick = 4;  //  Mouse press again (QMouseEvent).
const EventType Event_MouseButtonPress =  2;  //  Mouse press (QMouseEvent).
const EventType Event_MouseButtonRelease =  3;  //  Mouse release (QMouseEvent).
const EventType Event_MouseMove = 5;  //  Mouse move (QMouseEvent).
const EventType Event_MouseTrackingChange = 109;  //  The mouse tracking state has changed.
const EventType Event_Move =  13; //  Widget's position changed (QMoveEvent).
const EventType Event_OrientationChange = 208;  //  The screens orientation has changes (QScreenOrientationChangeEvent)
const EventType Event_Paint = 12; //  Screen update necessary (QPaintEvent).
const EventType Event_PaletteChange = 39; //  Palette of the widget changed.
const EventType Event_ParentAboutToChange = 131;  //  The widget parent is about to change.
const EventType Event_ParentChange =  21; //  The widget parent has changed.
const EventType Event_PlatformPanel = 212;  //  A platform specific panel has been requested.
const EventType Event_Polish =  75; //  The widget is polished.
const EventType Event_PolishRequest = 74; //  The widget should be polished.
const EventType Event_QueryWhatsThis =  123;  //  The widget should accept the event if it has "What's This?" help.
const EventType Event_RequestSoftwareInputPanel = 199;  //  A widget wants to open a software input panel (SIP).
const EventType Event_Resize =  14; //  Widget's size changed (QResizeEvent).
const EventType Event_ScrollPrepare = 204;  //  The object needs to fill in its geometry information (QScrollPrepareEvent).
const EventType Event_Scroll =  205;  //  The object needs to scroll to the supplied position (QScrollEvent).
const EventType Event_Shortcut =  117;  //  Key press in child for shortcut key handling (QShortcutEvent).
const EventType Event_ShortcutOverride =  51; //  Key press in child, for overriding shortcut key handling (QKeyEvent).
const EventType Event_Show =  17; //  Widget was shown on screen (QShowEvent).
const EventType Event_ShowToParent =  26; //  A child widget has been shown.
const EventType Event_SockAct = 50; //  Socket activated, used to implement QSocketNotifier.
const EventType Event_StateMachineSignal =  192;  //  A signal delivered to a state machine (QStateMachine::SignalEvent).
const EventType Event_StateMachineWrapped = 193;  //  The event is a wrapper for, i.e., contains, another event (QStateMachine::WrappedEvent).
const EventType Event_StatusTip = 112;  //  A status tip is requested (QStatusTipEvent).
const EventType Event_StyleChange = 100;  //  Widget's style has been changed.
const EventType Event_TabletMove =  87; //  Wacom tablet move (QTabletEvent).
const EventType Event_TabletPress = 92; //  Wacom tablet press (QTabletEvent).
const EventType Event_TabletRelease = 93; //  Wacom tablet release (QTabletEvent).
const EventType Event_OkRequest = 94; //  Ok button in decoration pressed. Supported only for Windows CE.
const EventType Event_TabletEnterProximity =  171;  //  Wacom tablet enter proximity event (QTabletEvent), sent to QApplication.
const EventType Event_TabletLeaveProximity =  172;  //  Wacom tablet leave proximity event (QTabletEvent), sent to QApplication.
const EventType Event_ThreadChange =  22; //  The object is moved to another thread. This is the last event sent to this object in the previous thread. See QObject::moveToThread().
const EventType Event_Timer = 1;  //  Regular timer events (QTimerEvent).
const EventType Event_ToolBarChange = 120;  //  The toolbar button is toggled on Mac OS X.
const EventType Event_ToolTip = 110;  //  A tooltip was requested (QHelpEvent).
const EventType Event_ToolTipChange = 184;  //  The widget's tooltip has changed.
const EventType Event_TouchBegin =  194;  //  Beginning of a sequence of touch-screen or track-pad events (QTouchEvent).
const EventType Event_TouchCancel = 209;  //  Cancellation of touch-event sequence (QTouchEvent).
const EventType Event_TouchEnd =  196;  //  End of touch-event sequence (QTouchEvent).
const EventType Event_TouchUpdate = 195;  //  Touch-screen event (QTouchEvent).
const EventType Event_UngrabKeyboard =  189;  //  Item loses keyboard grab (QGraphicsItem only).
const EventType Event_UngrabMouse = 187;  //  Item loses mouse grab (QGraphicsItem only).
const EventType Event_UpdateLater = 78; //  The widget should be queued to be repainted at a later time.
const EventType Event_UpdateRequest = 77; //  The widget should be repainted.
const EventType Event_WhatsThis = 111;  //  The widget should reveal "What's This?" help (QHelpEvent).
const EventType Event_WhatsThisClicked =  118;  //  A link in a widget's "What's This?" help was clicked.
const EventType Event_Wheel = 31; //  Mouse wheel rolled (QWheelEvent).
const EventType Event_WinEventAct = 132;  //  A Windows-specific activation event has occurred.
const EventType Event_WindowActivate =  24; //  Window was activated.
const EventType Event_WindowBlocked = 103;  //  The window is blocked by a modal dialog.
const EventType Event_WindowDeactivate =  25; //  Window was deactivated.
const EventType Event_WindowIconChange =  34; //  The window's icon has changed.
const EventType Event_WindowStateChange = 105;  //  The window's state (minimized, maximized or full-screen) has changed (QWindowStateChangeEvent).
const EventType Event_WindowTitleChange = 33; //  The window title has changed.
const EventType Event_WindowUnblocked = 104;  //  The window is unblocked after a modal dialog exited.
const EventType Event_WinIdChange = 203;  //  The window system identifer for this native widget has changed.
const EventType Event_ZOrderChange =  126;  //  The widget's z-order has changed. This event is never sent to top level windows.


typedef unsigned int MouseButton;
const MouseButton MouseButton_NoButton = 0;
const MouseButton MouseButton_LeftButton = 1;
const MouseButton MouseButton_RightButton = 2;
const MouseButton MouseButton_MiddleButton = 4;

typedef unsigned int ModiferKey;
const ModiferKey ModiferKey_NoModifier = 0x00000000; // No modifier key is pressed.
const ModiferKey ModiferKey_Shift = 0x02000000; // A Shift key on the keyboard is pressed.
const ModiferKey ModiferKey_Ctrl = 0x04000000; // A Ctrl key on the keyboard is pressed.
const ModiferKey ModiferKey_Alt = 0x08000000; // An Alt key on the keyboard is pressed.
const ModiferKey ModiferKey_Meta = 0x10000000; // A Meta key on the keyboard is pressed.
const ModiferKey ModiferKey_Keypad = 0x20000000; // A keypad button is pressed.
const ModiferKey ModiferKey_GroupSwitch = 0x40000000; // X11 only. A Mode_switch key on the keyboard is pressed.



class UndoRedoData {

public:
  FabricCore::RTVal m_rtval_commands;

  ~UndoRedoData();

  static FabricCore::RTVal s_rtval_commands;
};

FabricCore::RTVal UndoRedoData::s_rtval_commands;


UndoRedoData::~UndoRedoData(){
  if(m_rtval_commands.isValid())
    m_rtval_commands.invalidate();
}


SICALLBACK fabricSpliceManipulation_Init(CRef & in_ctxt)
{
  Context ctxt( in_ctxt );
  Command oCmd;
  oCmd = ctxt.GetSource();
  oCmd.PutDescription(L"Invoked by Splice during manipulation.");
  oCmd.SetFlag(siNoLogging, false);
  oCmd.EnableReturnValue( false ) ;
  return CStatus::OK;
}

SICALLBACK fabricSpliceManipulation_Execute(CRef & in_ctxt)
{
  Context ctxt( in_ctxt );
  UndoRedoData *undoRedoData = new UndoRedoData();
  undoRedoData->m_rtval_commands = UndoRedoData::s_rtval_commands;
  ctxt.PutAttribute(L"UndoRedoData", (CValue::siPtrType)undoRedoData);
  return CStatus::OK;
}

SICALLBACK fabricSpliceManipulation_Undo(CRef & in_ctxt)
{
  Context ctxt( in_ctxt );
  UndoRedoData* undoRedoData = (UndoRedoData*)(CValue::siPtrType)ctxt.GetAttribute(L"UndoRedoData");
  if(undoRedoData && undoRedoData->m_rtval_commands.isValid()){
    for(int i=0; i<undoRedoData->m_rtval_commands.getArraySize(); i++){
      undoRedoData->m_rtval_commands.getArrayElement(i).callMethod("", "undoAction", 0, 0);
    }
  }
  return CStatus::OK;
}

SICALLBACK fabricSpliceManipulation_Redo(CRef & in_ctxt)
{
  Context ctxt( in_ctxt );
  UndoRedoData* undoRedoData = (UndoRedoData*)(CValue::siPtrType)ctxt.GetAttribute(L"UndoRedoData");
  if(undoRedoData && undoRedoData->m_rtval_commands.isValid()){
    for(int i=0; i<undoRedoData->m_rtval_commands.getArraySize(); i++){
      undoRedoData->m_rtval_commands.getArrayElement(i).callMethod("", "doAction", 0, 0);
    }
  }
  return CStatus::OK;
}

SICALLBACK fabricSpliceManipulation_TermUndoRedo( CRef& in_ctxt )
{
  Context ctxt( in_ctxt );

  UndoRedoData* undoRedoData = (UndoRedoData*)(CValue::siPtrType)ctxt.GetAttribute(L"UndoRedoData");
  if(undoRedoData)
    delete undoRedoData;

  return CStatus::OK;
}

class fabricSpliceTool {

private:

  FabricCore::RTVal mEventDispatcher;

public:
  fabricSpliceTool(){
  }

  ~fabricSpliceTool(){
  }

  CStatus Activate( ToolContext& in_ctxt )
  {
    siToolCursor cursor = siPenCursor;
    in_ctxt.SetCursor( cursor );

    try{
        FabricCore::RTVal eventDispatcherHandle = FabricSplice::constructObjectRTVal("EventDispatcherHandle");
        mEventDispatcher = eventDispatcherHandle.callMethod("EventDispatcher", "getEventDispatcher", 0, 0);

      if(mEventDispatcher.isValid()){
        mEventDispatcher.callMethod("", "activateManipulation", 0, 0);
        in_ctxt.Redraw(true);
      }
    }
    catch(FabricCore::Exception e)    {
      xsiLogErrorFunc(e.getDesc_cstr());
      return CStatus::OK;
    }
    catch(FabricSplice::Exception e){
      xsiLogErrorFunc(e.what());
      return CStatus::OK;
    }

    return CStatus::OK;
  }

  CStatus Deactivate( ToolContext& in_ctxt )
  {
    if(mEventDispatcher.isValid()){
      // By deactivating the manipulation, we enable the manipulators to perform
      // cleanup, such as hiding paint brushes/gizmos. 
      mEventDispatcher.callMethod("", "deactivateManipulation", 0, 0);
      in_ctxt.Redraw(true);

      mEventDispatcher.invalidate();
    }

    return CStatus::OK;
  }

  void ConfigureMouseEvent(ToolContext& in_ctxt, FabricCore::RTVal klevent)
  {
    LONG cursorX, cursorY;
    in_ctxt.GetMousePosition( cursorX, cursorY );

    LONG width, height;
    in_ctxt.GetViewSize(width, height);

    FabricCore::RTVal klpos = FabricSplice::constructRTVal("Vec2");
    klpos.setMember("x", FabricSplice::constructFloat32RTVal((float)cursorX));
    klpos.setMember("y", FabricSplice::constructFloat32RTVal(height-((float)cursorY)));
    klevent.setMember("pos", klpos);

    long buttons = 0;
    if(in_ctxt.IsLeftButtonDown())
      buttons += MouseButton_LeftButton;
    if(in_ctxt.IsRightButtonDown())
      buttons += MouseButton_RightButton;
    if(in_ctxt.IsMiddleButtonDown())
      buttons += MouseButton_MiddleButton;

    // Note: In Softimage, we don't get independent events from Mousebutton down.
    klevent.setMember("button", FabricSplice::constructUInt32RTVal(buttons));
    klevent.setMember("buttons", FabricSplice::constructUInt32RTVal(buttons));
  }


  bool InvokeKLEvent(ToolContext& in_ctxt, FabricCore::RTVal klevent, int eventType){

    try
    {
      //////////////////////////
      // Setup the viewport
      FabricCore::RTVal inlineViewport = FabricSplice::constructObjectRTVal("InlineViewport");
      {

        LONG width, height;
        in_ctxt.GetViewSize(width, height);

        // Note: There is a task open to unify the viewports between rendering and manipulation
        // This will mean that resize can occur against the propper draw context. 
        // Here I create a temporary DrawContext, but that should be eliminated.
        FabricCore::RTVal drawContext = FabricSplice::constructObjectRTVal("DrawContext");
        drawContext.setMember("viewport", inlineViewport);
        std::vector<FabricCore::RTVal> args(3);
        args[0] = drawContext;
        args[1] = FabricSplice::constructFloat64RTVal(width);
        args[2] = FabricSplice::constructFloat64RTVal(height);
        inlineViewport.callMethod("", "resize", 3, &args[0]);

        {
          FabricCore::RTVal inlineCamera = FabricSplice::constructObjectRTVal("InlineCamera");

          Camera camera = in_ctxt.GetCamera();
          Primitive cameraPrim = camera.GetActivePrimitive();
          LONG projType = cameraPrim.GetParameterValue(L"proj");
          FabricCore::RTVal param = FabricSplice::constructBooleanRTVal(projType == 0);
          inlineCamera.callMethod("", "setOrthographic", 1, &param);

          double xsiViewportAspect = double(width) / double(height);
          double cameraAspect = cameraPrim.GetParameterValue(L"aspect");

          double fov = MATH::DegreesToRadians(cameraPrim.GetParameterValue(L"fov"));
          LONG fovType = cameraPrim.GetParameterValue(L"fovType");
          double fovY;
          if(fovType != 0){ // Perspective projection.
            double fovX = fov;
            if(xsiViewportAspect < cameraAspect){
              // bars top and bottom
              fovY = atan( tan(fovX * 0.5) / xsiViewportAspect ) * 2.0;
            }
            else{
              // bars left and right
              fovY = atan( tan(fovX * 0.5) / cameraAspect ) * 2.0;
            }
          }
          else{ // orthographic projection
            fovY = fov;
            if(xsiViewportAspect < cameraAspect){
              // bars top and bottom
              double fovX = atan( tan(fovY * 0.5) * cameraAspect ) * 2.0;
              fovY = atan( tan(fovX * 0.5) / xsiViewportAspect ) * 2.0;
            }
          }
          param = FabricSplice::constructFloat64RTVal(fovY);
          inlineCamera.callMethod("", "setFovY", 1, &param);


          double near = cameraPrim.GetParameterValue(L"near");
          double far = cameraPrim.GetParameterValue(L"far");
          param = FabricSplice::constructFloat64RTVal(near);
          inlineCamera.callMethod("", "setNearDistance", 1, &param);
          param = FabricSplice::constructFloat64RTVal(far);
          inlineCamera.callMethod("", "setFarDistance", 1, &param);

          FabricCore::RTVal cameraMat = FabricSplice::constructRTVal("Mat44");
          FabricCore::RTVal cameraMatData = cameraMat.callMethod("Data", "data", 0, 0);

          MATH::CTransformation xsiCameraXfo = camera.GetKinematics().GetGlobal().GetTransform();
          MATH::CMatrix4 xsiCameraMat = xsiCameraXfo.GetMatrix4();
          const double * xsiCameraMatDoubles = (const double*)&xsiCameraMat;

          float * cameraMatFloats = (float*)cameraMatData.getData();
          if(cameraMatFloats != NULL) {
            cameraMatFloats[0] = (float)xsiCameraMatDoubles[0];
            cameraMatFloats[1] = (float)xsiCameraMatDoubles[4];
            cameraMatFloats[2] = (float)xsiCameraMatDoubles[8];
            cameraMatFloats[3] = (float)xsiCameraMatDoubles[12];
            cameraMatFloats[4] = (float)xsiCameraMatDoubles[1];
            cameraMatFloats[5] = (float)xsiCameraMatDoubles[5];
            cameraMatFloats[6] = (float)xsiCameraMatDoubles[9];
            cameraMatFloats[7] = (float)xsiCameraMatDoubles[13];
            cameraMatFloats[8] = (float)xsiCameraMatDoubles[2];
            cameraMatFloats[9] = (float)xsiCameraMatDoubles[6];
            cameraMatFloats[10] = (float)xsiCameraMatDoubles[10];
            cameraMatFloats[11] = (float)xsiCameraMatDoubles[14];
            cameraMatFloats[12] = (float)xsiCameraMatDoubles[3];
            cameraMatFloats[13] = (float)xsiCameraMatDoubles[7];
            cameraMatFloats[14] = (float)xsiCameraMatDoubles[11];
            cameraMatFloats[15] = (float)xsiCameraMatDoubles[15];

            inlineCamera.callMethod("", "setFromMat44", 1, &cameraMat);
          }

          inlineViewport.callMethod("", "setCamera", 1, &inlineCamera);
        }
      }

      //////////////////////////
      // Setup the Host
      // We cannot set an interface value via RTVals.
      FabricCore::RTVal host = FabricSplice::constructObjectRTVal("Host");
      host.setMember("hostName", FabricSplice::constructStringRTVal("Softimage"));

      long modifiers = 0;
      if(in_ctxt.IsShiftKeyDown())
        modifiers += ModiferKey_Shift;
      if(in_ctxt.IsControlKeyDown())
        modifiers += ModiferKey_Ctrl;
      klevent.setMember("modifiers", FabricSplice::constructUInt32RTVal(modifiers));

      //////////////////////////
      // Configure the event...
      std::vector<FabricCore::RTVal> args(4);
      args[0] = host;
      args[1] = inlineViewport;
      args[2] = FabricSplice::constructUInt32RTVal(eventType);
      args[3] = FabricSplice::constructUInt32RTVal(modifiers);
      klevent.callMethod("", "init", 4, &args[0]);

      //////////////////////////
      // Invoke the event...
      mEventDispatcher.callMethod("Boolean", "onEvent", 1, &klevent);

      bool result = klevent.callMethod("Boolean", "isAccepted", 0, 0).getBoolean();

      // The manipulation system has requested that a custom command be invoked. 
      // Invoke the custom command passing the speficied args. 
      CString customCommand(host.maybeGetMember("customCommand").getStringCString());
      if(!customCommand.IsEmpty()){
        FabricCore::RTVal customCommandArgs = host.maybeGetMember("customCommandArgs");
        CValue returnVal;
        CValueArray args(customCommandArgs.getArraySize());
        for(int i=0; i<customCommandArgs.getArraySize(); i++){
          args[i] = CString(customCommandArgs.getArrayElement(i).getStringCString());
        }
        Application().ExecuteCommand(customCommand, args, returnVal);
      }

      if(host.maybeGetMember("redrawRequested").getBoolean())
        in_ctxt.Redraw(true);

      if(host.callMethod("Boolean", "undoRedoCommandsAdded", 0, 0).getBoolean()){
        // Cache the rtvals in a static variable that the command will then stor in the undo stack.
        UndoRedoData::s_rtval_commands = host.callMethod("UndoRedoCommand[]", "getUndoRedoCommands", 0, 0);

        CValue returnVal;
        CValueArray args(0);
        Application().ExecuteCommand(L"fabricSpliceManipulation", args, returnVal);
      }

      klevent.invalidate();
      return result;
    }
    catch(FabricCore::Exception e)    {
      xsiLogErrorFunc(e.getDesc_cstr());
      return false;
    }
    catch(FabricSplice::Exception e){
      xsiLogErrorFunc(e.what());
      return false;
    }
  }


  CStatus MouseMove( ToolContext& in_ctxt )
  {
    if(!mEventDispatcher.isValid())
      return false;

    FabricCore::RTVal klevent = FabricSplice::constructObjectRTVal("MouseEvent");

    ConfigureMouseEvent(in_ctxt, klevent);

    InvokeKLEvent(in_ctxt, klevent, Event_MouseMove);

    return CStatus::OK;
  }

  CStatus MouseDrag( ToolContext& in_ctxt )
  {
    if(!mEventDispatcher.isValid())
      return false;
    
    FabricCore::RTVal klevent = FabricSplice::constructObjectRTVal("MouseEvent");

    ConfigureMouseEvent(in_ctxt, klevent);

    InvokeKLEvent(in_ctxt, klevent, Event_MouseMove);

    return CStatus::OK;
  }

  CStatus MouseDown( ToolContext& in_ctxt )
  {
    if(!mEventDispatcher.isValid())
      return false;
    
    FabricCore::RTVal klevent = FabricSplice::constructObjectRTVal("MouseEvent");

    ConfigureMouseEvent(in_ctxt, klevent);

    InvokeKLEvent(in_ctxt, klevent, Event_MouseButtonPress);

    return CStatus::OK;
  }

  CStatus MouseUp( ToolContext& in_ctxt )
  {
    if(!mEventDispatcher.isValid())
      return false;
    
    FabricCore::RTVal klevent = FabricSplice::constructObjectRTVal("MouseEvent");

    ConfigureMouseEvent(in_ctxt, klevent);

    InvokeKLEvent(in_ctxt, klevent, Event_MouseButtonRelease);

    return CStatus::OK;
  }

  CStatus MouseEnter( ToolContext &in_ctxt )
  {
    if(!mEventDispatcher.isValid())
      return false;
    
    FabricCore::RTVal klevent = FabricSplice::constructObjectRTVal("MouseEvent");

    InvokeKLEvent(in_ctxt, klevent, Event_Enter);

    return CStatus::OK;
  }

  CStatus MouseLeave( ToolContext &in_ctxt )
  {
    if(!mEventDispatcher.isValid())
      return false;

    FabricCore::RTVal klevent = FabricSplice::constructObjectRTVal("MouseEvent");

    InvokeKLEvent(in_ctxt, klevent, Event_Leave);

    return CStatus::OK;
  }

  CStatus KeyDown( ToolContext &in_ctxt )
  {
    if(!mEventDispatcher.isValid())
      return false;

    FabricCore::RTVal klevent = FabricSplice::constructObjectRTVal("KeyEvent");

    int key = in_ctxt.GetShortcutKey();
    klevent.setMember("key", FabricSplice::constructUInt32RTVal(key));

    long count = 1;
    klevent.setMember("count", FabricSplice::constructUInt32RTVal(count));

    InvokeKLEvent(in_ctxt, klevent, Event_KeyPress);

    return CStatus::OK;
  }

  CStatus KeyUp( ToolContext &in_ctxt )
  {
    if(!mEventDispatcher.isValid())
      return false;

    FabricCore::RTVal klevent = FabricSplice::constructObjectRTVal("KeyEvent");

    int key = in_ctxt.GetShortcutKey();
    klevent.setMember("key", FabricSplice::constructUInt32RTVal(key));

    long count = 1;
    klevent.setMember("count", FabricSplice::constructUInt32RTVal(count));

    InvokeKLEvent(in_ctxt, klevent, Event_KeyRelease);

    return CStatus::OK;
  }

  CStatus ModifierChanged( ToolContext &in_ctxt )
  {
    if(!mEventDispatcher.isValid())
      return false;
    
    FabricCore::RTVal klevent = FabricSplice::constructObjectRTVal("MouseEvent");

    ConfigureMouseEvent(in_ctxt, klevent);

    InvokeKLEvent(in_ctxt, klevent, Event_ModifiedChange);

    return CStatus::OK;
  }

  CStatus MenuInit( ToolContext& in_ctxt )
  {
    MenuItem menuItem;
    Menu menu = in_ctxt.GetSource();

    // todo: add context menu items!
    return CStatus::OK;
  }

};

////////////////////////////////////////////////////////////////////////////////
//
// Plugin callbacks and tool callback registration.
//
////////////////////////////////////////////////////////////////////////////////
SICALLBACK fabricSpliceTool_Init( CRef& in_ctxt )
{
  ToolContext l_ctxt( in_ctxt );
  fabricSpliceTool *l_pTool = new fabricSpliceTool();
  if ( l_pTool ) {
    l_ctxt.PutUserData( CValue((CValue::siPtrType)l_pTool) );
  }
  return CStatus::OK;
}

SICALLBACK fabricSpliceTool_Term( CRef& in_ctxt )
{

  ToolContext l_ctxt( in_ctxt );
  fabricSpliceTool *l_pTool = (fabricSpliceTool *)(CValue::siPtrType)l_ctxt.GetUserData();
  if ( l_pTool ) {
    delete l_pTool;
    l_ctxt.PutUserData( CValue((CValue::siPtrType)NULL) ); // Clear user data.
  }
  return CStatus::OK;
}

#define DECLARE_TOOL_CALLBACK(TOOL,CALLBACK) \
SICALLBACK TOOL##_##CALLBACK( ToolContext& in_ctxt ) { \
  TOOL *l_pTool = (TOOL *)(CValue::siPtrType)in_ctxt.GetUserData(); \
  return ( l_pTool ? l_pTool->CALLBACK( in_ctxt ) : CStatus::Fail ); \
}

DECLARE_TOOL_CALLBACK( fabricSpliceTool, Activate );
DECLARE_TOOL_CALLBACK( fabricSpliceTool, Deactivate );
DECLARE_TOOL_CALLBACK( fabricSpliceTool, MouseMove );
DECLARE_TOOL_CALLBACK( fabricSpliceTool, MouseDrag );
DECLARE_TOOL_CALLBACK( fabricSpliceTool, MouseDown );
DECLARE_TOOL_CALLBACK( fabricSpliceTool, MouseUp );
DECLARE_TOOL_CALLBACK( fabricSpliceTool, MouseEnter );
DECLARE_TOOL_CALLBACK( fabricSpliceTool, MouseLeave );
DECLARE_TOOL_CALLBACK( fabricSpliceTool, KeyDown );
DECLARE_TOOL_CALLBACK( fabricSpliceTool, KeyUp );
DECLARE_TOOL_CALLBACK( fabricSpliceTool, ModifierChanged );
DECLARE_TOOL_CALLBACK( fabricSpliceTool, MenuInit );
