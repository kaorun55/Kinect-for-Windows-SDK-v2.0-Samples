using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
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
        AudioBeamFrameReader audioBeamFrameReader;
        byte[] audioBuffer;

        string fileName = "KinectAudio.wav";

        WaveFile waveFile = new WaveFile();

        public MainWindow()
        {
            InitializeComponent();
        }

        private void Window_Loaded( object sender, RoutedEventArgs e )
        {
            try {
                // Kinectを開く
                kinect = KinectSensor.GetDefault();
                kinect.Open();

                // 音声バッファを作成する
                audioBuffer = new byte[kinect.AudioSource.SubFrameLengthInBytes];

                // 音声リーダーを開く
                audioBeamFrameReader = kinect.AudioSource.OpenReader();
                audioBeamFrameReader.FrameArrived += audioBeamFrameReader_FrameArrived;

                audioBeamFrameReader.IsPaused = true;
            }
            catch ( Exception ex ) {
                MessageBox.Show( ex.Message );
                Close();
            }
        }

        void audioBeamFrameReader_FrameArrived( object sender, AudioBeamFrameArrivedEventArgs e )
        {
            using ( var audioFrame = e.FrameReference.AcquireBeamFrames() ) {
                if ( audioFrame == null ) {
                    return;
                }

                var subFrame = audioFrame[0].SubFrames[0];
                subFrame.CopyFrameDataToArray( audioBuffer );

                waveFile.Write( audioBuffer );

                // (例)実際のデータは32bit IEEE floatデータなので変換する
                float audioData1 = BitConverter.ToSingle( audioBuffer, 0 );
                float audioData2 = BitConverter.ToSingle( audioBuffer, 4 );
                float audioData3 = BitConverter.ToSingle( audioBuffer, 8 );
            }
        }

        private void Window_Closing( object sender, System.ComponentModel.CancelEventArgs e )
        {
            if ( waveFile  != null ) {
                waveFile.Dispose();
                waveFile = null;
            }

            if ( audioBeamFrameReader != null ) {
                audioBeamFrameReader.Dispose();
                audioBeamFrameReader = null;
            }

            if ( kinect != null ) {
                kinect.Close();
                kinect = null;
            }
        }


        private void Button_Click( object sender, RoutedEventArgs e )
        {
            MediaWave.Source = null;
            waveFile.Open( fileName );
            audioBeamFrameReader.IsPaused = false;
        }

        private void Button_Click_1( object sender, RoutedEventArgs e )
        {
            audioBeamFrameReader.IsPaused = true;
            waveFile.Close();
        }

        private void Button_Click_2( object sender, RoutedEventArgs e )
        {
            waveFile.Close();
            audioBeamFrameReader.IsPaused = true;

            MediaWave.Source = new Uri( string.Format( "./{0}", fileName ), UriKind.Relative );
            MediaWave.Position = new TimeSpan();
            MediaWave.Play();
        }
    }
}
