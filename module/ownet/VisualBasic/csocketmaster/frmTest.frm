VERSION 5.00
Begin VB.Form frmTest 
   BorderStyle     =   1  'Fixed Single
   Caption         =   "OWNet - VB"
   ClientHeight    =   3720
   ClientLeft      =   45
   ClientTop       =   360
   ClientWidth     =   7410
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   3720
   ScaleWidth      =   7410
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdDir 
      Caption         =   "Dir"
      Height          =   285
      Left            =   3255
      TabIndex        =   10
      Top             =   840
      Width           =   945
   End
   Begin VB.CommandButton cmdPresence 
      Caption         =   "Presence"
      Height          =   285
      Left            =   2205
      TabIndex        =   9
      Top             =   840
      Width           =   945
   End
   Begin VB.TextBox txtOutput 
      Height          =   2430
      Left            =   0
      MultiLine       =   -1  'True
      ScrollBars      =   3  'Both
      TabIndex        =   8
      Top             =   1260
      Width           =   7365
   End
   Begin VB.CommandButton cmdWrite 
      Caption         =   "Write"
      Height          =   285
      Left            =   1155
      TabIndex        =   7
      Top             =   840
      Width           =   945
   End
   Begin VB.TextBox txtPath 
      Height          =   285
      Left            =   1050
      TabIndex        =   6
      Text            =   "/10.E8C1C9000800/temperature"
      Top             =   375
      Width           =   4305
   End
   Begin VB.TextBox txtPort 
      Height          =   285
      Left            =   4350
      TabIndex        =   4
      Text            =   "1234"
      Top             =   45
      Width           =   945
   End
   Begin VB.TextBox txtHost 
      Height          =   285
      Left            =   1050
      TabIndex        =   2
      Text            =   "127.0.0.1"
      Top             =   30
      Width           =   1995
   End
   Begin VB.CommandButton cmdRead 
      Caption         =   "Read"
      Height          =   285
      Left            =   105
      TabIndex        =   0
      Top             =   840
      Width           =   945
   End
   Begin OWNet_VB.OWNet tmp_OWNet 
      Left            =   5985
      Top             =   630
      _ExtentX        =   2619
      _ExtentY        =   397
   End
   Begin VB.Label Label3 
      AutoSize        =   -1  'True
      BackStyle       =   0  'Transparent
      Caption         =   "Path:"
      Height          =   195
      Left            =   60
      TabIndex        =   5
      Top             =   435
      Width           =   375
   End
   Begin VB.Label Label2 
      AutoSize        =   -1  'True
      BackStyle       =   0  'Transparent
      Caption         =   "Server Port:"
      Height          =   195
      Left            =   3330
      TabIndex        =   3
      Top             =   105
      Width           =   840
   End
   Begin VB.Label Label1 
      AutoSize        =   -1  'True
      BackStyle       =   0  'Transparent
      Caption         =   "Server Host:"
      Height          =   195
      Left            =   30
      TabIndex        =   1
      Top             =   90
      Width           =   885
   End
End
Attribute VB_Name = "frmTest"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'
'VERSION: 2007.01.11 - 17:27  BRST
'
'Copyright (c) 2006 Spadim Technology / Brazil. All rights reserved.
'
'This program is free software; you can redistribute it and/or modify
'it under the terms of the GNU General Public License as published by
'the Free Software Foundation; either version 2 of the License, or (at
'your option) any later version.
'
'This program is distributed in the hope that it will be useful, but
'WITHOUT ANY WARRANTY; without even the implied warranty of
'MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
'General Public License for more details.
'
'You should have received a copy of the GNU General Public License
'along with this program; if not, write to the Free Software
'Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
'
'OWFS is an open source project developed by Paul Alfille and hosted at
'http://www.owfs.org
'
'mailto: roberto@ spadim.com.br
'        www.spadim.com.br
'
'
'

Option Explicit
Private Sub cmdDir_Click()
        On Error Resume Next
        Dim RetStr As String
        tmp_OWNet.SetHost txtHost.Text
        tmp_OWNet.SetPort CLng(txtPort.Text)
        RetStr = tmp_OWNet.Dir(txtPath.Text)
        If (tmp_OWNet.LastError() <> OWNET_ERR_NOERROR) Then
                txtOutput = "Error Reading Dir: " & tmp_OWNet.LastError()
        Else
                txtOutput = Replace(RetStr, ",", vbNewLine)
        End If
End Sub
Private Sub cmdPresence_Click()
        On Error Resume Next
        Dim RetStr As Boolean
        tmp_OWNet.SetHost txtHost.Text
        tmp_OWNet.SetPort CLng(txtPort.Text)
        RetStr = tmp_OWNet.Presence(txtPath.Text)
        If (tmp_OWNet.LastError() <> OWNET_ERR_NOERROR) Then
                txtOutput = "Error Reading Presence: " & tmp_OWNet.LastError()
        Else
                If (RetStr) Then
                        txtOutput = "Present"
                Else
                        txtOutput = "Not Present"
                End If
        End If
End Sub
Private Sub cmdRead_Click()
        On Error Resume Next
        Dim RetStr As String
        tmp_OWNet.SetHost txtHost.Text
        tmp_OWNet.SetPort CLng(txtPort.Text)
        RetStr = tmp_OWNet.Read(txtPath.Text)
        If (tmp_OWNet.LastError() <> OWNET_ERR_NOERROR) Then
                txtOutput = "Error Reading: " & tmp_OWNet.LastError()
        Else
                txtOutput = RetStr
        End If
End Sub
Private Sub cmdWrite_Click()
        On Error Resume Next
        Dim RetStr As Boolean
        tmp_OWNet.SetHost txtHost.Text
        tmp_OWNet.SetPort CLng(txtPort.Text)
        RetStr = tmp_OWNet.SetValue(txtPath.Text, InputBox("New Value for " & txtPath.Text, "New Value"))
        If (tmp_OWNet.LastError() <> OWNET_ERR_NOERROR) Then
                txtOutput = "Error Writing: " & tmp_OWNet.LastError()
        Else
                If (RetStr) Then
                        txtOutput = "Write OK"
                Else
                        txtOutput = "Can't Write"
                End If
        End If
End Sub

