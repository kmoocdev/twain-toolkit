Attribute VB_Name = "mdlUser"
Option Explicit

Public PrevProc     As Long
Public hMainWnd     As Long
''Public NewDSIdentity As TW_IDENTITY


'/*
' * SetWindowPos Flags
' */
Public Const SWP_NOSIZE         As Long = &H1
Public Const SWP_NOMOVE         As Long = &H2
Public Const SWP_NOZORDER       As Long = &H4
Public Const SWP_NOREDRAW       As Long = &H8
Public Const SWP_NOACTIVATE     As Long = &H10
Public Const SWP_FRAMECHANGED   As Long = &H20      '/* The frame changed: send WM_NCCALCSIZE */
Public Const SWP_SHOWWINDOW     As Long = &H40
Public Const SWP_HIDEWINDOW     As Long = &H80
Public Const SWP_NOCOPYBITS     As Long = &H100
Public Const SWP_NOOWNERZORDER  As Long = &H200
Public Const SWP_NOSENDCHANGING As Long = &H400     '/* Don't send WM_WINDOWPOSCHANGING */
Public Const SWP_DRAWFRAME      As Long = SWP_FRAMECHANGED
Public Const SWP_NOREPOSITION   As Long = SWP_NOOWNERZORDER

Public Const HWND_TOPMOST = -1
Public Const HWND_NOTOPMOST = -2

Public Const WM_DRAWCLIPBOARD = &H308
Public Const GWL_WNDPROC = (-4)

Public Type tagMSG
    hWnd    As Long
    message As Long
    wParam  As Long
    lParam  As Long
    time    As Long
    pt      As Long
    #If Mac Then
        lPrivate As Long
    #End If
End Type

Public Type POINTAPI
    x As Long
    y As Long
End Type

Public Type Msg
    hWnd As Long
    message As Long
    wParam As Long
    lParam As Long
    time As Long
    pt As POINTAPI
End Type


Public Declare Function FreeLibrary Lib "kernel32" _
                                (ByVal hLibModule As Long) As Long
                                
Public Declare Function LoadLibrary Lib "kernel32" Alias "LoadLibraryA" _
                                (ByVal lpLibFileName As String) As Long
                                
Public Declare Function GetProcAddress Lib "kernel32" _
                                (ByVal hModule As Long, ByVal lpProcName As String) As Long
                                
Public Declare Function CallWindowProc Lib "user32" Alias "CallWindowProcA" _
                                (ByVal lpPrevWndFunc As Long, _
                                 ByVal hWnd As Long, _
                                 ByVal Msg As Any, _
                                 ByVal wParam As Any, _
                                 ByVal lParam As Any) As Long
                                 
Public Declare Function GetWindowsDirectory Lib "kernel32" Alias "GetWindowsDirectoryA" _
                                (ByVal lpBuffer As String, _
                                 ByVal nSize As Long) As Long
                                 
Public Declare Function EnumWindows Lib "user32" _
                                (ByVal lpEnumFunc As Long, _
                                 ByVal lParam As Long) As Boolean
                                 
Public Declare Function SetWindowLong Lib "user32" Alias "SetWindowLongA" _
                                (ByVal hWnd As Long, _
                                 ByVal nIndex As Long, _
                                 ByVal dwNewLong As Long) As Long
                                 
Public Declare Function SetClipboardViewer Lib "user32" _
                                (ByVal hWnd As Long) As Long
                                
Public Declare Function GetMessage Lib "user32" Alias "GetMessageA" _
                                (lpMsg As Msg, _
                                 ByVal hWnd As Long, _
                                 ByVal wMsgFilterMin As Long, _
                                 ByVal wMsgFilterMax As Long) As Long
                                 
Public Declare Function TranslateMessage Lib "user32" (lpMsg As Msg) As Long

Public Declare Function DispatchMessage Lib "user32" Alias "DispatchMessageA" _
                                (lpMsg As Msg) As Long
                                
Public Declare Function GetClassLong Lib "user32" Alias "GetClassLongA" _
                                (ByVal hWnd As Long, _
                                 ByVal nIndex As Integer) As Long
                                 
Public Declare Function GetMenuState Lib "user32" (ByVal hMenu As Long, ByVal uId As Integer, ByVal uFlags As Integer) As Long

Public Declare Function GetMenu Lib "user32" (ByVal hWnd As Long) As Long

Public Sub HookForm(F As Form)
  '//  PrevProc = SetWindowLong(F.hWnd, GWL_WNDPROC, AddressOf aWindowProc)
End Sub

Public Sub UnHookForm(F As Form)
    SetWindowLong F.hWnd, GWL_WNDPROC, PrevProc
    PrevProc = 0
End Sub

''Public Function aWindowProc(ByVal hWnd As Long, ByVal uMsg As Long, ByVal wParam As Long, ByVal lParam As Long) As Long
''
''    If CALL_ProcessTWMessage(uMsg, hWnd) = 1 Then
''        Exit Function
''    End If
'''
''    aWindowProc = CallWindowProc(PrevProc, hWnd, uMsg, wParam, lParam)
''
''End Function


    
'//    frmTest.ImagXpress1.CropBorder 0.9, 2    '블랙밴드제거
'//    frmTest.ImagXpress1.Deskew               '기울기보정

Public Function TW_CallBack(ByVal hWnd As Long, ByVal uMsg As Long, ByVal wParam As Long, ByVal lParam As Long) As Long

    '// 화면에 이미지 표시 -- wParam : 이미지 메모리 버퍼 주소
'''''    frmTest.ImagXpress1.hDib = wParam

    frmTest.ImagXpress1.FileName = App.Path + "\TEMP.JPG"
    
    If lParam = -77 Then
        MsgBox "Not 24bit"
    ElseIf lParam = -88 Then
        MsgBox "Fail LoadImage"
    ElseIf lParam = -1 Then
        MsgBox "Fail Recognition"
    End If
    
'''''    '// JPG 파일로 저장
'''''    frmTest.ImagXpress1.SaveFileName = App.Path + "\TEMP.JPG"
'''''    frmTest.ImagXpress1.SaveFileType = FT_JPG
'''''    frmTest.ImagXpress1.SaveFile
    

End Function

