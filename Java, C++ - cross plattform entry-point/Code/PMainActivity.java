package P_App._Android;

import android.app.Activity;
import android.app.AlertDialog;
import android.os.Bundle;
import android.os.Handler;
import android.widget.TextView;
import android.view.View;
import android.view.InputDevice;
import android.view.Choreographer;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.MotionEvent;
import android.view.KeyEvent;
import android.util.DisplayMetrics;
import android.content.res.AssetManager;
import android.content.Context;
import android.hardware.input.InputManager;
import android.content.pm.PackageItemInfo;
import android.content.pm.PackageManager;
import android.content.pm.ApplicationInfo;

import java.text.AttributedString;
import java.util.ArrayList;
import java.util.List;
import javax.naming.directory.ModificationItem;
import java.io.File;

//TODO: How do i handle creation/deletion? I need some gamestate attribute (that i'll also be able to use as a savestate perhaps)
//TODO: Only one keyboard is supported at a time.


public class PMainActivity extends Activity implements SurfaceHolder.Callback{

//region CORE
    static{ 
        //https://www.geeksforgeeks.org/java/thread-uncaughtexceptionhandler-in-java-with-examples/

        //Set exception logging
        Thread.setDefaultUncaughtExceptionHandler( new Thread.UncaughtExceptionHandler(){
            @Override
            public void uncaughtException(Thread t, Throwable e) {
                PLog.Default(e.getMessage());
                System.exit(-1);
            }
        });

        //Load c++
        System.loadLibrary("main");
    }
    
    //Vars
    private static boolean appIsAlive = true;

    private Handler mainHandler = new Handler();
    private InputManager input = null;
    private SurfaceView surfaceView = null;

    //Utils
    private void OpenFatalDialogue(String message){
        new AlertDialog.Builder(this)
        .setTitle("Oops! :/")
        .setMessage(message)
        .setPositiveButton("Exit", (a, b) -> { finishAffinity(); } )
        .setCancelable(false)
        .show();
    }


    //https://developer.android.com/guide/components/activities/activity-lifecycle
    //https://developer.android.com/reference/android/view/SurfaceHolder.Callback
    //Lifecycle
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        //Meta-Info/Configuration
        //https://developer.android.com/reference/android/content/pm/PackageManager#getApplicationInfo(java.lang.String,%20int)
        //https://stackoverflow.com/questions/6589797/how-to-get-package-name-from-anywhere
        //https://developer.android.com/reference/android/content/pm/PackageManager.ApplicationInfoFlags
        //https://developer.android.com/reference/android/content/pm/ApplicationInfo
        //https://developer.android.com/guide/topics/resources/runtime-changes

        //Input:
        //https://developer.android.com/reference/android/content/Context#INPUT_SERVICE

        //Layout
        //https://developer.android.com/reference/android/view/View#setSystemUiVisibility(int)
        //https://developer.android.com/reference/android/view/View#SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        //https://stackoverflow.com/questions/62643517/immersive-fullscreen-on-android-11

        //Surface:
        //https://developer.android.com/reference/android/view/SurfaceView
        //https://developer.android.com/reference/android/view/SurfaceHolder#addCallback(android.view.SurfaceHolder.Callback)


        super.onCreate(savedInstanceState);

        //Enable Log
        try{
            String packageName = getPackageName();
            boolean isRelease = getPackageManager().getApplicationInfo(packageName, PackageManager.GET_META_DATA).metaData.getBoolean("isRelease", false);
            if (isRelease){ PLog.Disable(); }
            else PLog.Enable();
        
        } catch (Exception e){
            PLog.Default("Error: " + e.getMessage());
        }
        

        //Init base 
        PLog.Default("Core init!");
        surfaceView = new SurfaceView(this);
        surfaceView.getHolder().addCallback(this);
        setContentView(surfaceView);

        input = (InputManager)getSystemService(android.content.Context.INPUT_SERVICE);

        //Hide sys UI
        //TODO: Depracated from API 30
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
        );

        //Init Modules
        try{

            if(RWDir.Check()) RWDir.Set(this);
            if(Assets.Check()) Assets.Init( this ); 
            if(GameApp.Check()) GameApp.Open( this );
            
            if(ControllerHandler.Check()){
                controllerModule = new ControllerHandler(InputDevice.getDeviceIds());
                input.registerInputDeviceListener(controllerModule, mainHandler);
            }
            
            if(KeyboardHandler.Check()){ keyboardModule = new KeyboardHandler(); }
        } catch (Exception e){
            OpenFatalDialogue(e.getMessage());
            appIsAlive = false;
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder){
        PLog.Default("Surface created!");

        try{
            if(ScreenInfo.Check()) ScreenInfo.UpdateScreenInfo(this);
            if(GameApp.Check()) GameApp.StartGameLoop(holder);

        } catch (Exception e){
            OpenFatalDialogue(e.getMessage());
            appIsAlive = false;
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height){
        PLog.Default("Surface changed!");

        if(!appIsAlive){
            PLog.Default("While app was not alive-");
            return;
        }

        try{
            if(ScreenInfo.Check()) ScreenInfo.UpdateScreenInfo(this);      
            if(GameApp.Check()) GameApp.Resize(width, height);

        } catch (Exception e){
            OpenFatalDialogue(e.getMessage());
            appIsAlive = false;
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder){  
        PLog.Default("Surface destroyed!");

        try{
            if(GameApp.Check()) GameApp.EndGameLoop();

        } catch (Exception e){
            OpenFatalDialogue(e.getMessage());
            appIsAlive = false;
        }
    }

    @Override
    protected void onDestroy(){      
        PLog.Default("App destroyed!");

        //Cleanup instances
        if(keyboardModule != null){
            keyboardModule = null;
        }

        if(controllerModule != null) {
            input.unregisterInputDeviceListener(controllerModule);
            controllerModule.Close();
            controllerModule = null;
        }

        //Close game
        try{
            if(GameApp.Check()) GameApp.Close();
        } catch (Exception e){
            OpenFatalDialogue(e.getMessage());
            appIsAlive = false;
        }
   
        super.onDestroy();
    }


    //Input
    @Override
    public boolean onTouchEvent(MotionEvent event){
        if(!appIsAlive) return super.onTouchEvent(event);

        try{
            if(TouchInput.Check()) TouchInput.ProcessTouch(event, (float)surfaceView.getWidth(), (float)surfaceView.getHeight());

        } catch (Exception e){
            OpenFatalDialogue(e.getMessage());
            appIsAlive = false;
        }

        return super.onTouchEvent(event);
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event){
        if(!appIsAlive) return super.dispatchGenericMotionEvent(event);

        try{    
            if(controllerModule != null) controllerModule.ProcessMotionEvent(event);
        
        } catch (Exception e){
            OpenFatalDialogue(e.getMessage());
            appIsAlive = false;
        }

        return super.dispatchGenericMotionEvent(event);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if(!appIsAlive) return super.dispatchKeyEvent(event);

        try{          
            if(controllerModule != null) controllerModule.ProcessKeyEvent(event);
            if(keyboardModule != null) keyboardModule.ProcessKeyEvent(event);
        } catch (Exception e){
            OpenFatalDialogue(e.getMessage());
            appIsAlive = false;
        }

        return super.dispatchKeyEvent(event);
    }

    @Override   //This is depracated btw: https://developer.android.com/reference/androidx/activity/ComponentActivity#onBackPressed()
    public void onBackPressed(){
        PLog.Default("Back was pressed!");
        if(controllerModule == null) super.onBackPressed();
    }
//endregion


//region MODULES
    //GameApp
    public static native void GameAppDummy();
    public static native void InitGameApp();
    public static native void InitAppGraphics(Object surface);
    public static native void RunGameApp();
    public static native void ResizeAppGraphics(int w, int h);
    public static native void CloseAppGraphics();
    public static native void CloseGameApp();

    private class GameApp {
        //Vars
        //https://developer.android.com/reference/android/view/Choreographer
        //https://developer.android.com/reference/android/view/Choreographer.FrameCallback
        private static android.app.Activity parent = null;
        private static Choreographer choreo = null;
        private static LoadState state = new LoadState();

        private static final Choreographer.FrameCallback mainLoopChoreo = new Choreographer.FrameCallback(){
            @Override
            public void doFrame(long dtNano){
                if(!PMainActivity.appIsAlive) return;

                PMainActivity.RunGameApp();
                choreo.postFrameCallback(this);
            }
        };

        public static boolean Check(){ return state.Check(PMainActivity::GameAppDummy); }


        //Main
        public static void Open(android.app.Activity c){
            parent = c;
            choreo = Choreographer.getInstance();

            PMainActivity.InitGameApp();
        }

        public static void StartGameLoop(SurfaceHolder holder){
            PMainActivity.InitAppGraphics(holder.getSurface());

            choreo.postFrameCallback(mainLoopChoreo);
        }

        public static void Resize(int w, int h){ 
            PMainActivity.ResizeAppGraphics(w, h);
        }

        public static void EndGameLoop(){
            choreo.removeFrameCallback(mainLoopChoreo);

            PMainActivity.CloseAppGraphics();
        }

        public static void Close(){ 
            PMainActivity.CloseGameApp();
        }

    }


    //ScreenInfo
    public static native void ScreenInfoDummy();
    public static native void SetScreenDPI(int dpi);
    public static native void SetScreenResolution(int w_px, int h_px);

    private class ScreenInfo{
        private static LoadState state = new LoadState();
        public static boolean Check(){ return state.Check(PMainActivity::ScreenInfoDummy); }
        
        public static void UpdateScreenInfo(PMainActivity instance){
            DisplayMetrics displayMetrics = new DisplayMetrics();
            instance.getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
            
            int dpi = displayMetrics.densityDpi;
            int width_px = displayMetrics.widthPixels;
            int height_px = displayMetrics.heightPixels;
            
            PMainActivity.SetScreenDPI(dpi);
            PMainActivity.SetScreenResolution(width_px, height_px);
        }

    }


    //Touch
    public static native void InputTouchDummy();
    public static native void InputAddTouch(int id, float posX_percent01, float posY_percent01);
    public static native void InputSetTouch(int id, float posX_percent01, float posY_percent01);
    public static native void InputRemoveTouch(int id);

    private class TouchInput{ 
        private static LoadState state = new LoadState();
        public static boolean Check(){ return state.Check(PMainActivity::InputTouchDummy); }

        public static void ProcessTouch(MotionEvent event, float surfaceWidth, float surfaceHeight){
            int actionType = event.getActionMasked();
            int indexOfTriggeringPointer = event.getActionIndex();
            int idOfTriggeringPointer = event.getPointerId(indexOfTriggeringPointer);

            float posX = (float)event.getX( indexOfTriggeringPointer ) / surfaceWidth;
            float posY = (float)event.getY( indexOfTriggeringPointer ) / surfaceHeight;
       
            switch (actionType) {
                case MotionEvent.ACTION_DOWN:
                case MotionEvent.ACTION_POINTER_DOWN:
                    PMainActivity.InputAddTouch(idOfTriggeringPointer, posX, posY); 
                break;

                case MotionEvent.ACTION_MOVE:
                    PMainActivity.InputSetTouch(idOfTriggeringPointer, posX, posY);   
                    break;
                    
                case MotionEvent.ACTION_POINTER_UP:
                case MotionEvent.ACTION_UP:
                    PMainActivity.InputRemoveTouch(idOfTriggeringPointer);            
                    break;
            }    
        }
    }


    //Controller
    public static native void InputControllerDummy();
    
    public static native void InputControllerButtonDown(int controllerID, int axisIndex);
    public static native void InputControllerButtonUp(int controllerID, int axisIndex);
    
    public static native void InputControllerAxisUpdate(int controllerID, int axisIndex, float value);
    public static native void AddController(int controllerID);
    public static native void RemoveController(int controllerID);

    private ControllerHandler controllerModule = null;

    private class ControllerHandler implements InputManager.InputDeviceListener {
        private static LoadState state = new LoadState();
        public static boolean Check(){ return state.Check(PMainActivity::InputControllerDummy); }

        //https://developer.android.com/reference/android/hardware/input/InputManager.InputDeviceListener
        //https://developer.android.com/reference/android/hardware/input/InputManager#getInputDevice(int)

        private List<Integer> currentIDs = new ArrayList<Integer>();
        private boolean IsDeviceController(int deviceID){
            //https://developer.android.com/reference/android/view/InputDevice

            InputDevice device = input.getInputDevice(deviceID);
            if (device == null){
                PLog.Default("Device: " + deviceID + " was null and thus not a controller!");
                return false;
            }

            int deviceSources = device.getSources();
            boolean isController = (deviceSources & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK;
            isController = isController || (deviceSources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD;

            return isController;
        }

        public ControllerHandler(int currentIDs[]){
            for (int id : currentIDs){
                onInputDeviceAdded(id);
            }
        }

        public void Close(){
            PLog.Default("Clearing controllers");
            for (int id : currentIDs){  
                PMainActivity.RemoveController(id);
            }
            currentIDs.clear();
        }

        @Override
        public void onInputDeviceAdded(int deviceID){
            if(!IsDeviceController(deviceID)){
                PLog.Default("onAdded: device is not a controller: " + deviceID);
                return;
            }
            PLog.Default("Adding: " + deviceID);

            PMainActivity.AddController(deviceID);
            currentIDs.add(deviceID);
        }

        @Override
        public void onInputDeviceChanged(int deviceID){
            int newID = input.getInputDevice(deviceID).getId();
            if(!IsDeviceController(newID)){
                PLog.Default("onChanged: device is not a controller: " + newID);
                return;
            }
            PLog.Default("Changing controller " + deviceID + " to " + newID);

            if(currentIDs.contains(deviceID)){
                PMainActivity.RemoveController(deviceID);
                currentIDs.remove(Integer.valueOf(deviceID));
            }
      
            if(!currentIDs.contains(newID)){
                PMainActivity.AddController(newID);
                currentIDs.add(newID);
            }
        }
    
        @Override
        public void onInputDeviceRemoved(int deviceID){
            if(!IsDeviceController(deviceID)){
                PLog.Default("onRemoved: device is not a controller: " + deviceID);
                return;
            }
            PLog.Default("Removing: " + deviceID);

            PMainActivity.RemoveController(deviceID);
            currentIDs.remove(Integer.valueOf(deviceID));
        }

        public void ProcessMotionEvent(MotionEvent event){
            //https://developer.android.com/develop/ui/views/touch-and-input/game-controllers/controller-input
            //https://developer.android.com/reference/android/view/MotionEvent#getActionMasked()
            
            //Validate source
            int controllerID = event.getDeviceId();
            if(!currentIDs.contains( controllerID) ) return;
 
            //Validate operation
            boolean isFromJoystick = 
            (event.getSource() & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK &&
            (event.getSource() & InputDevice.SOURCE_GAMEPAD) != InputDevice.SOURCE_GAMEPAD;
            boolean actionIsMove = event.getActionMasked() == MotionEvent.ACTION_MOVE;
            
            if(!isFromJoystick || !actionIsMove) return;
            
            //Update controller
            PMainActivity.InputControllerAxisUpdate(controllerID, MotionEvent.AXIS_X, event.getAxisValue(MotionEvent.AXIS_X));
            PMainActivity.InputControllerAxisUpdate(controllerID, MotionEvent.AXIS_Y, event.getAxisValue(MotionEvent.AXIS_Y));
            
            //second joystick convention https://developer.android.com/reference/android/view/MotionEvent#AXIS_RZ
            PMainActivity.InputControllerAxisUpdate(controllerID, MotionEvent.AXIS_Z, event.getAxisValue(MotionEvent.AXIS_Z));    
            PMainActivity.InputControllerAxisUpdate(controllerID, MotionEvent.AXIS_RZ, event.getAxisValue(MotionEvent.AXIS_RZ));
            
            PMainActivity.InputControllerAxisUpdate(controllerID, MotionEvent.AXIS_HAT_X, event.getAxisValue(MotionEvent.AXIS_HAT_X));
            PMainActivity.InputControllerAxisUpdate(controllerID, MotionEvent.AXIS_HAT_Y, event.getAxisValue(MotionEvent.AXIS_HAT_Y));
        }

        public void ProcessKeyEvent(KeyEvent event){
            //https://developer.android.com/reference/android/view/KeyEvent
            //https://developer.android.com/develop/ui/views/touch-and-input/game-controllers/controller-input#java

            //Validate source
            int controllerID = event.getDeviceId();
            if(!currentIDs.contains( Integer.valueOf(controllerID)) ) return;

            //Validate operation
            boolean isFromControllerButtons = 
            (event.getSource() & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD &&
            (event.getSource() & InputDevice.SOURCE_JOYSTICK) != InputDevice.SOURCE_JOYSTICK;

            if(!isFromControllerButtons) return;


            //Do
            int actionType = event.getKeyCode();
            boolean actionIsDown = event.getAction() == KeyEvent.ACTION_DOWN;
            boolean actionIsUp = event.getAction() == KeyEvent.ACTION_UP;

            if(actionIsDown) PMainActivity.InputControllerButtonDown(controllerID, actionType);
            else if(actionIsUp) PMainActivity.InputControllerButtonUp(controllerID, actionType);
        }
        
    }


    //Keyboard
    public static native void InputKeyboardDummy();
    public static native void InputKeyboardButtonDown(int buttonID);
    public static native void InputKeyboardButtonUp(int buttonID);

    private KeyboardHandler keyboardModule = null;

    public class KeyboardHandler{   //Currently handles only one keyboard at a time. Doesn't need to be instance based~ but for consistencies sake (controllerhandler) it is.
        private static LoadState state = new LoadState();
        public static boolean Check(){ return state.Check(PMainActivity::InputKeyboardDummy); }

        public void ProcessKeyEvent(KeyEvent event){

            boolean isFromKeyboard = (event.getSource() & InputDevice.SOURCE_KEYBOARD) == InputDevice.SOURCE_KEYBOARD;
            if(!isFromKeyboard) return;

            int keyCode = event.getKeyCode();
            boolean actionIsDown = event.getAction() == KeyEvent.ACTION_DOWN;
            boolean actionIsUp = event.getAction() == KeyEvent.ACTION_UP;

            if(actionIsDown){
                PMainActivity.InputKeyboardButtonDown(keyCode);
            } else{
                PMainActivity.InputKeyboardButtonUp(keyCode);
            }
        }
    }


    //Assets
    public static native void AssetDummy();
    public static native void AssetsLoadManager(Object assetManager);

    private class Assets {
        private static LoadState state = new LoadState();
        public static boolean Check(){ return state.Check(PMainActivity::AssetDummy); }

        public static void Init(PMainActivity source){ 
            AssetManager am = source.getResources().getAssets();
            PMainActivity.AssetsLoadManager(am);
        } 
    }


    //Save RW Directory
    public static native void RWDirDummy();
    public static native void SetAndroidRWDir(String path);

    private class RWDir{
        private static LoadState state = new LoadState();
        public static boolean Check(){ return state.Check(PMainActivity::RWDirDummy); }

        public static void Set(PMainActivity instance){
            //https://developer.android.com/training/data-storage/app-specific
            //https://developer.android.com/reference/java/io/File#getAbsolutePath()

            PMainActivity.SetAndroidRWDir( instance.getFilesDir().getAbsolutePath() );
        }
    }

//endregion
}