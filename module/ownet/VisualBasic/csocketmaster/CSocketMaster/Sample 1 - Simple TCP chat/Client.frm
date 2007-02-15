VERSION 5.00
Begin VB.Form Client 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Client"
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
   Begin VB.TextBox txtLog 
      Height          =   2055
      Left            =   120
      Locked          =   -1  'True
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   4
      TabStop         =   0   'False
      Top             =   600
      Width           =   4335
   End
   Begin VB.CommandButton cmdConnect 
      Caption         =   "Connect"
      Height          =   375
      Left            =   600
      TabIndex        =   3
      Top             =   120
      Width           =   1215
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
   Begin VB.TextBox txtMessage 
      Height          =   300
      Left            =   240
      MaxLength       =   22
      TabIndex        =   0
      Top             =   2760
      Width           =   2775
   End
   Begin VB.CommandButton cmdSend 
      Caption         =   "Send"
      Default         =   -1  'True
      Enabled         =   0   'False
      Height          =   375
      Left            =   3240
      TabIndex        =   1
      Top             =   2760
      Width           =   975
   End
   Begin VB.Label Label1 
      Caption         =   "Port:"
      Height          =   255
      Left            =   2160
      TabIndex        =   5
      Top             =   165
      Width           =   495
   End
End
Attribute VB_Name = "Client"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Dim WithEvents Client As CSocketMaster
Attribute Client.VB_VarHelpID = -1

Private Sub Form_Load()
Set Client = New CSocketMaster
End Sub

Private Sub cmdConnect_Click()
cmdConnect.Enabled = False
txtPort.Enabled = False
Client.Connect Client.LocalIP, txtPort
End Sub

Private Sub cmdSend_Click()
Client.SendData txtMessage
txtMessage = ""
End Sub

Private Sub Client_Connect()
cmdSend.Enabled = True
End Sub

Private Sub Client_CloseSck()
cmdSend.Enabled = False
cmdConnect.Enabled = True
txtPort.Enabled = True
MsgBox "Connection closed"
Client.CloseSck
End Sub

Private Sub Client_DataArrival(ByVal bytesTotal As Long)
Dim data As String
Client.GetData data
txtLog = txtLog + data + vbCrLf
End Sub

Private Sub Client_Error(ByVal Number As Integer, Description As String, ByVal sCode As Long, ByVal Source As String, ByVal HelpFile As String, ByVal HelpContext As Long, CancelDisplay As Boolean)
MsgBox Description, vbCritical, "Client Error " & Number
cmdSend.Enabled = False
cmdConnect.Enabled = True
txtPort.Enabled = True
Client.CloseSck
End Sub

