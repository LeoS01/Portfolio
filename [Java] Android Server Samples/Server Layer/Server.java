package _Server;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.ArrayList;

public interface Server {
    public InputStream GetAsset(String id) throws Exception;                                         //'Assets' represent application specific files that act as defaults.
    public FileInputStream GetFile(String path, int maxDepth) throws Exception;                      //'Files' represent actual user files on a disk.

    public boolean WriteData(byte[] data, String destination) throws Exception;
    public ArrayList<String> GetAllFilesFromDirectory(String type, int maxDepth) throws Exception;
}