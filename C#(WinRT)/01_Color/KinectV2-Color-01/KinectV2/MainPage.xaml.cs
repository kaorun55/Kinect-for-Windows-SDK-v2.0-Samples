using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Popups;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;
using WindowsPreview.Kinect;

// 空白ページのアイテム テンプレートについては、http://go.microsoft.com/fwlink/?LinkId=234238 を参照してください

namespace KinectV2
{
    /// <summary>
    /// それ自体で使用できる空白ページまたはフレーム内に移動できる空白ページ。
    /// </summary>
    public sealed partial class MainPage : Page
    {
        KinectSensor kinect;

        ColorFrameReader colorFrameReader;
        FrameDescription colorFrameDesc;
        byte[] colorBuffer;
        WriteableBitmap colorBitmap;

        public MainPage()
        {
            this.InitializeComponent();
        }

        protected override void OnNavigatedTo( NavigationEventArgs e )
        {
            base.OnNavigatedTo( e );

            try {
                kinect = KinectSensor.GetDefault();
                kinect.Open();

                colorFrameDesc = kinect.ColorFrameSource.CreateFrameDescription( ColorImageFormat.Bgra );

                colorBitmap = new WriteableBitmap( colorFrameDesc.Width, colorFrameDesc.Height );
                ImageColor.Source = colorBitmap;

                colorBuffer = new byte[colorFrameDesc.Width * colorFrameDesc.Height * colorFrameDesc.BytesPerPixel];

                colorFrameReader = kinect.ColorFrameSource.OpenReader();
                colorFrameReader.FrameArrived += colorFrameReader_FrameArrived;
            }
            catch ( Exception ex ) {
                MessageDialog dlg = new MessageDialog( ex.Message );
                dlg.ShowAsync();
            }
        }

        void colorFrameReader_FrameArrived( ColorFrameReader sender, ColorFrameArrivedEventArgs args )
        {
            using ( var colorFrame = args.FrameReference.AcquireFrame() ) {
                colorFrame.CopyConvertedFrameDataToArray( colorBuffer, ColorImageFormat.Bgra );

                var stream = colorBitmap.PixelBuffer.AsStream();
                stream.Write( colorBuffer, 0, colorBuffer.Length );
                colorBitmap.Invalidate();
            }
        }
    }
}
