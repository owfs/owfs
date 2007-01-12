/**
 * ownet/java
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 	
 * OWFS is an open source project developed by Paul Alfille and hosted at
 * http://www.owfs.org
 * 
 * mailto:gzoug@aueb.gr
 */

package org.owfs.ownet;

import java.io.*;
import java.net.*;
import java.util.Vector;

/**
 * @author George M. Zouganelis
 * @version $Id:
 */
public class OWNet {

//
// default values and constants
//
    static final String OWNET_DEFAULT_HOST = "127.0.0.1";
    static final int OWNET_DEFAULT_PORT = 1234;
    
    static private final int OWNET_DEFAULT_DATALEN = 8192;
    
    
    static private final int OWNET_MSG_ERROR = 0;
    static private final int OWNET_MSG_NOP = 1;
    static private final int OWNET_MSG_READ = 2;
    static private final int OWNET_MSG_WRITE = 3;
    static private final int OWNET_MSG_DIR = 4;
    static private final int OWNET_MSG_SIZE = 5;
    static private final int OWNET_MSG_PRESENCE = 6;
    static private final int OWNET_MSG_DIRALL = 7;
    static private final int OWNET_MSG_GET = 8;
    static private final int OWNET_MSG_READ_ANY = 99999;
    
    
    static private final int OWNET_PROT_STRUCT_SIZE = 6;
    static private final int OWNET_PROT_VERSION = 0;
    static private final int OWNET_PROT_PAYLOAD = 1;
    static private final int OWNET_PROT_FUNCTION = 2;
    static private final int OWNET_PROT_RETVALUE = 2;
    static private final int OWNET_PROT_FLAGS = 3;
    static private final int OWNET_PROT_DATALEN = 4;
    static private final int OWNET_PROT_OFFSET = 5;
    

//
// class private variables
//
    private String remoteServer = OWNET_DEFAULT_HOST;     // default Server
    private int remotePort = OWNET_DEFAULT_PORT;          // default port
    private int formatflags = 259;
    private boolean debug = false;
    private int defaultDataLen = OWNET_DEFAULT_DATALEN;
    
//
// Constructors
//
    
    /** Create a new instance of OWNet */
    public OWNet() {
       // just a constructor        
    }
    
    /**
     * Create a new instance of OWNet
     * @param server ip or hostname of owserver to connect to
     * @param port remote's owserver port to connect to
     */
    public OWNet(String server, int port) {
        setRemoteServer(server);
        setRemotePort(port);
    }


//
// Getters / Setters
//
    
    /**
     * Get debugging mode
     * @return debug mode is on
     */
    public boolean isDebug() {
        return debug;
    }

    /**
     * Set debugging mode
     * @param debug enable debug mode
     */
    public void setDebug(boolean debug) {
        this.debug = debug;
    }
        
    // member access
    /**
     * Get remote server
     * @return remote server
     */
    public String getRemoteServer(){
        return remoteServer;
    }
    
    /**
     * Set remote server
     * @param newServer remote server to connect
     */
    public void setRemoteServer(String newServer){
        remoteServer = newServer;
    }

    /**
     * Get remote server's port
     * @return remote server's listening port
     */
    public int getRemotePort(){
        return remotePort;
    }

    /**
     * Set remote server's port
     * @param newPort remote server's listening port
     */
    public void setRemotePort(int newPort){
        remotePort = newPort;
    }

    /**
     * Get formating flags
     * @return formating flags (check http://www.owfs.org/index.php?page=owserver-flag-word)
     */
    public int getFormatflags() {
        return formatflags;
    }

    /**
     * Set formating flags
     * @param newformatflags formating flags (check http://www.owfs.org/index.php?page=owserver-flag-word)
     */
    public void setFormatflags(int newformatflags) {
        formatflags = newformatflags;
    }

    /**
     * Get default expected data length for receiving data
     * @return default payload
     */
    public int getDefaultDataLen() {
        return defaultDataLen;
    }

    /**
     * Set default excpected data length for receiving data
     * @param newDefaultDataLen default length of data to expect
     */
    public void setDefaultDataLen(int newDefaultDataLen) {
        defaultDataLen = newDefaultDataLen;
    }
    

    
    
//
// Private methods for protocol and network access
//
    
    /**
     * Print ownet message packet to STDERR
     * @param tag tag to append
     * @param msg message buffer to show
     */
    private void debugprint(String tag, int[] msg){
        if (!debug) return;
        System.err.print("* OWNET.Java DEBUG [" + tag + "] : " );
        for (int i=0; i<msg.length; i++) {
            System.err.print(msg[i] + " ");System.err.flush();
        }
        System.err.println();
        System.err.flush();
    }
       
    /**
     * Send a ownet message packet
     * @param out output stream to use
     * @param function ownet's header field
     * @param payloadlen ownet's header field
     * @param datalen ownet's header field
     * @throws java.io.IOException Exception to throw, when something is wrong
     */
    private void sendPacket(DataOutputStream out, int function, int payloadlen, int datalen) throws IOException
    {
        int[] msg = new int[OWNET_PROT_STRUCT_SIZE];
        msg[OWNET_PROT_VERSION] = 0; 
        msg[OWNET_PROT_PAYLOAD] = payloadlen; 
        msg[OWNET_PROT_FUNCTION] = function; 
        msg[OWNET_PROT_FLAGS] = formatflags; 
        msg[OWNET_PROT_DATALEN] = datalen; 
        msg[OWNET_PROT_OFFSET] = 0; 
        debugprint("sendPacket", msg); 
        for (int i = 0; i<OWNET_PROT_STRUCT_SIZE; i++) out.writeInt(msg[i]);  
    }

    /**
     * Get a ownet message packet
     * @param in input stream to use
     * @return the ownet's received header packet from server
     * @throws java.io.IOException Exception to throw, when something is wrong
     */
    private int[] getPacket(DataInputStream in) throws IOException 
    {
        int[] msg = new int[OWNET_PROT_STRUCT_SIZE];
        for (int i = 0; i<OWNET_PROT_STRUCT_SIZE; i++) msg[i] = in.readInt(); 
        debugprint("getPacket", msg);
        return msg;
    }
    
    /**
     * Get a data packet from remote server
     * @param in input stream to use
     * @param packetHeader ownet's header msg packet to use for receiving auxilary data
     * @throws java.io.IOException Exception to throw, when something is wrong
     * @return data received from remote server
     */
    private String getPacketData(DataInputStream in, int[] packetHeader) throws IOException {
        byte[] data = new byte[packetHeader[OWNET_PROT_PAYLOAD]];
        String retVal = null;
        in.read(data,0,data.length); 
        retVal = new String(data,packetHeader[OWNET_PROT_OFFSET],packetHeader[OWNET_PROT_DATALEN]);
        /*
         if (retVal.indexOf(0)>-1) {
            retVal = retVal.substring(0,retVal.indexOf(0));
        }
         */
        return retVal;
    }
    
    /**
     * Transmit a Java String as a C-String (null terminated)
     * @param out output stream to use
     * @param str string to transmit
     * @throws java.io.IOException Exception to throw, when something is wrong
     */
    private void sendCString(DataOutputStream out, String str) throws IOException {
       out.write(str.getBytes());out.writeByte(0);
    }            

    /**
     * Internal method to read from server
     * @param path attribute's path
     * @param expectedDataLen expected length of data to receive
     * @throws java.io.IOException Exception to throw, when something is wrong
     * @return attribute's value
     */
    private String OW_Read(String path, int expectedDataLen) throws IOException 
    {
        String retVal = "";

        Socket ows = null;
        DataInputStream in;
        DataOutputStream out;
        int[] msg = null;
        
        // try to connect to remote server
        // possible exception must be caught by the calling method
        
        ows = new Socket(remoteServer, remotePort);
        in = new DataInputStream(ows.getInputStream());
        out = new DataOutputStream(ows.getOutputStream());
        
        sendPacket(out, OWNET_MSG_READ, path.length()+1, expectedDataLen);
        sendCString(out,path);
        msg = getPacket(in);
        if (msg[OWNET_PROT_RETVALUE] >= 0) {  
            if (msg[OWNET_PROT_PAYLOAD]>=0) retVal = getPacketData(in,msg);
        } else {
          in.close();
          out.close();
          ows.close();
          throw new IOException("Error reading from server. Error " + msg[OWNET_PROT_RETVALUE]);
        }
        // close streams and connection
        // possible exception must be caught by the calling method
        in.close();
        out.close();
        ows.close();
        return retVal;
    }
    /**
     * Internal method to read from server, using default excpected datalen
     * @param path attribute's path
     * @throws java.io.IOException Exception to throw, when something is wrong
     * @return attribute's value
     */
    private String OW_Read(String path) throws IOException 
    {
        return OW_Read(path,this.defaultDataLen);
    }    
    
    
    /**
     * Internal method to write to server
     * @param path attribute's path
     * @param value value to set
     * @return sucess state (most likely to be true or exception)
     * @throws java.io.IOException Exception to throw, when something is wrong
     */
    private boolean OW_Write(String path, String value) throws IOException 
    {
        boolean retVal = false;

        Socket ows = null;
        DataInputStream in;
        DataOutputStream out;
        int[] msg = null;
        
        // try to connect to remote server
        // possible exception must be caught by the calling method
        ows = new Socket(remoteServer, remotePort);
        in = new DataInputStream(ows.getInputStream());
        out = new DataOutputStream(ows.getOutputStream());
        
        sendPacket(out, OWNET_MSG_WRITE, path.length() + 1 + value.length() + 1, value.length() + 1);
        sendCString(out,path);
        sendCString(out,value);
        msg = getPacket(in);      

        if (msg[OWNET_PROT_RETVALUE] >= 0) {
            retVal = true;
        } else {
            in.close();
            out.close();
            ows.close();
            throw new IOException("Error writing to server. Error " + msg[OWNET_PROT_RETVALUE]);
        }

        // close streams and connection
        // possible exception must be caught by the calling method
        in.close();
        out.close();
        ows.close();

        return retVal;
    }

    /**
     * Internal method to get presence state from server
     * @param path element to check
     * @throws java.io.IOException Exception to throw, when something is wrong
     * @return presence
     */
    private boolean OW_Presence(String path) throws IOException 
    {
        boolean retVal = false;

        Socket ows = null;
        DataInputStream in;
        DataOutputStream out;
        int[] msg = null;
        
        // try to connect to remote server
        // possible exception must be caught by the calling method
        ows = new Socket(remoteServer, remotePort);
        in = new DataInputStream(ows.getInputStream());
        out = new DataOutputStream(ows.getOutputStream());
        
        sendPacket(out, OWNET_MSG_PRESENCE, path.length()+1, 0);
        sendCString(out,path);
        msg = getPacket(in);
        if (msg[OWNET_PROT_RETVALUE] >= 0 ) {
            retVal = true;
        } else {
            in.close();
            out.close();
            ows.close();
            throw new IOException(path + " not found. Error " + msg[OWNET_PROT_RETVALUE]);
        }

        // close streams and connection
        // possible exception must be caught by the calling method
        in.close();
        out.close();
        ows.close();
        return retVal;
    }
    
    
    /**
     * Internal method to get directory list from server (multipacket mode)
     * @param path directory to list
     * @throws java.io.IOException Exception to throw, when something is wrong
     * @return vector list of elements found
     */
    private Vector<String> OW_Dir(String path) throws IOException 
    {
        Vector<String> retVal = new Vector<String>();
        Socket ows = null;
        DataInputStream in;
        DataOutputStream out;
        int[] msg = null;
        
        // try to connect to remote server
        // possible exception must be caught by the calling method
        ows = new Socket(remoteServer, remotePort);
        in = new DataInputStream(ows.getInputStream());
        out = new DataOutputStream(ows.getOutputStream());
        
        sendPacket(out, OWNET_MSG_DIR, path.length()+1, 0);
        sendCString(out,path);
        
        do{
            msg = getPacket(in);            
            if (msg[OWNET_PROT_RETVALUE] >= 0){
               if (msg[OWNET_PROT_PAYLOAD] > 0)  retVal.add(getPacketData(in,msg));
            } else {
               in.close();
               out.close();
               ows.close();
               throw new IOException("Error getting Directory. Error " + msg[OWNET_PROT_RETVALUE]);                                
            }
        } while (msg[OWNET_PROT_PAYLOAD]!=0); // <0 = please wait, 0=end of list, >0 = we have data waiting (is this the way?)

        // close streams and connection
        // possible exception must be caught by the calling method
        in.close();
        out.close();
        ows.close();

        return retVal;
    }

     /**
     * Internal method to get directory list from server (single packet mode)
     * @param path directory to list
     * @throws java.io.IOException Exception to throw, when something is wrong
     * @return vector list of elements found
     */
    private Vector<String> OW_DirAll(String path) throws IOException 
    {
        Vector<String> retVal = new Vector<String>();
        Socket ows = null;
        DataInputStream in;
        DataOutputStream out;
        int[] msg = null;
        String[] values = null;
        
        // try to connect to remote server
        // possible exception must be caught by the calling method
        ows = new Socket(remoteServer, remotePort);
        in = new DataInputStream(ows.getInputStream());
        out = new DataOutputStream(ows.getOutputStream());
        
        sendPacket(out, OWNET_MSG_DIRALL, path.length()+1, 0);
        sendCString(out,path);
        
        msg = getPacket(in);
        if (msg[OWNET_PROT_RETVALUE] >= 0 )
        {
            if (msg[OWNET_PROT_PAYLOAD]>0){
                values = getPacketData(in,msg).split(",");
                for (int i=0; i<values.length; i++) retVal.add(values[i]);
            }            
        } else {
           in.close();
           out.close();
           ows.close();
           throw new IOException("Error getting Directory. Error " + msg[OWNET_PROT_RETVALUE]);                                
        }

        // close streams and connection
        // possible exception must be caught by the calling method
        in.close();
        out.close();
        ows.close();

        return retVal;
    }


//
// Public access methods
//

    /**
     * Read from server or throw IOException on error
     * @param path attribute's path
     * @throws java.io.IOException Exception to throw, when something is wrong
     * @return attribute's value
     */
    public String Read(String path) throws IOException{
        return OW_Read(path);
    }
    
    /**
     * Read from server and swallow any IOException
     * @param path attribute's path
     * @return attribute's value, empty on error
     */
    public String safeRead(String path){
        try {
            return OW_Read(path);
        } catch (IOException e) {
            return "";
        }
    }

    /**
     * Write to server or throw IOException on error
     * @param path attribute's path
     * @param value value to set
     * @return sucess state (most likely to be true or exception)
     * @throws java.io.IOException Exception to throw, when something is wrong
     */    
    public boolean Write(String path, String value) throws IOException{
        return OW_Write(path,value);
    }

    /**
     * Write to server and swallow any IOException
     * @param path attribute's path
     * @param value value to set
     * @return sucess state, false on error
     */    
    public boolean safeWrite(String path, String value){
        try {
            return OW_Write(path,value);
        } catch (IOException e) {
            return false;
        }
    }
        
    /**
     * Get presence state from server
     * @param path element to check
     * @throws java.io.IOException Exception to throw, when something is wrong
     * @return presence
     */    
    public boolean Presence(String path) throws IOException{
        return OW_Presence(path);
    }
    
    /**
     * Get presence state from server and swallow any IOException
     * @param path element to check
     * @return presence, false on missing or error
     */    
    public boolean safePresence(String path){
        try {
            return OW_Presence(path);
        } catch (IOException e) {
            return false;
        }
    }
    
    /**
     * Get directory list from server (multipacket mode)
     * @param path directory to list
     * @throws java.io.IOException Exception to throw, when something is wrong
     * @return vector list of elements found
     */
    public Vector<String> Dir(String path) throws IOException{
        return OW_Dir(path);
    }

    /**
     * Get directory list from server (multipacket mode) and swallow any IOException
     * @param path directory to list
     * @return vector list of elements found, empty on error
     */
    public Vector<String> safeDir(String path){
        try {
            return OW_Dir(path);
        } catch (IOException e) {
            return new Vector<String>();
        }
    }
    
    /**
     * Get directory list from server (singlepacket mode)
     * @param path directory to list
     * @throws java.io.IOException Exception to throw, when something is wrong
     * @return vector list of elements found
     */
    public Vector<String> DirAll(String path) throws IOException{
        return OW_DirAll(path);
    }

    /**
     * Get directory list from server (singlepacket mode) and swallow any IOException
     * @param path directory to list
     * @return vector list of elements found, empty on error
     */
    public Vector<String> safeDirAll(String path){
        try {
            return OW_DirAll(path);
        } catch (IOException e) {
            return new Vector<String>();
        }
    }
    
}
