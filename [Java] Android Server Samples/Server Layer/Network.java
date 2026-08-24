package _Server;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.function.Consumer;
import java.util.function.Function;

public class Network{

    public static String GetTetheringIP() throws Exception{
        final String[] ip = new String[1];
        ip[0] = "";

        LoopInterfaces(
            (netInterface) -> { 
                String name = netInterface.getName().toLowerCase();

                //Note: This is what AI told would be equivalent to a USB-Tethered connection
                //I cannot really find a source to back this up. However~ it seems to work o.o
                return !name.contains("rndis") && !name.contains("usb") && !name.contains("ncm");
            },
            (addr) -> {
                String addressValue = addr.getHostAddress();
                boolean isLocal = addr.isSiteLocalAddress();       
                boolean isTypeC = addressValue.matches("192\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}");
                if(isLocal && isTypeC) ip[0] = addressValue;
            }
        );
        return ip[0];
    }

    public static List<String> GetLocalIPs() throws Exception{
        List<String> result = new ArrayList<String>();
        result.add("127.0.0.1");

        LoopInterfaces(
            (intrfc) -> { 
                try{
                    return intrfc.isLoopback() || !intrfc.isUp();
                } catch (Exception e){
                    return true;
                }
             },
            (addr) -> {
                String addressValue = addr.getHostAddress();
                boolean isLocal = addr.isSiteLocalAddress();       
                boolean isPrivateTypeC = addressValue.matches("192\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}");
                if(isLocal && isPrivateTypeC) result.add(addressValue);
            }
        );
        return result;
    }

    private static void LoopInterfaces(
        Function<NetworkInterface, Boolean> skipInterface, 
        Consumer<InetAddress> processAddres
    ) throws Exception{
        //https://docs.oracle.com/javase/8/docs/api/java/util/Enumeration.html
        //https://docs.oracle.com/en/java/javase/11/docs/api/java.base/java/net/NetworkInterface.html
        //https://docs.oracle.com/en/java/javase/11/docs/api/java.base/java/net/InetAddress.html
        //https://en.wikipedia.org/wiki/Private_network
        //https://stackoverflow.com/questions/5619345/what-does-inetaddress-issitelocaladdress-actually-mean

        //Get all interfaces
        Enumeration<NetworkInterface> allInterfaces = NetworkInterface.getNetworkInterfaces();
        while(allInterfaces.hasMoreElements()){
            //Validate interface
            NetworkInterface intrfc = allInterfaces.nextElement();
            if(skipInterface.apply(intrfc)) continue;
            
            //Get all ip-addresses
            Enumeration<InetAddress> ipAddresses = intrfc.getInetAddresses();
            while(ipAddresses.hasMoreElements()){
                InetAddress addr = ipAddresses.nextElement();
                processAddres.accept(addr);
            }
        }
        
    }
    
}