using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Microsoft.Kinect;

namespace KinectV2
{
    /// <summary>
    /// MainWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class MainWindow : Window
    {
        KinectSensor kinect;

        ColorFrameReader colorFrameReader;
        FrameDescription colorFrameDesc;
        byte[] colorBuffer;

        public MainWindow()
        {
            InitializeComponent();
        }

        private void Window_Loaded( object sender, RoutedEventArgs e )
        {
            try {
                kinect = KinectSensor.GetDefault();
                if ( kinect == null ) {
                    throw new Exception("Kinectを開けません");
                }

                kinect.Open();

                // カラー画像の情報を作成する(BGRAフォーマット)
                colorFrameDesc = kinect.ColorFrameSource.CreateFrameDescription( ColorImageFormat.Bgra );
                colorBuffer = new byte[colorFrameDesc.Width * colorFrameDesc.Height * colorFrameDesc.BytesPerPixel];

                // カラーリーダを開く
                colorFrameReader = kinect.ColorFrameSource.OpenReader();
                colorFrameReader.FrameArrived += colorFrameReader_FrameArrived;
            }
            catch ( Exception ex ) {
                MessageBox.Show( ex.Message );
                Close();
            }
        }

        private void Window_Closing( object sender, System.ComponentModel.CancelEventArgs e )
        {
            // 終了処理
            if ( colorFrameReader != null ) {
                colorFrameReader.Dispose();
                colorFrameReader = null;
            }

            if ( kinect != null ) {
                kinect.Close();
                kinect = null;
            }
        }

        void colorFrameReader_FrameArrived( object sender, ColorFrameArrivedEventArgs e )
        {
            // カラーフレームを取得する
            using ( var colorFrame = e.FrameReference.AcquireFrame() ) {
                if ( colorFrame == null ) {
                    return;
                }

                // BGRAデータを取得する
                colorFrame.CopyConvertedFrameDataToArray( colorBuffer, ColorImageFormat.Bgra );

                // ビットマップにする
                ImageColor.Source = BitmapSource.Create( colorFrameDesc.Width, colorFrameDesc.Height, 96, 96,
                    PixelFormats.Bgra32, null, colorBuffer, colorFrameDesc.Width * (int)colorFrameDesc.BytesPerPixel );
            }
        }
    }
}
