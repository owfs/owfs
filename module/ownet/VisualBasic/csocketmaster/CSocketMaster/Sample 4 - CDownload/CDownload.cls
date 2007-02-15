VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
  Persistable = 0  'NotPersistable
  DataBindingBehavior = 0  'vbNone
  DataSourceBehavior  = 0  'vbNone
  MTSTransactionMode  = 0  'NotAnMTSObject
END
Attribute VB_Name = "CDownload"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = True
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Explicit

'NOTE: If FileSize = 0 that means the size of the file
'      is unknown.

'==============================================================================
'EVENTS
'==============================================================================

Public Event Starting(ByVal FileSize As Long, ByVal Header As String)
Attribute Starting.VB_Description = "Occurs when file download is about to start"
Public Event DataArrival(ByVal bytesTotal As Long)
Attribute DataArrival.VB_Description = "Occurs when data has been received from the remote computer"
Public Event Error(ByVal Number As Integer, Description As String)
Attribute Error.VB_Description = "Error occurred"
Public Event Completed()
Attribute Completed.VB_Description = "Occurs when download is completed"

'==============================================================================
'CONSTANTS
'==============================================================================

Public Enum AccessConstants
    cdDirect = 0
    cdNamedProxy = 1
End Enum

'==============================================================================
'MEMBER VARIABLES
'==============================================================================

Private m_acAccess As AccessConstants
Private m_strProxy As String
Private m_strURL As String
Private m_strDestination As String
Private m_lngProxyPort As Long
Private m_blnRedirDisabled As Boolean

Private m_strHeader As String
Private m_blnHeaderArrived As Boolean
Private m_intFileHandle As Integer
Private m_lngFileSize As Long

'our socket
Private WithEvents cmSocket As CSocketMaster
Attribute cmSocket.VB_VarHelpID = -1

Private Sub Class_Terminate()
Set cmSocket = Nothing
End Sub

'==============================================================================
'PROPERTIES
'==============================================================================

Public Property Get Proxy() As String
Attribute Proxy.VB_Description = "Returns/Sets HTTP proxy"
Proxy = m_strProxy
End Property

Public Property Let Proxy(ByVal strProxy As String)
m_strProxy = Trim(strProxy)
End Property

Public Property Get ProxyPort() As Long
Attribute ProxyPort.VB_Description = "Returns/Sets proxy port"
ProxyPort = m_lngProxyPort
End Property

Public Property Let ProxyPort(ByVal lngProxyPort As Long)
m_lngProxyPort = lngProxyPort
End Property

Public Property Get AccessType() As AccessConstants
Attribute AccessType.VB_Description = "Returns/Sets the proxy behavior for this control's connections"
AccessType = m_acAccess
End Property

Public Property Let AccessType(ByVal acAccess As AccessConstants)
m_acAccess = acAccess
End Property

Public Property Get URL() As String
Attribute URL.VB_Description = "Returns/Sets file URL"
URL = m_strURL
End Property

Public Property Let URL(ByVal strURL As String)
m_strURL = Trim(strURL)
End Property

Public Property Get Destination() As String
Attribute Destination.VB_Description = "Returns/Sets full path where the file will be saved"
Destination = m_strDestination
End Property

Public Property Let Destination(ByVal strDestination As String)
m_strDestination = Trim(Destination)
End Property

Public Property Get DisableRedirection() As Boolean
Attribute DisableRedirection.VB_Description = "Returns/Sets if automatic redirection is disabled"
DisableRedirection = m_blnRedirDisabled
End Property

Public Property Let DisableRedirection(ByVal blnRedir As Boolean)
m_blnRedirDisabled = blnRedir
End Property

Public Property Get FileSize() As Long
Attribute FileSize.VB_Description = "Returns file size in bytes or zero if it is unknown"
FileSize = m_lngFileSize
End Property

Public Sub Download(Optional URL As Variant, Optional Destination As Variant)
Attribute Download.VB_Description = "Download file"
On Error GoTo Error_Handler
Set cmSocket = New CSocketMaster

If Not IsMissing(URL) Then
    m_strURL = Trim(URL)
End If

If Not IsMissing(Destination) Then
    m_strDestination = Trim(Destination)
End If

If m_acAccess = cdDirect Then
    cmSocket.Connect GetHostFromURL(m_strURL), 80
Else
    cmSocket.Connect m_strProxy, m_lngProxyPort
End If

Exit Sub
Error_Handler:
    Reset
    RaiseEvent Error(Err.Number, Err.Description)
End Sub

Public Sub Cancel()
Attribute Cancel.VB_Description = "Method used to cancel download"
Reset
End Sub


Private Sub cmSocket_Connect()
On Error GoTo Error_Handler

'Create the destination file
If Dir(m_strDestination, vbHidden + vbArchive + vbNormal + vbReadOnly + vbSystem) = GetFileFromPath(m_strDestination) Then SetAttr m_strDestination, vbNormal: Kill m_strDestination
m_intFileHandle = FreeFile
Open m_strDestination For Binary Lock Read Write As m_intFileHandle

Dim strCommand As String

strCommand = "GET " + GetFileFromURL(m_strURL) + " HTTP/1.0" + vbCrLf
strCommand = strCommand + "Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, application/vnd.ms-powerpoint, application/vnd.ms-excel, application/msword, application/x-shockwave-flash, */*" + vbCrLf
strCommand = strCommand + "Referer: " + GetHostFromURL(m_strURL) + vbCrLf
strCommand = strCommand + "User-Agent: Mozilla/4.0 (compatible; MSIE 5.5; Windows 98; Win 9x 4.90)" + vbCrLf
strCommand = strCommand + "Host: " + GetHostFromURL(m_strURL) + vbCrLf
strCommand = strCommand + vbCrLf

cmSocket.SendData strCommand

Exit Sub
Error_Handler:
    Reset
    RaiseEvent Error(Err.Number, Err.Description)
End Sub

Private Sub cmSocket_DataArrival(ByVal bytesTotal As Long)
On Error GoTo Error_Handler
Dim strChunk As String
cmSocket.GetData strChunk

'if header hasn't arrived
If m_blnHeaderArrived = False Then
    
    m_strHeader = m_strHeader & strChunk
    
    Dim lngSplit As Long
    lngSplit = InStr(1, m_strHeader, vbCrLf + vbCrLf)
    
    'has the header finished on this chunk?
    If lngSplit = 0 Or lngSplit = Null Then Exit Sub
    
    'yes! the header has finished
    m_blnHeaderArrived = True
    
    'maybe this chunk is half header and half file
    'we split the two
    strChunk = Right(m_strHeader, Len(m_strHeader) - lngSplit - 3)
    m_strHeader = Left(m_strHeader, lngSplit + 3)
    
    'is redirection enabled?
    If m_blnRedirDisabled = False Then
        Dim strLocation As String
        strLocation = GetVariableValue(m_strHeader, "Location")
        'does the header indicates a redirection?
        If strLocation <> "" Then
            Reset
            m_strURL = strLocation
            Download
            Exit Sub
        End If
    End If
    
    Dim strFileSize As String
    
    strFileSize = GetVariableValue(m_strHeader, "Content-Length")
    If strFileSize = "" Then
        m_lngFileSize = 0
    Else
        m_lngFileSize = Val(strFileSize)
    End If
    
    RaiseEvent Starting(m_lngFileSize, m_strHeader)
End If

'if header has arrived

Put m_intFileHandle, LOF(m_intFileHandle) + 1, strChunk

RaiseEvent DataArrival(Len(strChunk))

Exit Sub
Error_Handler:
    Reset
    RaiseEvent Error(Err.Number, Err.Description)
End Sub

Private Sub cmSocket_CloseSck()

'some web pages don't have headers so we have to
'raise all the events that couldn't be raised while
'the file was downloading
If m_blnHeaderArrived = False Then
    
    Dim strData As String
    strData = m_strHeader
    m_strHeader = ""
    
    RaiseEvent Starting(Len(strData), "")
    Put m_intFileHandle, LOF(m_intFileHandle) + 1, strData
    
    RaiseEvent DataArrival(Len(strData))
End If

Reset
RaiseEvent Completed
End Sub

'Ups! We got an error
Private Sub cmSocket_Error(ByVal Number As Integer, Description As String, ByVal sCode As Long, ByVal Source As String, ByVal HelpFile As String, ByVal HelpContext As Long, CancelDisplay As Boolean)
Reset
RaiseEvent Error(Number, Description)
End Sub

'returns the host from an URL
'ie: 'http://www.yahoo.com/file.txt' => 'www.yahoo.com'
Private Function GetHostFromURL(ByVal strURL As String) As String

strURL = Trim(strURL)
If Left(strURL, 7) = "http://" Then strURL = Mid(strURL, 8, Len(strURL) - 7)

Dim Init As Integer
Init = InStr(1, strURL, "/", vbTextCompare)

If Init <> 0 Then strURL = Left(strURL, Init - 1)
GetHostFromURL = strURL

End Function

'get the file part from an URL that goes after the
'GET command to download files IF IT IS NOT USING PROXY
'ie: 'http://www.yahoo.com/file.txt' => '/file.txt'
Private Function GetFileFromURL(ByVal strURL As String) As String

If m_acAccess = cdNamedProxy Then
    GetFileFromURL = strURL
    Exit Function
End If

If Left(strURL, 7) = "http://" Then strURL = Right(strURL, Len(strURL) - 7)
Dim Init As Integer
Init = InStr(1, strURL, "/", vbTextCompare)
If Init = 0 Or Init = Null Then
    GetFileFromURL = "/"
Else
    GetFileFromURL = Right(strURL, Len(strURL) - Init + 1)
End If
End Function

'get file part from a path
'ie: 'c:\folder\file.txt' => 'file.txt'
Private Function GetFileFromPath(ByVal strPath As String) As String
GetFileFromPath = strPath
If InStr(1, strPath, "\", vbTextCompare) = 0 Then Exit Function
Dim Position As Long
Position = 1
Do Until (Mid(strPath, Len(strPath) - Position, 1) = "\")
    Position = Position + 1
Loop
GetFileFromPath = Right(strPath, Position)
End Function

'get variable value from the header
Private Function GetVariableValue(ByRef strHeader As String, ByVal strVariable As String) As String
Dim Init As Long
Dim Last As Long

Init = InStr(1, strHeader, strVariable, vbTextCompare)

If Init = 0 Or Init = Null Then
    GetVariableValue = ""
    Exit Function
End If

Init = Init + Len(strVariable) + 1
Last = InStr(Init, strHeader, vbCrLf, vbTextCompare)


GetVariableValue = Trim(Mid(strHeader, Init, Last - Init))

End Function

'reset variables
Private Sub Reset()
Set cmSocket = Nothing
m_strHeader = ""
m_blnHeaderArrived = False
If m_intFileHandle <> 0 Then Close #m_intFileHandle
m_intFileHandle = 0
m_lngFileSize = 0
End Sub
