#include <iostream>
#include <sstream>

#include <Kinect.h>
#include <opencv2\opencv.hpp>

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
    IColorFrameReader* colorFrameReader = nullptr;
    std::vector<BYTE> colorBuffer;

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
            throw std::runtime_error("Kinectが開けません");
        }

        // カラーリーダーを取得する
        ComPtr<IColorFrameSource> colorFrameSource;
        ERROR_CHECK( kinect->get_ColorFrameSource( &colorFrameSource ) );
        ERROR_CHECK( colorFrameSource->OpenReader( &colorFrameReader ) );

        // バッファーを作成する
        colorBuffer.resize( 1920 * 1080 * 4 );
    }

    void run()
    {
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
        updateColorFrame();
    }

    // カラーフレームの更新
    void updateColorFrame()
    {
        // フレームを取得する
        ComPtr<IColorFrame> colorFrame;
        auto ret = colorFrameReader->AcquireLatestFrame( &colorFrame );
        if ( ret == S_OK ){
            // BGRAの形式でデータを取得する
            ERROR_CHECK( colorFrame->CopyConvertedFrameDataToArray(
                colorBuffer.size(), &colorBuffer[0], ColorImageFormat_Bgra ) );

            // スマートポインタを使ってない場合は、自分でフレームを解放する
            // colorFrame->Release();
        }
    }

    void draw()
    {
        // カラーデータを表示する
        cv::Mat colorImage( 1080, 1920, CV_8UC4, &colorBuffer[0] );
        cv::imshow( "Color Image", colorImage );
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
