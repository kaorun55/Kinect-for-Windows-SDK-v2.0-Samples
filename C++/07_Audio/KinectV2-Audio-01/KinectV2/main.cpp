#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

#include <Kinect.h>
#include <conio.h>

// Visual Studio Professional以上を使う場合はCComPtrの利用を検討してください。
#include "ComPtr.h"
//#include <atlbase.h>

#include "WaveFile.h"

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

    std::vector<float> audioBuffer;
    UINT subFrameLengthInBytes = 0;

    WaveFile audioFile;

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

            if ( _kbhit() != 0 ){
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
        ERROR_CHECK( audioSource->get_SubFrameLengthInBytes( &subFrameLengthInBytes ) );

        // バッファを作成する
        audioBuffer.resize( subFrameLengthInBytes / sizeof( float ) );

        audioFile.Open( "audio.wav" );
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

        audioBeamSubFrame->CopyFrameDataToArray( subFrameLengthInBytes, (BYTE*)&audioBuffer[0] );

        audioFile.Write( &audioBuffer[0], subFrameLengthInBytes );
    }

    void draw()
    {
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
