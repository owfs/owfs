VERSION 5.00
Begin VB.Form Server 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Server"
   ClientHeight    =   3195
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   4680
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   3195
   ScaleWidth      =   4680
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdSend 
      Caption         =   "Send"
      Default         =   -1  'True
      Enabled         =   0   'False
      Height          =   375
      Left            =   3240
      TabIndex        =   4
      Top             =   2760
      Width           =   975
   End
   Begin VB.TextBox txtMessage 
      Height          =   300
      Left            =   240
      MaxLength       =   22
      TabIndex        =   5
      Top             =   2760
      Width           =   2775
   End
   Begin VB.TextBox txtPort 
      Height          =   315
      Left            =   2880
      MaxLength       =   5
      TabIndex        =   2
      Text            =   "7777"
      Top             =   120
      Width           =   735
   End
   Begin VB.CommandButton cmdListen 
      Caption         =   "Listen"
      Height          =   375
      Left            =   600
      TabIndex        =   1
      Top             =   120
      Width           =   1215
   End
   Begin VB.TextBox txtLog 
      Height          =   2055
      Left            =   120
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   0
      TabStop         =   0   'False
      Top             =   600
      Width           =   4335
   End
   Begin VB.Label Label1 
      Caption         =   "Port:"
      Height          =   255
      Left            =   2160
      TabIndex        =   3
      Top             =   160
      Width           =   495
   End
End
Attribute VB_Name = "Server"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Dim WithEvents Server As CSocketMaster
Attribute Server.VB_VarHelpID = -1

Private Sub Form_Load()
Client.Show
Set Server = New CSocketMaster
End Sub

Private Sub cmdListen_Click()
cmdListen.Enabled = False
txtPort.Enabled = False
Server.Bind txtPort
Server.Listen
End Sub

Private Sub cmdSend_Click()
Server.SendData txtMessage
txtMessage = ""
End Sub

Private Sub Server_CloseSck()
cmdSend.Enabled = False
cmdListen.Enabled = True
txtPort.Enabled = True
MsgBox "Connection closed"
Server.CloseSck
End Sub

Private Sub Server_ConnectionRequest(ByVal requestID As Long)
Server.CloseSck
Server.Accept requestID
cmdSend.Enabled = True
End Sub

Private Sub Server_DataArrival(ByVal bytesTotal As Long)
Dim data As String
Server.GetData data
txtLog = txtLog + data + vbCrLf
End Sub

Private Sub Server_Error(ByVal Number As Integer, Description As String, ByVal sCode As Long, ByVal Source As String, ByVal HelpFile As String, ByVal HelpContext As Long, CancelDisplay As Boolean)
MsgBox Description, vbCritical, "Server Error " & Number
cmdSend.Enabled = False
cmdListen.Enabled = True
txtPort.Enabled = True
Server.CloseSck
End Sub
