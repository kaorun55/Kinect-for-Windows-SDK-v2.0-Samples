using System;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using Microsoft.Kinect;

namespace KinectV2
{
    /// <summary>
    /// MainWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class MainWindow : Window
    {
        KinectSensor kinect;

        InfraredFrameReader infraredFrameReader;
        FrameDescription infraredFrameDesc;

        public MainWindow()
        {
            InitializeComponent();
        }

        private void Window_Loaded( object sender, RoutedEventArgs e )
        {
            try {
                // Kinectを開く
                kinect = KinectSensor.GetDefault();
                if ( kinect == null ) {
                    throw new Exception("Kinectを開けません");
                }

                kinect.Open();

                // 赤外線画像の情報を取得する
                infraredFrameDesc = kinect.InfraredFrameSource.FrameDescription;

                // 赤外線リーダーを開く
                infraredFrameReader = kinect.InfraredFrameSource.OpenReader();
                infraredFrameReader.FrameArrived += infraredFrameReader_FrameArrived;
            }
            catch ( Exception ex ) {
                MessageBox.Show( ex.Message );
                Close();
            }
        }

        private void Window_Closing( object sender, System.ComponentModel.CancelEventArgs e )
        {
            // 終了処理
            if ( infraredFrameReader != null ) {
                infraredFrameReader.Dispose();
                infraredFrameReader = null;
            }

            if ( kinect != null ) {
                kinect.Close();
                kinect = null;
            }
        }

        void infraredFrameReader_FrameArrived( object sender, InfraredFrameArrivedEventArgs e )
        {
            // カラーフレームを取得する
            using ( var colorFrame = e.FrameReference.AcquireFrame() ) {
                if ( colorFrame == null ) {
                    return;
                }

                // 赤外線画像データを取得する
                var infraredBuffer = new ushort[infraredFrameDesc.Width * infraredFrameDesc.Height];
                colorFrame.CopyFrameDataToArray( infraredBuffer );

                // ビットマップにする
                ImageColor.Source = BitmapSource.Create( infraredFrameDesc.Width, infraredFrameDesc.Height, 96, 96,
                    PixelFormats.Gray16, null, infraredBuffer, infraredFrameDesc.Width * (int)infraredFrameDesc.BytesPerPixel );
            }
        }
    }
}
