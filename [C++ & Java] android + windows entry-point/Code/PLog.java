package P_App._Android;
import android.util.Log;


public class PLog {

    //CONFIG
    private static boolean isActive = true;
    public static void Enable(){ isActive = true; }

    public static void Disable(){ isActive = false; }

    
    //LOGGING
    public static void Default(String message){
        if (!isActive) { return; }
        Log.v("pengine.log", "Java: " + message);
    }    

    private static int logIteration = 0;
    public static void It(){
        Default(String.valueOf(logIteration));
        logIteration++;
    }

    public static void It_Reset(){
        Default("Reset log iterator to 0");
        logIteration = 0;
    }
}