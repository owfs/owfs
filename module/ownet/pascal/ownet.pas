Unit ownet;
 (**
  * Module: ownet/pascal
  *
  * Author:  George M. Zouganelis (gzoug@aueb.gr)
  * Version: $Id$
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
  *)

{$IFNDEF FPC}
{$ELSE}
  {$mode objfpc}{$H+}
{$ENDIF}

Interface

Uses
  Classes,  blcksock;
Const

    // ################################
    // Device display format
    // ################################

    // format as f.i (10.67C6697351FF)
    OWNET_FLAG_D_F_I    = $0000000;
    // format as fi (1067C6697351FF)
    OWNET_FLAG_D_FI     = $1000000;
    // format as f.i.c (10.67C6697351FF.8D)
    OWNET_FLAG_D_F_I_C  = $2000000;
    // format as f.ic (10.67C6697351FF8D)
    OWNET_FLAG_D_F_IC   = $3000000;
    // format as fi.c (1067C6697351FF.8D)
    OWNET_FLAG_D_FI_C   = $4000000;
    // format as fic (1067C6697351FF8D)
    OWNET_FLAG_D_FIC    = $5000000;

    // ################################
    // temperature scale format
    // ################################
    // Temperature format: (C) Centigrade
    OWNET_FLAG_T_C = $0000000;
    // Temperature format: (F) Fahrenheit
    OWNET_FLAG_T_F = $0010000;
    // Temperature format: (K) Kelvin
    OWNET_FLAG_T_K = $0020000;
    // Temperature format: (R) Rankine
    OWNET_FLAG_T_R = $0030000;

    // ################################
    // pressure scale format
    // ################################
    // Pressure format: mbar
    OWNET_FLAG_P_mbar = $0000000;
    // Pressure format: atm
    OWNET_FLAG_P_atm  = $0040000;
    // Pressure format: mmHg
    OWNET_FLAG_P_mmHg = $0080000;
    // Pressure format: inHg
    OWNET_FLAG_P_inHg = $00C0000;
    // Pressure format: psi
    OWNET_FLAG_P_psi  = $0100000;
    // Pressure format: Pa
    OWNET_FLAG_P_Pa   = $0140000;

    OWNET_PROT_STRUCT_SIZE = 6;

type
    TOWMessage = Array [0..OWNET_PROT_STRUCT_SIZE - 1] of Integer;
    TOWSocketType = (OWTCP, OWUDP);


    TOWNetSocket = Class(TObject)
    private
       _tcpsocket :  TTCPBlockSocket;
       _remoteServer : String;   // remote server
       _remotePort : Integer;    // remote port
       _persistentConnection : Boolean;
       _conn_timeout : Integer;
       _defDatalen : Integer;   // default data length for read/write commands
       _sg_flags : Integer;     // formating flags + persistent bit

       function  getRemotePort: Integer;
       function  getRemoteServer: String;
       procedure setRemotePort(const Value: Integer);
       procedure setRemoteServer(const Value: String);

       procedure CloseSockets(FreeSockets : Boolean = false);
       procedure setActiveSocket(socketType : TOWSocketType);

       Constructor Create; overload;
       Procedure Connect;
       Procedure Disconnect(Force : Boolean);
       Procedure sendPacket(func: Integer; payloadlen : Integer; datalen:Integer = 0);
       Function  getPacketData( packetHeader: TOWMessage ): String;
       Procedure sendCString(str:String);
       function  getPacket : TOWMessage;
       function  PackUnpackMsg(msg : TOWMessage) : TOWMessage;

       property  defDatalen :Integer read _defDatalen write _defDatalen;
       property  RemoteServer : String read getRemoteServer write setRemoteServer;
       property  RemotePort : Integer read  getRemotePort write setRemotePort;
       property  Timeout : Integer read _conn_timeout write _conn_timeout;
       property  sgFlags : Integer read _sg_flags write _sg_flags;
    end;



    TOWNet = Class
      private
         owsocket : TOWNetSocket;

         procedure setRemoteServer(const Value: String);
         function  getRemoteServer: String;

         procedure setRemotePort(const Value: Integer);
         function  getRemotePort: Integer;

         procedure setConnectionTimeout(const Value: Integer);
         function  getConnectionTimeout: Integer;

         procedure setDefaultDatalen(const Value: Integer);
         function  getDefaultDatalen: Integer;

         procedure setFormatingFlags(const Value: Integer);
         function  getFormatingFlags: Integer;

      public
         Constructor Create(); overload;
         Constructor Create(Host:String); overload;
         Constructor Create(Host:String; Port: Integer); overload;
         Destructor  Destroy;

         function  Read(path:String; expectedDatalen : Integer = 0) : String;
         function  Write(path : String; value : String) : Boolean;
         function  Presence(path : String) : Boolean;
         function  Dir(path : String; dirall : Boolean = True) : TStringList;

         property remoteServer : String read getRemoteServer write setRemoteServer;
         property remotePort : Integer read getRemotePort write setRemotePort;
         property connectionTimeout : Integer read getConnectionTimeout write setConnectionTimeout;
         property defaultDataLen : Integer read getDefaultDatalen write setDefaultDatalen;
         property formatingFlags : Integer read getFormatingFlags write setFormatingFlags;
    end;

Implementation

Uses SysUtils, StrUtils, synautil, winsock;

Const
    OWNET_DEFAULT_HOST = '127.0.0.1';
    OWNET_DEFAULT_PORT = 4304;

    OWNET_DEFAULT_DATALEN   = 4096;
    OWNET_DEFAULT_TIMEOUT   = 4000;
    OWNET_DEFAULT_SG_FLAGS  = $0000103;

    OWNET_MSG_ERROR     = 0;
    OWNET_MSG_NOP       = 1;
    OWNET_MSG_READ      = 2;
    OWNET_MSG_WRITE     = 3;
    OWNET_MSG_DIR       = 4;
    OWNET_MSG_SIZE      = 5;
    OWNET_MSG_PRESENCE  = 6;
    OWNET_MSG_DIRALL    = 7;
    OWNET_MSG_GET       = 8;
    OWNET_MSG_READ_ANY  = 99999;

    OWNET_PROT_VERSION     = 0;
    OWNET_PROT_PAYLOAD     = 1;
    OWNET_PROT_FUNCTION    = 2;
    OWNET_PROT_RETVALUE    = 2;
    OWNET_PROT_FLAGS       = 3;
    OWNET_PROT_DATALEN     = 4;
    OWNET_PROT_OFFSET      = 5;

    OWNET_FLAG_PERSIST      = $0000004; // for Persistent connections


{ TOWNetSocket }

constructor TOWNetSocket.Create;
begin
  inherited Create;
  _persistentConnection := false;
  _conn_timeout := OWNET_DEFAULT_TIMEOUT;
end;


{
procedure TOWNetSocket.CloseConnection;
begin
  _tcpsocket.CloseSocket;
  // Do not reset persistentConnection!
  //_persistentConnection := False;
end;
}

Procedure TOWNetSocket.CloseSockets(FreeSockets:Boolean = false);
begin
   if _tcpsocket<>Nil then
   Begin
     _tcpsocket.CloseSocket;
     if FreeSockets then FreeAndNil(_tcpSocket);
   End;
   // Do not reset persistentConnection!
   //_PersistentConnection := False;
end;


Procedure TOWNetSocket.setActiveSocket(SocketType : TOWSocketType);
Begin
   CloseSockets(true);
   _tcpsocket := TTCPBlockSocket.Create;
End;


function TOWNetSocket.PackUnpackMsg(msg: TOWMessage): TOWMessage;
var
    i : Integer;
    m : TOWMessage;
begin
  for i := 0 to OWNET_PROT_STRUCT_SIZE-1 do
  begin
    m[i] := ((msg[i] and $ff000000) shr 24) +
            ((msg[i] and $00ff0000) shr 8) +
            ((msg[i] and $0000ff00) shl 8) +
            ((msg[i] and $000000ff) shl 24);
  end;
  result := m;
end;



procedure TOWNetSocket.sendPacket(func, payloadlen : Integer; datalen : Integer = 0);
var
  msg : TOWMessage;
  expDataLen : Integer;
begin
  if datalen=0 then
     expDataLen := defDatalen
  else
     expDataLen := datalen;

  msg[OWNET_PROT_VERSION]  := 0;
  msg[OWNET_PROT_PAYLOAD]  := payloadlen;
  msg[OWNET_PROT_FUNCTION] := func;
  msg[OWNET_PROT_FLAGS]    := _sg_flags or OWNET_FLAG_PERSIST;
  msg[OWNET_PROT_DATALEN]  := expdatalen;
  msg[OWNET_PROT_OFFSET]   := 0;
  msg := PackUnpackMsg(msg);
  try
    _tcpsocket.SendBuffer(@msg,sizeof(msg));
  except
    if (_PersistentConnection) then
    begin
       Disconnect(true);
       Connect;
       _tcpsocket.SendBuffer(@msg,sizeof(msg));
    end;
  end;
end;

procedure TOWNetSocket.sendCString(str: String);
begin
    _tcpsocket.SendString(str+#0);
end;


function TOWNetSocket.getPacket: TOWMessage;
var
  msg : TOWMessage;
begin
  try
    fillchar(msg,sizeof(msg),0);
    _tcpsocket.RecvBufferEx(@msg,sizeof(msg),_conn_timeout);
    msg := PackUnpackMsg(msg);
    _persistentConnection := ((msg[OWNET_PROT_FLAGS] and OWNET_FLAG_PERSIST) = OWNET_FLAG_PERSIST);
  finally
    result := msg;
  end;
end;



function TOWNetSocket.getPacketData(packetHeader: TOWMessage): String;
var
  data: pointer;
  retVal : String;
begin
  retval := '';
  try try
     getmem(data, packetHeader[OWNET_PROT_PAYLOAD]+1);
    _tcpsocket.RecvBufferEx(data,packetHeader[OWNET_PROT_PAYLOAD],Timeout);
    retVal := AnsiMidStr(pchar(data),0 {packetHeader[OWNET_PROT_OFFSET]}, packetHeader[OWNET_PROT_DATALEN]);
  except
  end
  finally
    freemem(data);
    result := retval;
  end;
end;


procedure TOWNetSocket.Connect;
begin
  if _tcpsocket <> nil then
  begin
      if not _persistentConnection then
      begin
        CloseSockets(true);
        setActiveSocket(OWTCP);
        _tcpsocket.Connect(_remoteServer,intToStr(_remotePort));
      End
      else
       _tcpsocket.Connect(_remoteServer,intToStr(_remotePort));
  end
  else
  begin
     CloseSockets(true);
     setActiveSocket(OWTCP);
     _tcpsocket.Connect(_remoteServer,intToStr(_remotePort));
  end;
end;


procedure TOWNetSocket.Disconnect(Force: Boolean);
begin
  if ((not _PersistentConnection) or force) and (_tcpsocket <> Nil) then closeSockets(true);
end;


function TOWNetSocket.getRemotePort: Integer;
begin
  result := _remotePort;
end;

function TOWNetSocket.getRemoteServer: String;
begin
  result := _remoteServer;
end;

procedure TOWNetSocket.setRemotePort(const Value: Integer);
begin
  _remotePort := Value;
end;

procedure TOWNetSocket.setRemoteServer(const Value: String);
begin
  _remoteServer := Value;
end;



{ TOWNet }


constructor TOWNet.Create;
begin
   inherited;
   owsocket          := TOWNetSocket.Create;
   remoteServer      := OWNET_DEFAULT_HOST;
   remotePort        := OWNET_DEFAULT_PORT;
   formatingFlags    := OWNET_DEFAULT_SG_FLAGS;
   connectionTimeout := OWNET_DEFAULT_TIMEOUT;
   defaultDataLen    := OWNET_DEFAULT_DATALEN;
end;


Constructor TOWNet.Create(Host:String);
var
  i : Integer;
begin
  self.Create;
  i := pos(':',Host);
  if i=0 then remoteServer:=Host
  else
  begin
     remoteServer := LeftStr(Host,i-1);
     remotePort := StrToInt(RightStr(Host,Length(Host)-i));
  end;
end;


constructor TOWNet.Create(Host: String; Port: Integer);
begin
  Self.Create;
  RemoteServer := Host;
  RemotePort := Port;
end;


destructor TOWNet.Destroy;
begin
   try
     if owsocket<>Nil then owsocket.CloseSockets(true);
     freeandnil(owsocket);
   Except
   end;
   inherited;
end;


// ---

procedure TOWNet.setConnectionTimeout(const Value: Integer);
begin
  owsocket.Timeout := Value;
end;
function TOWNet.getConnectionTimeout: Integer;
begin
    result := owsocket.Timeout;
end;

// ---

procedure TOWNet.setDefaultDatalen(const Value: Integer);
begin
 owsocket.defDatalen := Value;
end;
function TOWNet.getDefaultDatalen: Integer;
begin
   result := owsocket.defDatalen;
end;

// ---

procedure TOWNet.setFormatingFlags(const Value: Integer);
begin
  owsocket.sgFlags := Value;
end;
function TOWNet.getFormatingFlags: Integer;
begin
  result := owsocket.sgFlags;
end;

// ---

procedure TOWNet.setRemotePort(const Value: Integer);
begin
  owsocket.remotePort := Value;
end;
function TOWNet.getRemotePort: Integer;
begin
   result := owsocket.RemotePort;
end;


// ---

procedure TOWNet.setRemoteServer(const Value: String);
begin
  owsocket.RemoteServer := Value;
end;
function TOWNet.getRemoteServer: String;
begin
   result := owsocket.RemoteServer;
end;



// ###########################
// #  O W N e t              #
// ###########################

function TOWNet.Presence(path: String): Boolean;
var
  msg : TOWMessage;
  retVal : Boolean;
begin
  retVal := false;
  try
    owsocket.Connect;
    owsocket.sendPacket(OWNET_MSG_PRESENCE, length(path)+1);
    owsocket.sendCString(path);
    msg := owsocket.getPacket;
    retVal :=  (msg[OWNET_PROT_RETVALUE] >= 0);
  finally
    owsocket.Disconnect(false);
    result := retval;
  end;
end;

function TOWNet.Read(path: String; expectedDatalen: Integer = 0): String;
var
  msg  : TOWMessage;
  retval : String;
begin
   retVal := '';
   try
     owsocket.Connect;
     owsocket.sendPacket(OWNET_MSG_READ, length(path)+1, expectedDataLen);
     owsocket.sendCString(path);
     msg := owsocket.getPacket;
     if (msg[OWNET_PROT_RETVALUE] >= 0) and (msg[OWNET_PROT_PAYLOAD]>=0) then retVal := owsocket.getPacketData(msg);
   finally
     owsocket.Disconnect(false);
     result := retVal;
   end;
end;

function TOWNet.Dir(path: String; dirall: Boolean = true): TStringList;
var
  msg : TOWMessage;
  retVal : TStringList;
  owcommand : Integer;
  s : String;
begin
   s := '';
   try
     if dirall then
        owcommand := OWNET_MSG_DIRALL
     else
        owcommand := OWNET_MSG_DIR;
     owsocket.Connect;
     owsocket.sendPacket(owcommand, length(path)+1);
     owsocket.sendCString(path);

     repeat
        msg := owsocket.getPacket;
        if (msg[OWNET_PROT_RETVALUE] >= 0) and (msg[OWNET_PROT_PAYLOAD] > 0) then
        Begin
          if dirall then
             s := owsocket.getPacketData(msg)
          else
          begin
             if length(s)>0 then s:=s+',';
             s := s + owsocket.getPacketData(msg);
          end;
        end;
      until dirall or (msg[OWNET_PROT_PAYLOAD]=0); //<0 = please wait, 0=end of list, >0 = we have data waiting (is this the way?)
   finally
     retVal := TStringList.Create;
     retVal.CommaText := s;
     owsocket.Disconnect(false);
     result := retVal;
   end;
end;

function TOWNet.Write(path, value: String) : Boolean;
var
  msg  : TOWMessage;
  retval : boolean;
begin
   retVal := false;
   try
     owsocket.Connect;
     owsocket.sendPacket(OWNET_MSG_WRITE, length(path)+1 + length(value)+1, length(value)+1);
     owsocket.sendCString(path);
     owsocket.sendCString(value);
     msg := owsocket.getPacket;
     retVal := (msg[OWNET_PROT_RETVALUE] >= 0);
   finally
     owsocket.Disconnect(false);
     result := retVal;
   end;

end;

end.

