#include <iostream>
#include <sstream>

#include <Kinect.h>
#include <opencv2\opencv.hpp>

#include <math.h>

// Visual Studio Professional以上を使う場合はCComPtrの利用を検討してください。
#include "ComPtr.h"
//#include <atlbase.h>

// 次のように使います
// ERROR_CHECK( ::GetDefaultKinectSensor( &kinect ) );
// 書籍での解説のためにマクロにしています。実際には展開した形で使うことを検討してください。
#define ERROR_CHECK( ret )  \
    if ( (ret) != S_OK ) {    \
        std::stringstream ss;	\
        ss << "failed " #ret " " << std::hex << ret << std::endl;			\
        throw std::runtime_error( ss.str().c_str() );			\
    }

class KinectApp
{
private:

    IKinectSensor* kinect = nullptr;
    IAudioBeamFrameReader* audioBeamFrameReader;

    float beamAngle;
    float beamAngleConfidence;

public:

    // 初期化
    void initialize()
    {
        // デフォルトのKinectを取得する
        ERROR_CHECK( ::GetDefaultKinectSensor( &kinect ) );

        // Kinectを開く
        ERROR_CHECK( kinect->Open() );

        BOOLEAN isOpen = false;
        ERROR_CHECK( kinect->get_IsOpen( &isOpen ) );
        if ( !isOpen ){
            throw std::runtime_error( "Kinectが開けません" );
        }

        initializeAudioSource();
    }

    void run()
    {
        std::cout << "キーを押すと終了します" << std::endl;

        while ( 1 ) {
            update();
            draw();

            auto key = cv::waitKey( 10 );
            if ( key == 'q' ){
                break;
            }
        }
    }

private:

    void initializeAudioSource()
    {
        // オーディオを開く
        ComPtr<IAudioSource> audioSource;
        ERROR_CHECK( kinect->get_AudioSource( &audioSource ) );

        ERROR_CHECK( audioSource->OpenReader( &audioBeamFrameReader ) );
    }

    // データの更新処理
    void update()
    {
        updateAudioFrame();
    }

    // オーディオフレームの更新
    void updateAudioFrame()
    {
        ComPtr<IAudioBeamFrameList> audioBeamFrameList;
        auto ret = audioBeamFrameReader->AcquireLatestBeamFrames( &audioBeamFrameList );
        if ( ret != S_OK ){
            return;
        }

        ComPtr<IAudioBeamFrame> audioBeamFrame;
        ERROR_CHECK( audioBeamFrameList->OpenAudioBeamFrame( 0, &audioBeamFrame ) );

        ComPtr<IAudioBeamSubFrame> audioBeamSubFrame;
        ERROR_CHECK( audioBeamFrame->GetSubFrame( 0, &audioBeamSubFrame ) );

        // 角度および角度の信頼性を取得する
        ERROR_CHECK( audioBeamSubFrame->get_BeamAngle( &beamAngle ) );
        ERROR_CHECK( audioBeamSubFrame->get_BeamAngleConfidence( &beamAngleConfidence ) );
    }

    void draw()
    {
        cv::Mat image = cv::Mat::zeros(480, 640, CV_8UC4);

        // ラジアンから度に変換する
        auto angle = beamAngle * 180 / 3.1416;

        // 線を回転させる(逆回転)
        auto alpha = 3.1416 / -angle;
        int offsetX = 320;
        int offsetY = 240;

        auto X2 = 0 * cos( alpha ) - offsetY * sin( alpha );
        auto Y2 = 0 * sin( alpha ) + offsetY * cos( alpha );

        // 回転させた線を描画する
        cv::line( image, cv::Point( offsetX, 0 ), cv::Point( offsetX + X2, Y2 ), cv::Scalar( 255, 255, 255 ), 10 );

        cv::imshow("AudioBeamAngle", image);
    }
};

void main()
{
    try {
        KinectApp app;
        app.initialize();
        app.run();
    }
    catch ( std::exception& ex ){
        std::cout << ex.what() << std::endl;
    }
}
