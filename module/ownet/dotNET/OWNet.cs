 /*
  * C# class for accessing OWFS through owserver daemon over IP.
  * Created using SharpDevelop.
  * 
  *         Module: ownet/C#
  *      Namespace: org.owfs.ownet
  * Exported Class: OWNet
  *       Requires: .NET Framework 2.0
  *
  *  Author: George M. Zouganelis (gzoug@aueb.gr)
  * Version: $Id$
  *               OWNet.cs,v 1.3 2012/20/12 adho (adam.horvath@hotswap.eu)
  *                      Added: public byte[] safeReadRaw(String path)
  *                      And    public byte[] safeWriteRaw(String path)
  *
  * 
  * OWFS is an open source project developed by Paul Alfille 
  * and hosted at http://www.owfs.org
  * 
  * 
  * GPL
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

using System;
using System.Text;
using System.Net;
using System.Net.Sockets;

namespace org.owfs.ownet {

  public class OWNet {
  
  //
  // default values and constants
  //

	private const string OWNET_DEFAULT_HOST = "127.0.0.1";
	private const int OWNET_DEFAULT_PORT = 4304;

	private const int OWNET_DEFAULT_DATALEN = 4096;
	private const int OWNET_DEFAULT_TIMEOUT = 8000;

	private const int OWNET_MSG_ERROR = 0;
	private const int OWNET_MSG_NOP = 1;
	private const int OWNET_MSG_READ = 2;
	private const int OWNET_MSG_WRITE = 3;
	private const int OWNET_MSG_DIR = 4;
	private const int OWNET_MSG_SIZE = 5;
	private const int OWNET_MSG_PRESENCE = 6;
	private const int OWNET_MSG_DIRALL = 7;
	private const int OWNET_MSG_GET = 8;
	private const int OWNET_MSG_READ_ANY = 99999;

	private const int OWNET_PROT_STRUCT_SIZE = 6;
	private const int OWNET_PROT_VERSION = 0;
	private const int OWNET_PROT_PAYLOAD = 1;
	private const int OWNET_PROT_FUNCTION = 2;
	private const int OWNET_PROT_RETVALUE = 2;
	private const int OWNET_PROT_FLAGS = 3;
	private const int OWNET_PROT_DATALEN = 4;
	private const int OWNET_PROT_OFFSET = 5;

    // ################################
    // Device display format
    // ################################
    /**
     * format as f.i (10.67C6697351FF)
     */
	public const int OWNET_FLAG_D_F_I    = 0x0000000;
    /**
     * format as fi (1067C6697351FF)
     */
	public const int OWNET_FLAG_D_FI     = 0x1000000;
    /**
     * format as f.i.c (10.67C6697351FF.8D)
     */
	public const int OWNET_FLAG_D_F_I_C  = 0x2000000;
    /**
	 * format as f.ic (10.67C6697351FF8D)
     */
	public const int OWNET_FLAG_D_F_IC   = 0x3000000;
    /**
	 * format as fi.c (1067C6697351FF.8D)
     */
	public const int OWNET_FLAG_D_FI_C   = 0x4000000;
    /**
     * format as fic (1067C6697351FF8D)
     */
	public const int OWNET_FLAG_D_FIC    = 0x5000000;

    // ################################
    // temperature scale format
    // ################################
    /**
     * Temperature format: (C) Centigrade
     */
	public const int OWNET_FLAG_T_C      = 0x0000000;
    /**
     * Temperature format: (F) Fahrenheit
     */
	public const int OWNET_FLAG_T_F      = 0x0010000;
    /**
     * Temperature format: (K) Kelvin
     */
	public const int OWNET_FLAG_T_K      = 0x0020000;
    /**
     * Temperature format: (R) Rankine
     */
	public const int OWNET_FLAG_T_R      = 0x0030000;

	// Persistent connections
	public  const int OWNET_FLAG_PERSIST     = 0x0000004;

	
	private const int OWNET_DEFAULT_SG_FLAGS = 0x0000103;

  //
  // class private variables
  //
	private  string _remoteServer = OWNET_DEFAULT_HOST;          // remote server
	private     int _remotePort   = OWNET_DEFAULT_PORT;          // remote port
	private     int _sg_flags     = OWNET_DEFAULT_SG_FLAGS;      // formating flags + persistent bit
	private    bool _debugEnabled = false;                       // debug enable
	private     int _defaultTimeout = OWNET_DEFAULT_TIMEOUT;     // timeout for socket reads
	private     int _defaultDataLen = OWNET_DEFAULT_DATALEN;     // default data length for read/write commands
	private    bool _persistentConnection = true;

	// socket connection

	private Socket owsocket = null;
	private NetworkStream netStream = null;


  //
  // Constructors
  //

    /// <summary>
    /// Create a new instance of OWNet
    /// OWNet autoconnects to remote server when requested.
    /// Default server:port are localhost:4304 (127.0.0.1:4304).
    /// If others than default are requested,
    /// set them using the RemoteServer/RemotePort properties,
    /// before performing any read/write opertions to OW 
    /// or
    /// use the appropriate contructor OWNet(String server,int port)
    /// </summary>
    public OWNet() {
       // just a constructor
    }

    /// <summary>
    /// Create a new instance of OWNet and set server:port for future connections
    /// OWNet autoconnects to remote server when requested
    /// </summary>
    /// <param name="server">server ip or hostname of owserver to connect to</param>
    /// <param name="port">port remote's owserver port to connect to</param>
    public OWNet(String server, int port) {
		RemoteServer = server;
        RemotePort = port;
    }

  //
  // Class properties (attributes)
  //

    /// <summary>
    /// Enable/Disable Debug mode
    /// </summary>
    /// <value>bool</value>
	public bool Debug {
    	get {
    		return _debugEnabled;
    	}
    	set {
    		_debugEnabled = value;
    	}
    }


    /// <summary>
    /// Get/Set Remote server
    /// </summary>
    /// <value>String</value>
	public String RemoteServer{
    	get {
    		return _remoteServer;
    	}
    	set {
    		_remoteServer = value;
    	}
    }


    /// <summary>
    /// Get/Set Remote server's port
    /// </summary>
    /// <value>int</value>
    public int RemotePort{
    	get {
    		return _remotePort;
    	}
    	set {
    		_remotePort = value;
    	}
    }


    /// <summary>
    /// Get/Set default timeout for socket reads
    /// </summary>
    /// <value>int</value>
    public int DefaultTimeout {
    	get { 
    		return _defaultTimeout; 
    	}
    	set { 
    		 _defaultTimeout = value;
		     if (owsocket != null) 
		     	try {
    		 	   owsocket.ReceiveTimeout=_defaultTimeout;
		     	   //owsocket.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.SendTimeout, _defaultTimeout);
    		    } catch (Exception) {
		        }
    	}
    }

    /// <summary>
    /// Get/Set formating flags
    /// check http://www.owfs.org/index.php?page=owserver-flag-word for valid formats
    /// </summary>
    /// <value>int</value>
    public int Flags {
    	get {
    		return _sg_flags;
    	}
    	set {
    	    _sg_flags = value;
    	}
    }


    /// <summary>
    /// Get/Set default expected data length (payload) for receiving data 
    /// </summary>
    /// <value>int</value>
    public int DefaultDataLen {
    	get {
    		return _defaultDataLen;
    	}
    	set {
    		_defaultDataLen = value;
    	}
    }

    /// <summary>
    /// Get/Set persistent connection
    /// </summary>
    /// <value>bool</value>
    public bool PersistentConnection {
    	get {
    		return _persistentConnection;
    	}
    	set {
    		_persistentConnection = value;
    		if (_persistentConnection) {
    		    _sg_flags |= OWNET_FLAG_PERSIST;
    		} else {
    			_sg_flags &= (~OWNET_FLAG_PERSIST);
    		}
    		
    	}
    }

  //
  // Private methods for protocol and network access
  //

    /// <summary>
    /// Print ownet message packet to STDERR
    /// </summary>
    /// <param name="tag">tag to append</param>
    /// <param name="msg">message buffer to show</param>
    private void debugprint(String tag, int[] msg){
		if (!_debugEnabled) return;
		System.Console.Error.WriteLine("* OWNET.Java DEBUG [{0}] : ",tag);
		System.Console.Error.Write("version:{0}, ",msg[OWNET_PROT_VERSION]);
		System.Console.Error.Write("payload:{0}, ",msg[OWNET_PROT_PAYLOAD]);
		System.Console.Error.Write("function/ret:{0}, ",msg[OWNET_PROT_FUNCTION]);
		System.Console.Error.Write("flags:{0}, ",msg[OWNET_PROT_FLAGS]);
		System.Console.Error.Write("datalen:{0}, ",msg[OWNET_PROT_DATALEN]);
		System.Console.Error.WriteLine("offset/owtap flags:{0}",msg[OWNET_PROT_OFFSET]);
	}

	/// <summary>
	/// Print a message to STDERR
	/// </summary>
	/// <param name="tag">tag to append</param>
	/// <param name="message">message to show</param>
	private void debugprint(String tag, String message){
		if (!_debugEnabled) return;
		System.Console.Error.WriteLine("* OWNET.Java DEBUG [{0}] : ",tag);
		System.Console.Error.WriteLine(message);
    }


    /// <summary>
    /// Connect to remote host.
    /// If we have already connected in persistent mode, reconnection is discarded
    /// unless the remote has close the connection.
    /// If no previous connection has been made, or previous connection was not persistent,
    /// we follow a disconnect/connect sequence.
    /// </summary>
    private void Resetowsocket(){
    	  disconnect(true);
    	  owsocket = new Socket(AddressFamily.InterNetwork,SocketType.Stream,ProtocolType.Tcp);
		  owsocket.Blocking = true;
		  owsocket.ReceiveTimeout = _defaultTimeout;
		  //owsocket.SendBufferSize=this.DefaultDataLen;
		  //owsocket.ReceiveBufferSize=this.DefaultDataLen;
		  owsocket.Connect(RemoteServer, RemotePort);
		  netStream = new NetworkStream(owsocket,System.IO.FileAccess.ReadWrite);
    }
    
    /// <summary>
    /// Connect to remote server
    /// </summary>
	private void connect()/* throws IOException */{
	   if (!_persistentConnection) {
		  // make sure we have cleared old connection objects
		  debugprint("CONNECT","!persistentConnection, Initializing connection");
		  Resetowsocket();
		  
	   } else if (owsocket!=null) {
		  // if we have a connection object and it's closed, reopen it.
		  debugprint("CONNECT","persistentConnection\n");
		  if (!owsocket.Connected) {
		  	debugprint("CONNECT","reconnecting to " + RemoteServer );
		  	owsocket.Connect(RemoteServer, RemotePort);
          }
       } else {
          // should never been here (persistent connection & no connection object)
          debugprint("CONNECT","persistentConnection, Initializing connection");
          Resetowsocket();
         
		}
    }

    /// <summary>
    /// Disconnect from remote host
    /// On persistent connections, disconnection is silently discarder, unless force parameter is set
    /// </summary>
    /// <param name="force">force a disconnect, even in persistent mode</param>
	private void disconnect(bool force) /* throws IOException */ {
        if ((!_persistentConnection) || force) {
          debugprint("DISCONNECT","Cleaning up connection");
          if (netStream != null) netStream.Close() ; netStream = null;
		  if (owsocket != null) owsocket.Close();  owsocket = null;
        }
    }


    /// <summary>
    /// Send a ownet message packet
    /// </summary>
    /// <param name="function">ownet's functon header field</param>
    /// <param name="payloadlen">ownet's payload header field</param>
    /// <param name="datalen">ownet's datalen header field</param>
	private void sendPacket(int function, int payloadlen, int datalen) /* throws IOException */
    {
        int[] msg = new int[OWNET_PROT_STRUCT_SIZE];
        byte[] buf = new byte[4];
        int i;
        msg[OWNET_PROT_VERSION] = 0;
        msg[OWNET_PROT_PAYLOAD] = payloadlen;
        msg[OWNET_PROT_FUNCTION] = function;
        msg[OWNET_PROT_FLAGS] = _sg_flags | OWNET_FLAG_PERSIST;
        msg[OWNET_PROT_DATALEN] = datalen;
        msg[OWNET_PROT_OFFSET] = 0;
        debugprint("sendPacket", msg);
        try {		  
		   for ( i = 0; i<OWNET_PROT_STRUCT_SIZE; i++) 
		   {
        	 buf = System.BitConverter.GetBytes(msg[i]);
        	 Array.Reverse(buf); //LittleEndian to BigEndian
		   	 netStream.Write(buf,0,4);
		   	 netStream.Flush();
		   }
		} catch (Exception) {
		   // if persistent connection has timeout
			if (_persistentConnection) {
				disconnect(true);
				connect();
				try {
					for ( i = 0; i<OWNET_PROT_STRUCT_SIZE; i++) 
					{
					  netStream.Write(buf,0,4);
					  netStream.Flush();
					}
				} catch (Exception ee) {
                    throw ee;
                }
           }
        }

    }

    /// <summary>
    /// Get a ownet message packet
    /// </summary>
    /// <returns>the ownet's received header packet from server</returns>
	private int[] getPacket() /* throws IOException */
    {
		byte[] buf = new byte[4];
		int[] msg = new int[OWNET_PROT_STRUCT_SIZE];
        for (int i = 0; i<OWNET_PROT_STRUCT_SIZE; i++)
        {
		  netStream.Read(buf,0,4);
		  Array.Reverse(buf); // BigEndian to LittleEndian
          msg[i] = BitConverter.ToInt32(buf,0);
        }
        _persistentConnection = ((msg[OWNET_PROT_FLAGS] & OWNET_FLAG_PERSIST) == OWNET_FLAG_PERSIST);
        debugprint("getPacket (Server Persistance support: "+_persistentConnection+")", msg);
        return msg;
    }

    /// <summary>
    /// Get a data packet from remote server
    /// </summary>
    /// <param name="packetHeader">ownet's header msg packet to use for receiving auxilary data</param>
    /// <returns>data received from remote server</returns>
	private String getPacketData(int[] packetHeader) /* throws IOException */
	{
        byte[] data = new byte[packetHeader[OWNET_PROT_PAYLOAD]];
		netStream.Read(data,0,packetHeader[OWNET_PROT_PAYLOAD]);			
		String retVal = new String(Encoding.ASCII.GetChars(data,0 /*packetHeader[OWNET_PROT_OFFSET]*/,packetHeader[OWNET_PROT_PAYLOAD]));
		return retVal;
    }

    /// <summary>
    /// Get a data packet from remote server
    /// </summary>
    /// <param name="packetHeader">ownet's header msg packet to use for receiving auxilary data</param>
    /// <returns>data received from remote server</returns>
    private byte[] getPacketDataRaw(int[] packetHeader) /* throws IOException */
    {
        byte[] data = new byte[packetHeader[OWNET_PROT_PAYLOAD]];
        netStream.Read(data, 0, packetHeader[OWNET_PROT_PAYLOAD]);
        return data;
    }

    /// <summary>
    /// Transmit a string as null terminated
    /// </summary>
    /// <param name="str">String to transmit</param>
    private void sendCString(String str) /* throws IOException */
    {	
        byte[] buf = Encoding.ASCII.GetBytes(str);
        netStream.Write(buf,0,buf.Length);
        netStream.WriteByte(0);
        netStream.Flush();
    }

    /// <summary>
    /// Transmit a string as null terminated
    /// </summary>
    /// <param name="str">String to transmit</param>
    private void sendRaw(byte[] buf) /* throws IOException */
    {
        netStream.Write(buf, 0, buf.Length);
        netStream.WriteByte(0);
        netStream.Flush();
    }

    /// <summary>
    /// Internal method to read from server
    /// </summary>
    /// <param name="path">attribute's path</param>
    /// <param name="expectedDataLen">expected length of data to receive</param>
    /// <returns>attribute's value</returns>
	private String OW_Read(String path, int expectedDataLen) /* throws IOException */
    {
        String retVal = "";
        int[] msg = null;

        // try to connect to remote server
        // possible exception must be caught by the calling method
        connect();
        sendPacket(OWNET_MSG_READ, path.Length+1, expectedDataLen);
        sendCString(path);
        msg = getPacket();
        if (msg[OWNET_PROT_RETVALUE] >= 0) 
        {
            if (msg[OWNET_PROT_PAYLOAD]>=0) retVal = getPacketData(msg);
        } else {
          disconnect(false);
          throw new Exception("Error reading from server. Error " + msg[OWNET_PROT_RETVALUE]);
        }
        // close streams and connection
        // possible exception must be caught by the calling method
        disconnect(false);
        return retVal;

    }

    /// <summary>
    /// Internal method to read from server
    /// </summary>
    /// <param name="path">attribute's path</param>
    /// <param name="expectedDataLen">expected length of data to receive</param>
    /// <returns>attribute's value</returns>
    private byte[] OW_ReadRaw(String path, int expectedDataLen) /* throws IOException */
    {
        byte[] retVal = null;
        int[] msg = null;

        // try to connect to remote server
        // possible exception must be caught by the calling method
        connect();
        sendPacket(OWNET_MSG_READ, path.Length + 1, expectedDataLen);
        sendCString(path);
        msg = getPacket();
        if (msg[OWNET_PROT_RETVALUE] >= 0)
        {
            if (msg[OWNET_PROT_PAYLOAD] >= 0) retVal = getPacketDataRaw(msg);
        }
        else
        {
            disconnect(false);
            throw new Exception("Error reading from server. Error " + msg[OWNET_PROT_RETVALUE]);
        }
        // close streams and connection
        // possible exception must be caught by the calling method
        disconnect(false);
        return retVal;
    }

	/// <summary>
    /// Internal method to read from server, using default excpected datalen
    /// </summary>
    /// <param name="path">attribute's path</param>
    /// <returns>attribute's value</returns>
	private String OW_Read(String path) /* throws IOException */
    {
        return OW_Read(path, _defaultDataLen);
    }

    /// <summary>
    /// Internal method to read from server, using default excpected datalen
    /// </summary>
    /// <param name="path">attribute's path</param>
    /// <returns>attribute's value</returns>
    private byte[] OW_ReadRaw(String path) /* throws IOException */
    {
        return OW_ReadRaw(path, _defaultDataLen);
    }



 
    /// <summary>
    /// Internal method to write to server
    /// </summary>
    /// <param name="path">attribute's path</param>
    /// <param name="value">value to set</param>
    /// <returns>sucess state (most likely to be true or exception)</returns>
    private bool OW_Write(String path, String value) /* throws IOException */
    {
        bool retVal = false;
        int[] msg = null;

        // try to connect to remote server
        // possible exception must be caught by the calling method
        connect();

        sendPacket(OWNET_MSG_WRITE, path.Length + 1 + value.Length + 1, value.Length + 1);
        sendCString(path);
        sendCString(value);
        msg = getPacket();

        if (msg[OWNET_PROT_RETVALUE] >= 0) 
        {
            retVal = true;
        } else {
            disconnect(false);
            throw new Exception("Error writing to server. Error " + msg[OWNET_PROT_RETVALUE]);
        }

        // close streams and connection
        // possible exception must be caught by the calling method
        disconnect(false);
        return retVal;
    }

    /// <summary>
    /// Internal method to write to server
    /// </summary>
    /// <param name="path">attribute's path</param>
    /// <param name="value">value to set</param>
    /// <returns>sucess state (most likely to be true or exception)</returns>
    private bool OW_WriteRaw(String path, byte[] value) /* throws IOException */
    {
        bool retVal = false;
        int[] msg = null;

        // try to connect to remote server
        // possible exception must be caught by the calling method
        connect();

        sendPacket(OWNET_MSG_WRITE, path.Length + 1 + value.Length + 1, value.Length + 1);
        sendCString(path);
        sendRaw(value);
        msg = getPacket();

        if (msg[OWNET_PROT_RETVALUE] >= 0)
        {
            retVal = true;
        }
        else
        {
            disconnect(false);
            throw new Exception("Error writing to server. Error " + msg[OWNET_PROT_RETVALUE]);
        }

        // close streams and connection
        // possible exception must be caught by the calling method
        disconnect(false);
        return retVal;
    }


    /// <summary>
    /// Internal method to get presence state from server
    /// </summary>
    /// <param name="path">path element to check</param>
    /// <returns>presence</returns>
	private bool OW_Presence(String path) /* throws IOException */
    {
        bool retVal = false;
        int[] msg = null;

        // try to connect to remote server
        // possible exception must be caught by the calling method
        connect();

        sendPacket(OWNET_MSG_PRESENCE, path.Length + 1, 0);
        sendCString(path);
        msg = getPacket();
        if (msg[OWNET_PROT_RETVALUE] >= 0 ) 
        {
          retVal = true;
        } else {
          disconnect(false);
          throw new Exception(path + " not found. Error " + msg[OWNET_PROT_RETVALUE]);
        }
        // close streams and connection
        // possible exception must be caught by the calling method
        disconnect(false);
        return retVal;
    }


    /// <summary>
    /// Internal method to get directory list from server (multipacket mode)
    /// </summary>
    /// <param name="path">directory to list</param>
    /// <param name="dirall">request a 'dirall' command</param>
    /// <returns>list of elements found></returns>
	private String[] OW_Dir(String path, bool dirall) /* throws IOException */
    {
        StringBuilder retVals = new StringBuilder();
		int[] msg = null;

        // try to connect to remote server
        // possible exception must be caught by the calling method
        connect();
        if (dirall) 
        {
          sendPacket(OWNET_MSG_DIRALL, path.Length + 1, 0);
        } else {
          sendPacket(OWNET_MSG_DIR, path.Length + 1, 0);
        }
        sendCString(path);
        do 
        {
          msg = getPacket();
          if (msg[OWNET_PROT_RETVALUE] >= 0)
          {
            if (msg[OWNET_PROT_PAYLOAD] > 0)  
            {
               if (!dirall) 
               {
                 if (retVals.Length>0) retVals.Append(',');
                 retVals = retVals.Append(getPacketData(msg));
               } else {
                 retVals.Append(getPacketData(msg));
               }
            }
          } else {
            disconnect(false);
            throw new Exception("Error getting Directory. Error " + msg[OWNET_PROT_RETVALUE]);
          }
        } while (!dirall && (msg[OWNET_PROT_PAYLOAD]!=0) ); // <0 = please wait, 0=end of list, >0 = we have data waiting (is this the way?)

        // close streams and connection
        // possible exception must be caught by the calling method
        disconnect(false);
        return retVals.ToString().Split(',');
    }

  //
  // Public access methods
  //
    /// <summary>
    /// Connect to remote host
    /// </summary>
	public void Connect() /* throws IOException */
	{
        connect();
    }

    
    /// <summary>
    /// Connect to remote host and swallow any Exception
    /// </summary>
    /// <returns>true on successful connection </returns>
    public bool safeConnect() 
    {
        try { connect(); } catch (Exception) {return false;}
        return true;
    }


    /// <summary>
    /// Disconnect from remote host.
    /// Note: On clean-up, you should always try a Disconnect
    /// </summary>
	public void Disconnect() /* throws IOException */
	{
        disconnect(true);
    }

    /// <summary>
    /// Disconnect from remote host and swallow any IOException
    /// Note: Before freeing OWNet, you should always be polite and try a Disconnect to honor remote server
    /// </summary>
    /// <returns>true on successful disconnect</returns>
    public bool safeDisconnect()
    {
        try { disconnect(true); } catch (Exception) {return false;}
        return true;
    }


    /// <summary>
    /// Read from server or throw IOException on error
    /// </summary>
    /// <param name="path">attribute's path</param>
    /// <returns>attribute's value</returns>
	public String Read(String path) /* throws IOException */
	{
        return OW_Read(path);
    }

    /// <summary>
    /// Read from server and swallow any IOException
    /// </summary>
    /// <param name="path">attribute's path</param>
    /// <returns>attribute's value, empty on error</returns>
    public String safeRead(String path)
    {
      try {
        return OW_Read(path);
      } catch (Exception) {
        return "";
      }
    }

    /// <summary>
    /// Read from server or throw IOException on error
    /// </summary>
    /// <param name="path">attribute's path</param>
    /// <returns>attribute's value</returns>
    public byte[] ReadRaw(String path) /* throws IOException */
    {
        return OW_ReadRaw(path);
    }

    /// <summary>
    /// Read from server and swallow any IOException
    /// </summary>
    /// <param name="path">attribute's path</param>
    /// <returns>attribute's value, empty on error</returns>
    public byte[] safeReadRaw(String path)
    {
        try
        {
            return OW_ReadRaw(path);
        }
        catch (Exception)
        {
            return null;
        }
    }


    /// <summary>
    /// Write to server or throw IOException on error
    /// </summary>
    /// <param name="path">attribute's path</param>
    /// <param name="value">value to set</param>
    /// <returns>sucess state (most likely to be true or exception)</returns>
    public bool Write(String path, String value) /* throws IOException */
    {
        return OW_Write(path,value);
    }


    /// <summary>
    /// Write to server or throw IOException on error
    /// </summary>
    /// <param name="path">attribute's path</param>
    /// <param name="value">value to set</param>
    /// <returns>sucess state (most likely to be true or exception)</returns>
    public bool WriteRaw(String path, byte[] value) /* throws IOException */
    {
        return OW_WriteRaw(path, value);
    }

    /// <summary>
    /// Write to server and swallow any IOException
    /// </summary>
    /// <param name="path">attribute's path</param>
    /// <param name="value">value to set</param>
    /// <returns>sucess state, false on error</returns>
    public bool safeWrite(String path, String value){
        try {
            return OW_Write(path,value);
        } catch (Exception) {
            return false;
        }
    }

    /// <summary>
    /// Write to server and swallow any IOException
    /// </summary>
    /// <param name="path">attribute's path</param>
    /// <param name="value">value to set</param>
    /// <returns>sucess state, false on error</returns>
    public bool safeWriteRaw(String path, byte[] value)
    {
        try
        {
            return OW_WriteRaw(path, value);
        }
        catch (Exception)
        {
            return false;
        }
    }

    /// <summary>
    /// Get presence state from server
    /// </summary>
    /// <param name="path">element to check</param>
    /// <returns>presence</returns>
	public bool Presence(String path) /* throws IOException */
	{
        return OW_Presence(path);
    }

    /// <summary>
    /// Get presence state from server and swallow any IOException
    /// </summary>
    /// <param name="path">element to check</param>
    /// <returns>presence, false on missing or error</returns>
    public bool safePresence(String path){
        try {
          return OW_Presence(path);
        } catch (Exception) {
          return false;
        }
    }

    /// <summary>
    /// Get directory list from server (multipacket mode)
    /// </summary>
    /// <param name="path">directory to list</param>
    /// <returns>array of elements found</returns>
	public String[] Dir(String path) /* throws IOException */
	{
        return OW_Dir(path,false);
    }


    /// <summary>
    /// Get directory list from server (multipacket mode) and swallow any IOException
    /// </summary>
    /// <param name="path">directory to list</param>
    /// <returns>array of elements found, empty on error</returns>
    public String[] safeDir(String path){
        String[] retVal = {};
        try {
          retVal =  OW_Dir(path,false);
        } catch (Exception) { }
        return retVal;
    }

    /// <summary>
    /// Get directory list from server (singlepacket mode)
    /// </summary>
    /// <param name="path">directory to list</param>
    /// <returns>array of elements found</returns>
	public String[] DirAll(String path) /* throws IOException */
	{
        return OW_Dir(path,true);
    }

    /// <summary>
    /// Get directory list from server (singlepacket mode) and swallow any IOException
    /// </summary>
    /// <param name="path">directory to list</param>
    /// <returns>array of elements found, empty on error</returns>
    public String[]  safeDirAll(String path){
        String[] retVal = {};
        try {
            retVal = OW_Dir(path,true);
		} catch (Exception) { }
        return retVal;
    }

  } // End of OWNet class definition

}


