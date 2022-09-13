Attribute VB_Name = "mdlTwain"
Option Explicit

''########################################################''
''//////////////// DLL 호출 함수들 ////////////////////////
'// 시작함수
Public Declare Function CALL_InitInstance Lib "TwainCall.dll" (ByVal hInstance As Long, ByVal CallBack As Long) As Long
'// select
Public Declare Function CALL_SelectSource Lib "TwainCall.dll" () As Long
'// Aquire and Scan
Public Declare Function CALL_TWAppAquire Lib "TwainCall.dll" (ByVal ShowUI As Integer, ByVal FlagScan As Integer, ByVal NRecogForm As Integer) As Long
'//Public Declare Function CALL_ProcessTWMessage Lib "TwainCall.dll" (ByVal lpMsg As String, ByVal hWnd As Long) As Long
'// 종료함수
Public Declare Function CALL_TWAppQuit Lib "TwainCall.dll" () As Long
'// 스캐너 스위치 클릭 동작
Public Declare Function CALL_TWSwClick Lib "TwainCall.dll" () As Long

'**************테스트용 함수
Public Declare Function CALL_TestFun Lib "TwainCall.dll" (ByVal imgFilename As String, ByVal Formnumber As Long, ByVal RnBdiff As Long, ByVal Thetavalue As Double) As Long
Public Declare Function CALL_TestMem Lib "TwainCall.dll" (ByVal imgFilename As String) As Long
''########################################################''

'/****************************************************************************
' * Country Constants                                                        *
' ****************************************************************************/
'
Public Const TWCY_USA                   As Integer = 1

