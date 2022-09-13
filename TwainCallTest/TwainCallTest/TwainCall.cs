using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace TwainCallTest
{
    public class TwainCall
    {
        //public delegate int callback1(int hWnd, int uMsg, int wParam, int lParam);
        //public delegate int callback2(int hWnd, int uMsg, int wParam, int lParam);

        //[DllImport("TwainCall.dll")]
        //public static extern int CALL_InitInstance(int hInstance, callback1 _CallBackMem, callback2 _CallBackFile);

        [DllImport("TwainCall.dll")]
        public static extern int CALL_InitInstance(IntPtr hInstance, callback1 _CallBackFile, callback2 _CallBackMem);

        public delegate void callback1(int hWnd, int uMsg, int wParam, int lParam);
        public delegate void callback2(int hWnd, int uMsg, int wParam, int lParam);

        [DllImport("user32.dll", EntryPoint = "SetWindowLongW")]
        public static extern IntPtr SetWindowLongPtr32(IntPtr hWnd, int nIndex, IntPtr dwNewLong);

        [DllImport("user32.dll")]
        public static extern int SendMessage(int hWnd, IntPtr msg, int wParam, int lParam);

        [DllImport("user32.dll")]
        public static extern IntPtr GetActiveWindow();

        [DllImport("TwainCall.dll")]
        public static extern int CALL_SelectSource();

        [DllImport("TwainCall.dll")]
        public static extern int CALL_StringSource(string szProductName);

        [DllImport("TwainCall.dll")]
        public static extern int CALL_TWAppAquire(int ShowUI, string ImgDir, string ImgPrefix, int fiFileFormat, int optFlag);

        [DllImport("TwainCall.dll")]
        public static extern int CALL_TWAppAquireFixedName(int ShowUI, string szFileName, int optFlag);

        [DllImport("TwainCall.dll")]
        public static extern int CALL_SetCap_SetParm();

        [DllImport("TwainCall.dll")]
        public static extern int CALL_SetCap_DivScan();

        [DllImport("TwainCall.dll")]
        public static extern int CALL_StartSetup();

        [DllImport("TwainCall.dll")]
        public static extern int CALL_SetProperty(int cap, int val);

        [DllImport("TwainCall.dll")]
        public static extern int CALL_ChangeCount(int _count);

        [DllImport("TwainCall.dll")]
        public static extern IntPtr CALL_Get_WinProc();

        [DllImport("TwainCall.dll")]
        public static extern int CALL_Set_PrevProc(IntPtr prevproc);

        [DllImport("TwainCall.dll")]
        public static extern void CALL_EjectCtl_MSG_SET(int EjectInstruct);

        [DllImport("TwainCall.dll")]
        public static extern void CALL_StopFeeding();

        [DllImport("TwainCall.dll")]
        public static extern void CALL_Set_Ejection();

        [DllImport("TwainCall.dll")]
        public static extern void CALL_TWAppQuit();

        [DllImport("TwainCall.dll")]
        public static extern void CALL_EjectNumbering_MSG_SET(int NumberingFlg,string NumberingChar);

        [DllImport("TwainCall_SaveIMG.dll")]
        public static extern void CALL_SaveIMGFromHANDLE(int pDIB, string szSaveFile);
    }
}
