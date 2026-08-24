package _Server;
import java.io.OutputStream;
import java.lang.annotation.Native;
import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.InputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.security.KeyStore;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.function.Consumer;
import java.util.logging.Handler;
import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLServerSocketFactory;


//This class acts like a Facade for hosting a server.
public abstract class NativeSocketServer implements Server{
    //Config
    private static final int _maxConcurrentThreads = 8;
    private static final int _maxCustomHTMLDepth = 0;
    public static final String _security_whitelist = "whitelist";

    public static class Config{
        public String protocol = "";
        public String targetIP = "";
        public String securityConcept = "";
        public String pkcsPassword = "";
        public int targetPort = 0;
        public int maxClients = 1;

        public synchronized String GetURL(){ return protocol + "://" + targetIP + ":" + targetPort; }
    };

    //To overwrite per OS:
    public abstract InputStream GetKeystorep12() throws Exception;
    public abstract void RequestIPAccept(String ip, String hash);
    public abstract void Log(String log, int importance);

    //Main Functionality
    public NativeSocketServer(Config cfg) throws Exception{
        //https://www.baeldung.com/java-executors-cached-fixed-threadpool

        if(cfg.protocol == null || cfg.targetIP == null || cfg.pkcsPassword == null) throw new Exception("Invalid NativeSocketServer Config!");
        this.cfg = cfg;
    }
    
    public void Run() throws Exception {
        //Validate
        if(isRunning) throw new Exception("Cannot run server twice~ please Interrupt() first!");
        Log("Starting server at: " + cfg.GetURL() + "\nwith max clients: " + cfg.maxClients + "\nand security-concept: " + cfg.securityConcept, 2);
        
        //Init Resources
        threadPool = Executors.newFixedThreadPool(_maxConcurrentThreads);
        isRunning = true;
        usesCustomHTML = GetAllFilesFromDirectory(".html", _maxCustomHTMLDepth).size() > 0; 

        //Select Socket
        switch (cfg.protocol) {
            case "http":
                //https://docs.oracle.com/javase/8/docs/api/java/net/Socket.html
                socket = new ServerSocket();
            break;

            case "https":
                //https://www.misterpki.com/pkcs12/
                //https://docs.oracle.com/javase/8/docs/api/java/security/KeyStore.html
                //https://docs.oracle.com/en/java/javase/17/docs/api/java.base/javax/net/ssl/KeyManagerFactory.html
                //https://docs.oracle.com/en/java/javase/17/docs/api/java.base/javax/net/ssl/SSLContext.html
                //https://docs.oracle.com/en/java/javase/17/docs/api/java.base/javax/net/ssl/SSLServerSocketFactory.html

                //Note: The exact order of operations was proposed by AI (02.07.26) and written out by a human
                KeyStore ks = KeyStore.getInstance("PKCS12");
                char[] pw = cfg.pkcsPassword.toCharArray();
                ks.load( GetKeystorep12(), pw);
                
                KeyManagerFactory kmf = KeyManagerFactory.getInstance(KeyManagerFactory.getDefaultAlgorithm());
                kmf.init(ks, pw);
                
                SSLContext sctx = SSLContext.getInstance("TLS");
                sctx.init(kmf.getKeyManagers(), null, null);

                socket = sctx.getServerSocketFactory().createServerSocket();
            break;

            default:
                throw new Exception("Unknown Protocol: " + cfg.protocol);
        }
        
        //Run Server
        Log("Created socket, now binding...", 0);
        socket.bind( new InetSocketAddress(cfg.targetIP, cfg.targetPort) );

        while (!socket.isClosed() && isRunning) {
            try{
                Socket client = socket.accept();
                if(!isRunning || socket.isClosed()) break;
                client.setKeepAlive(false);
                threadPool.submit(() -> HandleClient(client));

            } catch(Exception e){
                if(!isRunning || socket.isClosed()) break;
                Log(e.getMessage(), -1);
            }
        }

        //Close
        Interrupt();
        socket = null;
    }

    private void HandleClient(Socket client) {
        try(
            Socket autoClose = client;
            BufferedReader in = new BufferedReader( new InputStreamReader(client.getInputStream()) );
            OutputStream out = client.getOutputStream();
        ){
            //Get Resources
            String clientID = client.getInetAddress().getHostAddress();
            String clientHash =  clientID.chars().sum() + ":" + clientID.hashCode();

            //Validate
            switch (cfg.securityConcept) {                        
                case _security_whitelist:
                    //Check whitelist
                    synchronized(ipWhiteList){
                        boolean hasWhiteListed = HasIPWhiteListed(clientID);
                        boolean hasWhitelistCapacity = ipWhiteList.size() < cfg.maxClients;
                        if(!hasWhiteListed && hasWhitelistCapacity) RequestIPAccept(clientID, clientHash);
                        
                        //Validate Client
                        if(!hasWhitelistCapacity && !hasWhiteListed){
                            out.write(HTTParse.WriteText("Max clients reached!"));
                            throw new Exception("Max clients reached!" + clientID);
                        }
                        
                        if(!hasWhiteListed){
                            out.write(HTTParse.WriteText("Your client has not been whitelisted yet, hash: " + clientHash));
                            throw new Exception("Not whitelisted IP: " + clientID);
                        }
                    }
                break;

                default:
                    break;
            }
            Log("Serving client: " + clientID + ":" + client.getPort(), 2);

            //Get full request-header
            String rl = "";
            String firstLine = null;
            String header = ""; 
            StringBuilder headerBuilder = new StringBuilder();
            int maxLines = 1024;
            while( (rl = in.readLine()) != null && maxLines-- > 0){
                if(rl.isEmpty() || rl.equals("\r")) break;
                headerBuilder.append(rl).append(" \n");
                if(firstLine == null) firstLine = rl;
            } 
            header = headerBuilder.toString();
            if(firstLine == null) throw new Exception("Invalid HTTP Header!");

            //Parse header
            Log("Header from " + clientID + ":\n" + firstLine + "\n", 0);

            String[] headerParts = header.split(" ", 2);
            if(headerParts.length < 1) throw new Exception("Invalid HTTP-Header!");

            String method = headerParts[0].trim();
            if(method.isEmpty()) throw new Exception("Empty HTTP Method!!");

            //Handle response
            switch (method) {
                case "GET":
                    HTTParse.HandleGET(out, header, usesCustomHTML, this);
                return;

                case "PUT":
                    HTTParse.HandlePUT(out, header, this);
                break;

                default:
                    throw new Exception("Unhandled HTTP request: " + method);
            }

        } catch (Exception e){ 
            String msg = e.getMessage();
            if(msg != null){
                //on a self-signed certificate we gonna' get a lots of these...
                if(!msg.contains("SSLV3_ALERT_CERTIFICATE_UNKNOWN")) Log("Server runtime error: " + msg, -1);
            }
        }

        Log("Served!\n", 1);
    }

    public synchronized void Interrupt(){
        if(!isRunning) return;

        isRunning = false;
        ipWhiteList.clear();
        try{
            if(socket != null) socket.close();
            threadPool.shutdown();
            if(!threadPool.awaitTermination(3, TimeUnit.SECONDS)){
                threadPool.shutdownNow();
            }
            Log("Closed server!", 1);
        } catch (Exception e){
            threadPool.shutdownNow();
            Log("INTERRUPT ERROR: " + e.getMessage(), -1);
        }
    }


    //Utils
    public boolean HasIPWhiteListed(String ip){ 
        synchronized(ipWhiteList){
            return ipWhiteList.contains(ip);
        }
    }

    public void WhitelistIP(String ip){ 
        synchronized(ipWhiteList){
            ipWhiteList.add(ip);
        }
        Log("Whitelisting IP: " + ip, 0); 
    }

    public boolean IsRunning(){ return isRunning; }
    public Config GetConfig(){ return cfg; }

    //Variables
    private ExecutorService threadPool = null;
    private Config cfg = new Config();
    private boolean usesCustomHTML = false;
    private List<String> ipWhiteList = new ArrayList<String>();
    private volatile ServerSocket socket = null;
    private volatile boolean isRunning = false;
}