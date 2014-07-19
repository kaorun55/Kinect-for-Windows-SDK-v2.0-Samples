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

        DepthFrameReader depthFrameReader;
        ushort[] depthBuffer;

        WriteableBitmap depthImage;
        Int32Rect depthRect;
        int depthStride;

        Point depthPoint;
        const int R = 20;

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

                // 表示のためのデータを作成
                var depthFrameDesc = kinect.DepthFrameSource.FrameDescription;
                depthImage = new WriteableBitmap( depthFrameDesc.Width, depthFrameDesc.Height,
                    96, 96, PixelFormats.Gray16, null );
                depthBuffer = new ushort[depthFrameDesc.LengthInPixels];
                depthRect = new Int32Rect( 0, 0, depthFrameDesc.Width, depthFrameDesc.Height );
                depthStride = (int)(depthFrameDesc.Width * depthFrameDesc.BytesPerPixel);

                depthPoint = new Point( depthFrameDesc.Width / 2, depthFrameDesc.Height / 2 );

                ImageDepth.Source = depthImage;

                // Depthリーダーを開く
                depthFrameReader = kinect.DepthFrameSource.OpenReader();
                depthFrameReader.FrameArrived += depthFrameReader_FrameArrived;

                kinect.Open();
            }
            catch ( Exception ex ) {
                MessageBox.Show( ex.Message );
                Close();
            }
        }

        private void Window_Closing( object sender, System.ComponentModel.CancelEventArgs e )
        {
            if ( depthFrameReader != null ) {
                depthFrameReader.Dispose();
                depthFrameReader = null;
            }

            if ( kinect != null ) {
                kinect.Close();
                kinect = null;
            }
        }

        void depthFrameReader_FrameArrived( object sender, DepthFrameArrivedEventArgs e )
        {
            using ( var depthFrame = e.FrameReference.AcquireFrame() ) {
                if ( depthFrame == null ) {
                    return;
                }

                // Depthデータを取得する
                depthFrame.CopyFrameDataToArray( depthBuffer );

                // 距離情報の表示を更新する
                UpdateDepthValue( depthFrame );

                // 画像化する
                for ( int i = 0; i < depthBuffer.Length; i++ ) {
                    depthBuffer[i] = (ushort)(depthBuffer[i] * 65535 / 8000);
                }

                depthImage.WritePixels( depthRect, depthBuffer, depthStride, 0 );
            }
        }

        private void UpdateDepthValue( DepthFrame depthFrame )
        {
            CanvasPoint.Children.Clear();

            // クリックしたポイントを表示する
            var ellipse = new Ellipse()
            {
                Width = R,
                Height = R,
                StrokeThickness = R / 4,
                Stroke = Brushes.Red,
            };
            Canvas.SetLeft( ellipse, depthPoint.X - (R / 2) );
            Canvas.SetTop( ellipse, depthPoint.Y - (R / 2) );
            CanvasPoint.Children.Add( ellipse );

            // クリックしたポイントのインデックスを計算する
            int depthindex =(int)((depthPoint.Y  * depthFrame.FrameDescription.Width) + depthPoint.X);

            // クリックしたポイントの距離を表示する
            var text = new TextBlock()
            {
                Text = string.Format( "{0}mm", depthBuffer[depthindex] ),
                FontSize = 20,
                Foreground = Brushes.Green,
            };
            Canvas.SetLeft( text, depthPoint.X );
            Canvas.SetTop( text, depthPoint.Y - R );
            CanvasPoint.Children.Add( text );
        }

        private void Window_MouseLeftButtonDown( object sender, MouseButtonEventArgs e )
        {
            depthPoint = e.GetPosition( this );
        }
    }
}
