package P_App._Android;
import java.util.function.Consumer;

public class LoadState{
    private enum Presence{
        ASK((byte)0),
        CONFIRMED((byte)1),
        ABSENT((byte)-1);

        private byte value = 0;
        private Presence(byte val){ value = val; }
    }


    private Presence state = Presence.ASK;
    public LoadState(){ state = Presence.ASK; }

    public boolean Check(Runnable nativeDummy){
        if(state != Presence.ASK) return state == Presence.CONFIRMED;

        try{
            nativeDummy.run();
            state = Presence.CONFIRMED;
        } catch(Error e) {
            state = Presence.ABSENT;
        } 

        return state == Presence.CONFIRMED;
    }
}