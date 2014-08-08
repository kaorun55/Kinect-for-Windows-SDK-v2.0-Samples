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

    // BodyIndex
    IBodyIndexFrameReader* bodyIndexFrameReader = nullptr;
    int BodyIndexWidth;
    int BodyIndexHeight;
    std::vector<BYTE> bodyIndexBuffer;

    // Body
    IBodyFrameReader* bodyFrameReader = nullptr;
    IBody* bodies[6];

    // Audio
    IAudioBeamFrameReader* audioBeamFrameReader;
    float beamAngle;

    // ビーム方向のTrackingIdとそのインデックス
    UINT64 audioTrackingId = (UINT64)-1;
    int audioTrackingIndex = -1;


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


        // ボディリーダーを取得する
        ComPtr<IBodyFrameSource> bodyFrameSource;
        ERROR_CHECK( kinect->get_BodyFrameSource( &bodyFrameSource ) );
        ERROR_CHECK( bodyFrameSource->OpenReader( &bodyFrameReader ) );

        // ボディインデックスリーダーを取得する
        ComPtr<IBodyIndexFrameSource> bodyIndexFrameSource;
        ERROR_CHECK( kinect->get_BodyIndexFrameSource( &bodyIndexFrameSource ) );
        ERROR_CHECK( bodyIndexFrameSource->OpenReader( &bodyIndexFrameReader ) );

        // ボディインデックスの解像度を取得する
        ComPtr<IFrameDescription> bodyIndexFrameDescription;
        ERROR_CHECK( bodyIndexFrameSource->get_FrameDescription( &bodyIndexFrameDescription ) );
        bodyIndexFrameDescription->get_Width( &BodyIndexWidth );
        bodyIndexFrameDescription->get_Height( &BodyIndexHeight );

        // バッファーを作成する
        bodyIndexBuffer.resize( BodyIndexWidth * BodyIndexHeight );

        // オーディオを開く
        ComPtr<IAudioSource> audioSource;
        ERROR_CHECK( kinect->get_AudioSource( &audioSource ) );

        ERROR_CHECK( audioSource->OpenReader( &audioBeamFrameReader ) );
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

    // データの更新処理
    void update()
    {
        updateAudioFrame();
        updateBodyFrame();
        updateBodyIndexFrame();
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

        // ビーム方向にいる人の数を取得する
        UINT32 count = 0;
        ERROR_CHECK( audioBeamSubFrame->get_AudioBodyCorrelationCount( &count ) );
        if ( count == 0 ){
            audioTrackingId = (UINT64)-1;
            return;
        }

        // ビーム方向の人のTrackingIdを取得する
        ComPtr<IAudioBodyCorrelation> audioBodyCorrelation;
        ERROR_CHECK( audioBeamSubFrame->GetAudioBodyCorrelation( 0, &audioBodyCorrelation ) );

        ERROR_CHECK( audioBodyCorrelation->get_BodyTrackingId( &audioTrackingId ) );
    }

    // ボディフレームの更新
    void updateBodyFrame()
    {
        // フレームを取得する
        ComPtr<IBodyFrame> bodyFrame;
        auto ret = bodyFrameReader->AcquireLatestFrame( &bodyFrame );
        if ( ret == S_OK ){
            // データを取得する
            ERROR_CHECK( bodyFrame->GetAndRefreshBodyData( 6, &bodies[0] ) );
        }
    }

    // ボディインデックスフレームの更新
    void updateBodyIndexFrame()
    {
        // フレームを取得する
        ComPtr<IBodyIndexFrame> bodyIndexFrame;
        auto ret = bodyIndexFrameReader->AcquireLatestFrame( &bodyIndexFrame );
        if ( ret != S_OK ){
            return;
        }

        // データを取得する
        ERROR_CHECK( bodyIndexFrame->CopyFrameDataToArray( bodyIndexBuffer.size(), &bodyIndexBuffer[0] ) );
    }

    void draw()
    {
        cv::Mat image = cv::Mat::zeros( BodyIndexHeight, BodyIndexWidth, CV_8UC4 );


        // ビーム方向の人のインデックスを探す
        audioTrackingIndex = -1;

        if ( audioTrackingId != (UINT64)-1 ){
            for ( int i = 0; i < 6; ++i ){
                UINT64 trackingId = 0;
                bodies[i]->get_TrackingId( &trackingId );
                if ( trackingId == audioTrackingId ){
                    audioTrackingIndex = i;
                    break;
                }
            }
        }


        // ビーム方向の人に色付けする
        for ( int i = 0; i < BodyIndexWidth * BodyIndexHeight; ++i ){
            int index = i * 4;
            // 人がいれば255以外
            if ( bodyIndexBuffer[i] != 255 ){
                if ( bodyIndexBuffer[i] == audioTrackingIndex ){
                    image.data[index + 0] = 255;
                    image.data[index + 1] = 0;
                    image.data[index + 2] = 0;
                }
                else {
                    image.data[index + 0] = 0;
                    image.data[index + 1] = 0;
                    image.data[index + 2] = 255;
                }
            }
            else{
                image.data[index + 0] = 255;
                image.data[index + 1] = 255;
                image.data[index + 2] = 255;
            }
        }


        // ラジアンから度に変換する
        auto angle = beamAngle * 180 / 3.1416;

        // 線を回転させる(逆回転)
        auto alpha = 3.1416 / -angle;
        int offsetX = BodyIndexWidth / 2;
        int offsetY = BodyIndexHeight / 2;

        auto X2 = 0 * cos( alpha ) - offsetY * sin( alpha );
        auto Y2 = 0 * sin( alpha ) + offsetY * cos( alpha );

        // 回転させた線を描画する
        cv::line( image, cv::Point( offsetX, 0 ), cv::Point( offsetX + X2, Y2 ), 0, 10 );

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
