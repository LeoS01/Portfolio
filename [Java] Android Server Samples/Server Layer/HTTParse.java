package _Server;
import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.IOError;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Map;
import java.util.function.Function;
import java.util.function.BiFunction;

public class HTTParse{
    //Config
    private static final Map<String, String> suffixToMime = Map.of(
        "png", "image/png",
        "jpg", "image/jpeg",
        "jpeg", "image/jpeg",
        "html", "text/html",
        "css", "text/css",
        "js", "text/javascript"
    );
    private static final String _charSet = "UTF-8";
    private static final int _chunkSize = 8192;

    public static boolean HandleGET(OutputStream out, String inputHeader, boolean useCustomHTML, Server host) throws Exception {
        //TODO: Improve exception handling with return codes!

        //Utils
        Function<String, Boolean> Error = (message) -> {
            try{
                out.write(WriteText(message));
                out.flush();
            } catch (Exception e) {}
            return false;
        };

        //Prepare
        String errorResponse = "Invalid header: " + inputHeader;
        String[] inputTokens = inputHeader.split(" ");

        //Validate
        if(inputTokens.length < 3) return Error.apply(errorResponse + "invalid http-header length of: " + inputTokens.length);
        if(!"GET".equals(inputTokens[0])) return Error.apply(errorResponse + " invalid request-type: " + inputTokens[0]);

        //Clean requested URI
        if(inputTokens[1].equalsIgnoreCase("/")) inputTokens[1] = "Index.html";
        if(inputTokens[1].startsWith("/")) inputTokens[1] = inputTokens[1].substring(1);

        //Get Type
        String[] requestSuffix = inputTokens[1].split("\\.");
        if(requestSuffix.length < 2) return Error.apply(errorResponse + " invalid suffix-split of length:" + requestSuffix.length + " on: " + inputTokens[1]);

        //Select
        String prefix = requestSuffix[0];
        String suffix = requestSuffix[requestSuffix.length - 1];

        //Special fetch request: '*'' wildcard for all files with a specific suffix
        if("*".equals(prefix)){
            //Get Files
            ArrayList<String> allFiles = host.GetAllFilesFromDirectory(suffix, -1);
            
            //Build JSON
            StringBuilder jsonBuilder = new StringBuilder();
            jsonBuilder.append("[");
            for(int i = allFiles.size() - 1; i >= 0 ; i--){
                jsonBuilder.append("\"" +allFiles.get(i)+ "\"");
                if(i == 0) continue;
                jsonBuilder.append(", ");
            }
            jsonBuilder.append("]");

            out.write(
                WriteGenericResponse("200 OK", "application/json", jsonBuilder.toString())
            );
            out.flush();
            return true;
        }

        //Fetches single file?
        switch (suffix) {
            case "png":
            case "jpg":
            case "jpeg":
                InputStream is = host.GetFile(inputTokens[1], -1);
                try{

                    byte[] imageBytes = is.readAllBytes();
                    out.write(
                        AnnounceBytes("200 OK", suffixToMime.get(suffix), imageBytes.length)
                    );
                    
                    out.write(imageBytes);
                } catch (Exception e) {
                    // TODO: handle exception
                }
                is.close();
            break;

            case "html":
            case "css":
            case "js":
                InputStream file = null;
                if(useCustomHTML) file = host.GetFile(inputTokens[1], -1);
                if(file == null) file = host.GetAsset(inputTokens[1]);

                try {
                    out.write(
                        WriteGenericResponse("200 OK", suffixToMime.get(suffix), GetIStrAsString(file) )
                    );
                } catch (Exception e) {
                    // TODO: handle exception
                }
                file.close();
            break;

            case "mp4":
            case "mp3":
                long requestStart = 0;
                long requestEnd = -1;

                for(int i = 0; i < inputTokens.length; i++){
                    
                    //Is relevant line?
                    if(i == 0) continue;
                    if(!inputTokens[i-1].contains("Range:")) continue;
                    if(!inputTokens[i].contains("bytes=")) continue;

                    //Extract relevant numerical values
                    try{
                        //Get
                        String[] values = inputTokens[i].split("=");
                        values = values[1].split("-");
                        
                        //Parse
                        requestStart = Long.parseLong(values[0]);
                        requestEnd = Long.parseLong(values[1]);
                    } catch (Exception e){
                        //Just continue using the whole file~
                        requestStart = 0;
                        requestEnd = -1;
                    }
                }
                
                String mediaType = suffix.equalsIgnoreCase("mp3")? "audio/mpeg" : "video/mp4";
                try{
                    //Prepare
                    FileInputStream fs = host.GetFile(inputTokens[1], -1);
                    try{

                        long totalSize = fs.getChannel().size();
                        fs.getChannel().position(requestStart);
                        
                        //Send partial content header
                        if(requestEnd < 0) requestEnd = totalSize - 1;
                        out.write( WritePartialContent(mediaType, totalSize, requestStart, requestEnd) );
                        
                        //Stream the rest!
                        byte[] chunk = new byte[_chunkSize];
                        long requestedSize = requestEnd - requestStart + 1;
                        long bytesRead = 0;
                        while(bytesRead < requestedSize){
                            int remaining = (int)Math.min((long)chunk.length, requestedSize - bytesRead);
                            int read = fs.read(chunk, 0, remaining);
                            if(read == -1) break;
                            
                            bytesRead += read;
                            out.write(chunk, 0, read);
                        }
                    } catch (Exception e){
                        //TODO: Handle exception
                    }
                    fs.close();
                } catch (Exception e) {
                    out.flush();
                    return Error.apply(e.getMessage());
                }
            break;

            default:
                out.write( WriteText(errorResponse + " invalid suffix: " + suffix) );
            break;
        }
        
        out.flush();
        return true;
    }

    public static boolean HandlePUT(OutputStream out, String inputHeader, Server host){
        //Utils
        Function<String, Boolean> Error = (message) -> {
            try{
                out.write(WriteText(message));
                out.flush();
            } catch (Exception e) {}
            return false;
        };

        //Prepare
        String[] inputTokens = inputHeader.split(" ");
        String errorResponse = "Invalid PUT: " + inputHeader;

        //Validate
        if(inputTokens.length < 3) return Error.apply(errorResponse + "invalid http-header length of: " + inputTokens.length);
        if(!"PUT".equals(inputTokens[0])) return Error.apply(errorResponse + " invalid request-type: " + inputTokens[0]);

        //TODO: Parse URI and data!

        return true;
    }

    public static byte[] WriteText(String content) throws Exception {
        return WriteGenericResponse("200 OK", "text/plain", content);
    }

    public static byte[] AnnounceBytes(String code, String type, int byteCount) throws Exception {
        //https://www.rfc-editor.org/rfc/rfc9110.html
        String response = 
        "HTTP/1.1 " + code + "\r\n" +
        "Content-Type: " + type + "\r\n" +
        "Content-Length: " + byteCount + "\r\n" +
        "Connection: close" + "\r\n" +
        "\r\n" ;

        return response.getBytes(_charSet);
    }

    public static byte[] WriteGenericResponse(String code, String type, String content) throws Exception {
        //https://www.rfc-editor.org/rfc/rfc9110.html
        String response = 
        "HTTP/1.1 " + code + "\r\n" +
        "Content-Type: " + type + "\r\n" +
        "Content-Length: " + content.getBytes().length + "\r\n" +
        "Connection: close" + "\r\n" +
        "\r\n" 
        + content;

        return response.getBytes(_charSet);
    }

    private static byte[] WritePartialContent(String type, long fullLength, long start, long end) throws Exception {
        String header = 
        "HTTP/1.1 206 Partial Content" + "\r\n" +
        "Content-Type: " + type + "\r\n" +
        "Content-Length: " + (end - start + 1) + "\r\n" +
        "Content-Range: bytes " + start + "-" + end + "/" + fullLength + "\r\n" +
        "Connection: close" + "\r\n" +
        "\r\n";
        return header.getBytes(_charSet);
    }

    private static byte[] WriteGenericCloseResponse(String type) throws Exception {
        String response = 
            "HTTP/1.1 200 OK" + "\r\n" +
            "Content-Type: " + type + "\r\n" +
            "Connection: close\r\n" +
            "\r\n";
        return response.getBytes(_charSet);
    }

    
    private static String GetIStrAsString(InputStream fs){
        //https://docs.oracle.com/javase/8/docs/api/java/io/BufferedReader.html#readLine--
        //https://docs.oracle.com/javase/8/docs/api/java/lang/StringBuilder.html
        //https://docs.oracle.com/javase/8/docs/api/java/io/InputStreamReader.html
        if(fs == null) return "";
        try{
            byte[] bytes = fs.readAllBytes();
            return new String(bytes, StandardCharsets.UTF_8);
        } catch(Exception e){
            return "Error during [InputStream->String] conversion!";
        }
    }
}