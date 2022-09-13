Attribute VB_Name = "Module1"
Public Type TW_IDENTITY
    Id              As Long             'TW_UINT32      typedef unsigned long
    ProtocolMajor   As Integer          'TW_UINT16      typedef unsigned short
    ProtocolMinor   As Integer          'TW_UINT16      typedef unsigned short
    SupportedGroups As Integer             'TW_UINT32      typedef unsigned long
    SupportedGroups2 As Integer
    Manufacturer(0 To 33)   As Byte     'TW_STR32       typedef char
    ProductFamily(0 To 33)  As Byte      'TW_STR32       typedef char
    ProductName(0 To 33)    As Byte      'TW_STR32       typedef char
End Type

Public Declare Function StructToPointer Lib "CallEntryWin32.dll" (ByRef A As TW_IDENTITY) As Long



