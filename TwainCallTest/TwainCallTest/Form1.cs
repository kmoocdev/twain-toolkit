using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.IO;
using System.Diagnostics;

namespace TwainCallTest
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        int sCnt = 0;

        [DllImport("TwainCall.dll")]
        static extern int CALL_InitInstance(IntPtr hInstance, callback1 _CallBackMem, callback2 _CallBackFile);
        delegate void callback1(int hWnd, int uMsg, int wParam, int lParam);
        delegate void callback2(int hWnd, int uMsg, int wParam, int lParam);


        [DllImport("user32.dll")]
        static extern IntPtr GetActiveWindow();

        [DllImport("user32.dll")]
        public static extern int SendMessage(int hWnd, IntPtr msg, int wParam, int lParam);

        [DllImport("user32.dll")]
        static extern int SetWindowLong(IntPtr hWnd, int nIndex, uint dwNewLong);


        [DllImport("user32.dll", EntryPoint = "SetWindowLongW")]
        private static extern IntPtr SetWindowLongPtr32(IntPtr hWnd, int nIndex, IntPtr dwNewLong);


        private void Form1_Load(object sender, EventArgs e)
        {
            
        }

        void TW_CallBackMem(int hWnd, int uMsg, int wParam, int lParam)
        {
            if(lParam==45)
            {
                
            }
            else if (lParam == 0)
            {
                string fnm = Application.StartupPath + @"\IMAGE\" + DateTime.Now.ToString("HHmmssfff") + ".jpg";
                TwainCall.CALL_SaveIMGFromHANDLE(wParam, fnm);
                pictureBox1.Image = Bitmap.FromFile(fnm);
                Application.DoEvents();                
                if(checkBox2.Checked==true) TwainCall.CALL_EjectNumbering_MSG_SET((checkBox1.Checked ? 1 : 0), DateTime.Now.ToString("yyyyMMddHHmmssfff"));
                else TwainCall.CALL_EjectNumbering_MSG_SET((checkBox1.Checked ? 1 : 0), "AB-DEFGHIJKL1 345");
                TwainCall.CALL_EjectCtl_MSG_SET(0);
                TwainCall.CALL_Set_Ejection();
            }
            
        }

        void TW_CallBackFile(int hWnd, int uMsg, int wParam, int lParam)
        {
            TwainCall.CALL_EjectCtl_MSG_SET(1);
            TwainCall.CALL_Set_Ejection();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            int k = 0;
            sCnt = 0;
            k = TwainCall.CALL_ChangeCount(0);
            k = TwainCall.CALL_StringSource("HT-4165 Image Scanner");
            if (k == 0)
            {
                TwainCall.CALL_StartSetup();
                TwainCall.CALL_SetCap_SetParm();
                TwainCall.CALL_SetCap_DivScan();
                TwainCall.CALL_SetProperty(4103, 1);
                TwainCall.CALL_SetProperty(4115, 0);
                TwainCall.CALL_TWAppAquire(0, string.Empty, string.Empty, 0, 0);
            }
            else
            {
                MessageBox.Show("설정오류");
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            int k = 0;
            TwainCall.callback1 callback1 = new TwainCall.callback1(TW_CallBackFile);
            TwainCall.callback2 callback2 = new TwainCall.callback2(TW_CallBackMem);

            IntPtr hWnd = TwainCall.GetActiveWindow();

            IntPtr dllwinproc;

            TwainCall.CALL_InitInstance(hWnd, callback1, callback2);

            dllwinproc = TwainCall.CALL_Get_WinProc();

            IntPtr PrevProc = TwainCall.SetWindowLongPtr32(hWnd, -4, dllwinproc);

            TwainCall.CALL_Set_PrevProc(PrevProc);


        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
            TwainCall.CALL_TWAppQuit();
        }
    }
}
