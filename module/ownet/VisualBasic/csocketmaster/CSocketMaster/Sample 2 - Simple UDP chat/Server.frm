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
   Begin VB.CommandButton cmdBind 
      Caption         =   "Bind"
      Height          =   375
      Left            =   1800
      TabIndex        =   3
      Top             =   120
      Width           =   975
   End
   Begin VB.TextBox txtPort 
      Height          =   315
      Left            =   840
      MaxLength       =   5
      TabIndex        =   1
      Text            =   "8888"
      Top             =   120
      Width           =   735
   End
   Begin VB.TextBox txtLog 
      Height          =   2415
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
      Left            =   360
      TabIndex        =   2
      Top             =   165
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
Server.Protocol = sckUDPProtocol
End Sub

Private Sub cmdBind_Click()
Server.CloseSck
Server.Bind txtPort
End Sub

Private Sub Server_DataArrival(ByVal bytesTotal As Long)
Dim data As String
Server.GetData data
txtLog = txtLog + data + vbCrLf
End Sub

Private Sub Server_Error(ByVal Number As Integer, Description As String, ByVal sCode As Long, ByVal Source As String, ByVal HelpFile As String, ByVal HelpContext As Long, CancelDisplay As Boolean)
MsgBox Description, vbCritical, "Server Error " & Number
Server.CloseSck
End Sub
