 /**
  * Module: ownet/java 
  * Package: org.owfs.ownet.OWNet
  *
  * @author George M. Zouganelis (gzoug@aueb.gr)
  * @version $Id$
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
  */

package org.owfs.ownet;

import java.io.*;
import java.net.*;
import java.util.Vector;


/**
 * Java class for accessing OWFS through owserver daemon over IP.
 *
 * OWFS is an open source project developed by Paul Alfille and hosted at
 * http://www.owfs.org
 *
 */


/* ---------------------------------------------------------------------
 * Changes:
 * ---------------------------------------------------------------------
 * 2007/01/12: initial commit
 * 2007/01/28: Persistent connection support
 *             Connect/Disconnect introduced
 *             fix for large packet socket reads, timeout added
 */

public class OWNet {
//
// default values and constants
//

    static private final String OWNET_DEFAULT_HOST = "127.0.0.1";
    //IANA  #50010 Assigned default port number (http://www.iana.org/assignments/port-numbers)
    //owserver        4304/tcp   One-Wire Filesystem Server
    //owserver        4304/udp   One-Wire Filesystem Server
    static private final int OWNET_DEFAULT_PORT = 4304;

    static private final int OWNET_DEFAULT_DATALEN = 4096;
    static private final int OWNET_DEFAULT_TIMEOUT = 8000;

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

    // ################################
    // Device display format
    // ################################
    /**
     * format as f.i (10.67C6697351FF)
     */
    static public final int OWNET_FLAG_D_F_I    = 0x0000000; 
    /**
     * format as fi (1067C6697351FF)
     */
    static public final int OWNET_FLAG_D_FI     = 0x1000000; 
    /**
     * format as f.i.c (10.67C6697351FF.8D)
     */
    static public final int OWNET_FLAG_D_F_I_C  = 0x2000000; 
    /**
     * format as f.ic (10.67C6697351FF8D)
     */
    static public final int OWNET_FLAG_D_F_IC   = 0x3000000; 
    /**
     * format as fi.c (1067C6697351FF.8D)
     */
    static public final int OWNET_FLAG_D_FI_C   = 0x4000000; 
    /**
     * format as fic (1067C6697351FF8D)
     */
    static public final int OWNET_FLAG_D_FIC    = 0x5000000; 

    // ################################
    // temperature scale format
    // ################################
    /**
     * Temperature format: (C) Centigrade
     */
    static public final int OWNET_FLAG_T_C      = 0x0000000; 
    /**
     * Temperature format: (F) Fahrenheit
     */
    static public final int OWNET_FLAG_T_F      = 0x0010000; 
    /**
     * Temperature format: (K) Kelvin
     */
    static public final int OWNET_FLAG_T_K      = 0x0020000; 
    /**
     * Temperature format: (R) Rankine
     */
    static public final int OWNET_FLAG_T_R      = 0x0030000; 

    
    
    static private final int OWNET_DEFAULT_SG_FLAGS = 0x0000103;
    static private final int OWNET_FLAG_PERSIST     = 0x0000004; // for Persistent connections
//
// class private variables
//
    private  String remoteServer = OWNET_DEFAULT_HOST;          // remote server
    private     int remotePort   = OWNET_DEFAULT_PORT;          // remote port
    private     int sg_flags     = OWNET_DEFAULT_SG_FLAGS;      // formating flags + persistent bit
    private boolean debugEnabled = false;                       // debug enable
    private     int conn_timeout = OWNET_DEFAULT_TIMEOUT;       // timeout for socket reads
    private     int defDataLen   = OWNET_DEFAULT_DATALEN;       // default data length for read/write commands
    
    // socket connection
    
    private Socket owsocket = null;
    private DataInputStream in = null;
    private DataOutputStream out = null;
    private boolean persistentConnection = false;

    
    
//
// Constructors
//

    /** 
     * Create a new instance of OWNet 
     * OWNet autoconnects to remote server when requested
     * Default server:port are localhost:4304 (127.0.0.1:4304).
     * If others than default are requested,
     * set them using the setters setRemoteServer/setRemotePort, 
     * before performing any read/write opertions to OW.
     */
    public OWNet() {
       // just a constructor
    }

    /**
     * Create a new instance of OWNet and set server:port for future connections
     * OWNet autoconnects to remote server when requested
     * 
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
        return debugEnabled;
    }
    
   
    /**
     * Set debugging mode (default=false)
     * @param newstate enable/disable debug mode
     */
    public void setDebug(boolean newstate) {
        debugEnabled = newstate;
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
     * Get default timeout for socket reads
     * @return timeout
     */
    public int getDefaultTimeout() {
        return conn_timeout;
    }

    /**
     * Set default timeout for socket reads
     * @param mils timeout in msec for socket reads
     */
    public void setDefaultTimeout(int mils) {
        conn_timeout = mils;
        if (owsocket != null) try { owsocket.setSoTimeout(mils); } catch (Exception E) {}
    }


    /**
     * Get formating flags
     * @return formating flags (check http://www.owfs.org/index.php?page=owserver-flag-word)
     */
    public int getFormatflags() {
        return sg_flags;
    }

    /**
     * Set formating flags
     * @param newformatflags formating flags (check http://www.owfs.org/index.php?page=owserver-flag-word)
     */
    public void setFormatflags(int newformatflags) {
        sg_flags = newformatflags;
    }

    /**
     * Get default expected data length for receiving data
     * @return default payload
     */
    public int getDefaultDataLen() {
        return defDataLen;
    }

    /**
     * Set default excpected data length for receiving data
     * @param newDefaultDataLen default length of data to expect
     */
    public void setDefaultDataLen(int newDefaultDataLen) {
        defDataLen = newDefaultDataLen;
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
        if (!debugEnabled) return;
        System.err.print("* OWNET.Java DEBUG [" + tag + "] : ");
        System.err.print("version:"+msg[OWNET_PROT_VERSION]+", ");
        System.err.print("payload:"+msg[OWNET_PROT_PAYLOAD]+", ");
        System.err.print("function/ret:"+msg[OWNET_PROT_FUNCTION]+", ");
        System.err.print("flags:"+msg[OWNET_PROT_FLAGS]+", ");
        System.err.print("datalen:"+msg[OWNET_PROT_DATALEN]+", ");
        System.err.print("offset:"+msg[OWNET_PROT_OFFSET]+"\n");
        System.err.flush();
    }

    /**
     * Print a message to STDERR
     * @param tag tag to append
     * @param message message to show
     */
    private void debugprint(String tag, String message){
        if (!debugEnabled) return;
        System.err.print("* OWNET.Java DEBUG [" + tag + "] : ");
        System.err.print(message);
        System.err.flush();
    }
    
    
    /**
     * Connect to remote host.
     * If we have already connected in persistent mode, reconnection is discarded 
     * unless the remote has close the connection. 
     * If no previous connection has been made, or previous connection was not persistent, 
     * we follow a disconnect/connect sequence.
     * @throws java.io.IOException Exception to throw on connection error
     */
    private void connect() throws IOException {
       if (!persistentConnection) {
          // make sure we have cleared old connection objects
          debugprint("CONNECT","!persistentConnection\n");
          if (owsocket!=null) disconnect(true);  
          debugprint("CONNECT","Initializing connection\n");          
          owsocket = new Socket(remoteServer, remotePort);
          owsocket.setSoTimeout(conn_timeout);
          in = new DataInputStream(owsocket.getInputStream());
          out = new DataOutputStream(owsocket.getOutputStream());
       } else if (owsocket!=null) {
          // if we have a connection object and it's closed, reopen it.
          debugprint("CONNECT","persistentConnection\n");
          if (owsocket.isClosed()) {
              debugprint("CONNECT","reconnecting to " + owsocket.getRemoteSocketAddress()+"\n");
              owsocket.connect(owsocket.getRemoteSocketAddress());
          } 
       } else {
          // should never been here (persistent connection & no connection object)
          debugprint("CONNECT","persistentConnection\n");
          if (owsocket!=null) disconnect(true);  
          debugprint("CONNECT","Initializing connection\n");          
          owsocket = new Socket(remoteServer, remotePort);
          owsocket.setSoTimeout(conn_timeout);
          in = new DataInputStream(owsocket.getInputStream());
          out = new DataOutputStream(owsocket.getOutputStream());          
        }
    }

    /**
     * Disconnect from remote host
     * On persistent connections, disconnection is silently discarder, unless force parameter is set
     * @param force force a disconnect, even in persistent mode
     * @throws java.io.IOException Exception to throw on disconnection error
     */
    private void disconnect(boolean force) throws IOException {
        if ((!persistentConnection) || force) {
          debugprint("DISCONNECT","Cleaning up connection\n");          
          persistentConnection = false;
          if (in != null) in.close(); in = null;
          if (out != null) out.close(); out = null;
          if (owsocket != null) owsocket.close();  owsocket = null;
          debugprint("DISCONNECT","Done.\n");          
        }
    }
    
    /**
     * Connect to remote host
     * @throws java.io.IOException Exception to throw on connection error
     */
    public void Connect() throws IOException {
        connect();
    }
    
    /**
     * Connect to remote host and swallow any IOException
     * @return success
     */
    public boolean safeConnect() {
        try { connect(); } catch (IOException e) {return false;}
        return true;
    }
   
    
    
    /**
     * Disconnect from remote host. 
     * Note: On clean-up, you should always try a Disconnect
     * @throws java.io.IOException Exception to throw on disconnection error
     */
    public void Disconnect() throws IOException {
        disconnect(true);
    }
    
    /**
     * Disconnect from remote host and swallow any IOException
     * Note: Before freeing OWNet, you should always be polite and try a Disconnect to honor remote server
     * @return success
     */
    public boolean safeDisconnect()  {
        try { disconnect(true); } catch (IOException e) {return false;}
        return true;
    }

    
    
    /**
     * Send a ownet message packet
     * @param function ownet's header field
     * @param payloadlen ownet's header field
     * @param datalen ownet's header field
     * @throws java.io.IOException Exception to throw, when something is wrong
     */
    private void sendPacket(int function, int payloadlen, int datalen) throws IOException
    {
        int[] msg = new int[OWNET_PROT_STRUCT_SIZE];
        byte[] b;
        int i;
        msg[OWNET_PROT_VERSION] = 0;
        msg[OWNET_PROT_PAYLOAD] = payloadlen;
        msg[OWNET_PROT_FUNCTION] = function;
        msg[OWNET_PROT_FLAGS] = sg_flags | OWNET_FLAG_PERSIST;
        msg[OWNET_PROT_DATALEN] = datalen;
        msg[OWNET_PROT_OFFSET] = 0;
        debugprint("sendPacket", msg);
        try {
           for ( i = 0; i<OWNET_PROT_STRUCT_SIZE; i++) out.writeInt(msg[i]);
        } catch (IOException e) { // if timeout (maybe!)
           if (persistentConnection) {               
                disconnect(true);
                connect();
                try {
                  for ( i = 0; i<OWNET_PROT_STRUCT_SIZE; i++) out.writeInt(msg[i]);
                } catch (IOException ee) {
                    throw ee;
                }
           }
        }
       
    }

    /**
     * Get a ownet message packet
     * @return the ownet's received header packet from server
     * @throws java.io.IOException Exception to throw, when something is wrong
     */
    private int[] getPacket() throws IOException
    {
        int[] msg = new int[OWNET_PROT_STRUCT_SIZE];
        for (int i = 0; i<OWNET_PROT_STRUCT_SIZE; i++){ 
            msg[i] = in.readInt();
        }   
        
        persistentConnection = ((msg[OWNET_PROT_FLAGS] & OWNET_FLAG_PERSIST) == OWNET_FLAG_PERSIST);
        debugprint("getPacket (Persistent="+persistentConnection+")", msg);        
        return msg;
    }

    /**
     * Get a data packet from remote server
     * @param packetHeader ownet's header msg packet to use for receiving auxilary data
     * @throws java.io.IOException Exception to throw, when something is wrong
     * @return data received from remote server
     */
    private String getPacketData(int[] packetHeader) throws IOException {
        byte[] data = new byte[packetHeader[OWNET_PROT_PAYLOAD]];
        String retVal = null;
        in.readFully(data,0,data.length);
        retVal = new String(data,packetHeader[OWNET_PROT_OFFSET],packetHeader[OWNET_PROT_DATALEN]);
        return retVal;
    }

    /**
     * Transmit a Java String as a C-String (null terminated)
     * @param str string to transmit
     * @throws java.io.IOException Exception to throw, when something is wrong
     */
    private void sendCString(String str) throws IOException {
       out.write(str.getBytes());out.writeByte(0);
       //out.flush();
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
        int[] msg = null;

        // try to connect to remote server
        // possible exception must be caught by the calling method
        connect();
        sendPacket(OWNET_MSG_READ, path.length()+1, expectedDataLen);
        sendCString(path);
        msg = getPacket();
        if (msg[OWNET_PROT_RETVALUE] >= 0) {
            if (msg[OWNET_PROT_PAYLOAD]>=0) retVal = getPacketData(msg);
        } else {
          disconnect(false);
          throw new IOException("Error reading from server. Error " + msg[OWNET_PROT_RETVALUE]);
        }
        // close streams and connection
        // possible exception must be caught by the calling method
        disconnect(false);
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
        return OW_Read(path,defDataLen);
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
        int[] msg = null;

        // try to connect to remote server
        // possible exception must be caught by the calling method
        connect();

        sendPacket(OWNET_MSG_WRITE, path.length() + 1 + value.length() + 1, value.length() + 1);
        sendCString(path);
        sendCString(value);
        msg = getPacket();

        if (msg[OWNET_PROT_RETVALUE] >= 0) {
            retVal = true;
        } else {
            disconnect(false);
            throw new IOException("Error writing to server. Error " + msg[OWNET_PROT_RETVALUE]);
        }

        // close streams and connection
        // possible exception must be caught by the calling method
        disconnect(false);
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
        int[] msg = null;

        // try to connect to remote server
        // possible exception must be caught by the calling method
        connect();

        sendPacket(OWNET_MSG_PRESENCE, path.length()+1, 0);
        sendCString(path);
        msg = getPacket();
        if (msg[OWNET_PROT_RETVALUE] >= 0 ) {
            retVal = true;
        } else {
            disconnect(false);
            throw new IOException(path + " not found. Error " + msg[OWNET_PROT_RETVALUE]);
        }

        // close streams and connection
        // possible exception must be caught by the calling method
        disconnect(false);
        return retVal;
    }


    /**
     * Internal method to get directory list from server (multipacket mode)
     * @param path directory to list
     * @return vector list of elements found
     * @throws java.io.IOException Exception to throw, when something is wrong
     */
    private Vector<String> OW_Dir(String path) throws IOException
    {
        Vector<String> retVal = new Vector<String>();
        int[] msg = null;

        // try to connect to remote server
        // possible exception must be caught by the calling method
        connect();
        sendPacket(OWNET_MSG_DIR, path.length()+1, 0);
        sendCString(path);

        do{
            msg = getPacket();
            if (msg[OWNET_PROT_RETVALUE] >= 0){
               if (msg[OWNET_PROT_PAYLOAD] > 0)  retVal.add(getPacketData(msg));
            } else {
               disconnect(false);
               throw new IOException("Error getting Directory. Error " + msg[OWNET_PROT_RETVALUE]);
            }
        } while (msg[OWNET_PROT_PAYLOAD]!=0); // <0 = please wait, 0=end of list, >0 = we have data waiting (is this the way?)

        // close streams and connection
        // possible exception must be caught by the calling method
        disconnect(false);
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
        int[] msg = null;
        String[] values = null;

        // try to connect to remote server
        // possible exception must be caught by the calling method
        connect();

        sendPacket(OWNET_MSG_DIRALL, path.length()+1, 0);
        sendCString(path);

        msg = getPacket();
        if (msg[OWNET_PROT_RETVALUE] >= 0 )
        {
            if (msg[OWNET_PROT_PAYLOAD]>0){
                values = getPacketData(msg).split(",");
                for (int i=0; i<values.length; i++) retVal.add(values[i]);
            }
        } else {
           disconnect(false);
           throw new IOException("Error getting Directory. Error " + msg[OWNET_PROT_RETVALUE]);
        }

        // close streams and connection
        // possible exception must be caught by the calling method
        disconnect(false);
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
