VERSION 5.00
Object = "{6B7E6392-850A-101B-AFC0-4210102A8DA7}#1.3#0"; "COMCTL32.OCX"
Begin VB.Form frmDownloader 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Downloader"
   ClientHeight    =   2655
   ClientLeft      =   45
   ClientTop       =   330
   ClientWidth     =   5445
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   2655
   ScaleWidth      =   5445
   StartUpPosition =   3  'Windows Default
   Begin ComctlLib.ProgressBar ProgressBar1 
      Height          =   255
      Left            =   960
      TabIndex        =   6
      Top             =   1080
      Width           =   4335
      _ExtentX        =   7646
      _ExtentY        =   450
      _Version        =   327682
      Appearance      =   1
   End
   Begin VB.CommandButton cmdCancel 
      Caption         =   "Cancel"
      Enabled         =   0   'False
      Height          =   375
      Left            =   3000
      TabIndex        =   3
      Top             =   2040
      Width           =   1695
   End
   Begin VB.TextBox txtDestination 
      Height          =   285
      Left            =   960
      TabIndex        =   1
      Text            =   "C:\psc.htm"
      Top             =   600
      Width           =   4335
   End
   Begin VB.TextBox txtURL 
      Height          =   285
      Left            =   960
      TabIndex        =   0
      Text            =   "http://www.planet-source-code.com/"
      Top             =   200
      Width           =   4335
   End
   Begin VB.CommandButton cmdDownload 
      Caption         =   "Download"
      Height          =   375
      Left            =   720
      TabIndex        =   2
      Top             =   2040
      Width           =   1695
   End
   Begin VB.Label lblBytes 
      Caption         =   "0"
      Height          =   255
      Left            =   2880
      TabIndex        =   9
      Top             =   1560
      Width           =   1455
   End
   Begin VB.Label Label3 
      Caption         =   "Bytes received:"
      Height          =   255
      Left            =   1680
      TabIndex        =   8
      Top             =   1560
      Width           =   1215
   End
   Begin VB.Label lblRate 
      Alignment       =   2  'Center
      Caption         =   "0 %"
      Height          =   255
      Left            =   120
      TabIndex        =   7
      Top             =   1080
      Width           =   735
   End
   Begin VB.Label Label2 
      Alignment       =   1  'Right Justify
      Caption         =   "Destination:"
      Height          =   255
      Left            =   -120
      TabIndex        =   5
      Top             =   600
      Width           =   975
   End
   Begin VB.Label Label1 
      Alignment       =   1  'Right Justify
      Caption         =   "URL:"
      Height          =   255
      Left            =   360
      TabIndex        =   4
      Top             =   240
      Width           =   495
   End
End
Attribute VB_Name = "frmDownloader"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Dim WithEvents Download As CDownload
Attribute Download.VB_VarHelpID = -1

Private Sub Form_Load()
Set Download = New CDownload
End Sub

Private Sub cmdDownload_Click()
cmdDownload.Enabled = False
txtURL.Enabled = False
txtDestination.Enabled = False
cmdCancel.Enabled = True
ProgressBar1.Value = 0
lblRate = "0 %"
lblBytes = "0"
Download.Download txtURL, txtDestination
End Sub

Private Sub cmdCancel_Click()
cmdCancel.Enabled = False
Download.Cancel
txtURL.Enabled = True
txtDestination.Enabled = True
cmdDownload.Enabled = True
End Sub

Private Sub Download_Starting(ByVal FileSize As Long, ByVal Header As String)
If FileSize <> 0 Then
    ProgressBar1.Max = FileSize
End If
End Sub

Private Sub Download_DataArrival(ByVal bytesTotal As Long)
lblBytes = Val(lblBytes) + bytesTotal
If Download.FileSize <> 0 Then
    ProgressBar1.Value = ProgressBar1.Value + bytesTotal
    lblRate = Int(Val(lblBytes) * 100 / Download.FileSize) & "%"
End If
End Sub

Private Sub Download_Completed()
lblRate = "100 %"
ProgressBar1.Max = 100
ProgressBar1.Value = 100

MsgBox "Download completed", vbOKOnly, "Done"

cmdCancel.Enabled = False
txtURL.Enabled = True
txtDestination.Enabled = True
cmdDownload.Enabled = True
End Sub

Private Sub Download_Error(ByVal Number As Integer, Description As String)
cmdCancel.Enabled = False
MsgBox Description, vbCritical, "Error " & Number
txtURL.Enabled = True
txtDestination.Enabled = True
cmdDownload.Enabled = True
End Sub

Private Sub txtDestination_GotFocus()
txtDestination.SelStart = Len(txtDestination)
txtDestination.SelLength = 0
End Sub

Private Sub txtURL_GotFocus()
txtURL.SelStart = Len(txtURL)
txtURL.SelLength = 0
End Sub
