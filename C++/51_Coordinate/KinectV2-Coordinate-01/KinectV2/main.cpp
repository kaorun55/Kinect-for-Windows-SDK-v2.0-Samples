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

    // アプリ状態
    int showState = 0;

    // Kinect
    IKinectSensor* kinect = nullptr;
    ICoordinateMapper *coordinateMapper;

    // Color
    IColorFrameReader* colorFrameReader = nullptr;
    std::vector<BYTE> colorBuffer;
    int colorWidth;
    int colorHeight;
    unsigned int colorBytesPerPixel;

    // Depth
    IDepthFrameReader* depthFrameReader = nullptr;
    std::vector<UINT16> depthBuffer;

    int depthWidth;
    int depthHeight;

    // BodyIndex
    IBodyIndexFrameReader* bodyIndexFrameReader = nullptr;
    std::vector<BYTE> bodyIndexBuffer;

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

        // 座標変換インタフェースを取得
        kinect->get_CoordinateMapper( &coordinateMapper );

        // フレームの初期化
        initializeColorFrame();
        initializeDepthFrame();
        initializeBodyIndexFrame();
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
            else if ( (key >> 16) == VK_RIGHT ){
                showState = (showState + 1) % 3;
            }
        }
    }

private:

    void initializeColorFrame()
    {
        // カラーリーダーを取得する
        ComPtr<IColorFrameSource> colorFrameSource;
        ERROR_CHECK( kinect->get_ColorFrameSource( &colorFrameSource ) );
        ERROR_CHECK( colorFrameSource->OpenReader( &colorFrameReader ) );

        // カラー画像のサイズを取得する
        ComPtr<IFrameDescription> colorFrameDescription;
        ERROR_CHECK( colorFrameSource->CreateFrameDescription(
            ColorImageFormat::ColorImageFormat_Bgra, &colorFrameDescription ) );
        ERROR_CHECK( colorFrameDescription->get_Width( &colorWidth ) );
        ERROR_CHECK( colorFrameDescription->get_Height( &colorHeight ) );
        ERROR_CHECK( colorFrameDescription->get_BytesPerPixel( &colorBytesPerPixel ) );

        // バッファーを作成する
        colorBuffer.resize( colorWidth * colorHeight * colorBytesPerPixel );
    }

    void initializeDepthFrame()
    {
        // Depthリーダーを取得する
        ComPtr<IDepthFrameSource> depthFrameSource;
        ERROR_CHECK( kinect->get_DepthFrameSource( &depthFrameSource ) );
        ERROR_CHECK( depthFrameSource->OpenReader( &depthFrameReader ) );

        // Depth画像のサイズを取得する
        ComPtr<IFrameDescription> depthFrameDescription;
        ERROR_CHECK( depthFrameSource->get_FrameDescription( &depthFrameDescription ) );
        ERROR_CHECK( depthFrameDescription->get_Width( &depthWidth ) );
        ERROR_CHECK( depthFrameDescription->get_Height( &depthHeight ) );

        // バッファーを作成する
        depthBuffer.resize( depthWidth * depthHeight );
    }

    void initializeBodyIndexFrame()
    {
        // ボディインデックスリーダーを取得する
        ComPtr<IBodyIndexFrameSource> bodyIndexFrameSource;
        ERROR_CHECK( kinect->get_BodyIndexFrameSource( &bodyIndexFrameSource ) );
        ERROR_CHECK( bodyIndexFrameSource->OpenReader( &bodyIndexFrameReader ) );

        // バッファーを作成する
        bodyIndexBuffer.resize( depthWidth * depthHeight );
    }

    // データの更新処理
    void update()
    {
        updateColorFrame();
        updateDepthFrame();
        updateBodyIndexFrame();
    }

    // カラーフレームの更新
    void updateColorFrame()
    {
        // フレームを取得する
        ComPtr<IColorFrame> colorFrame;
        auto ret = colorFrameReader->AcquireLatestFrame( &colorFrame );
        if ( ret != S_OK ){
            return;
        }

        // BGRAの形式でデータを取得する
        ERROR_CHECK( colorFrame->CopyConvertedFrameDataToArray(
            colorBuffer.size(), &colorBuffer[0], ColorImageFormat::ColorImageFormat_Bgra ) );
    }

    // Depthフレームの更新
    void updateDepthFrame()
    {
        // Depthフレームを取得する
        ComPtr<IDepthFrame> depthFrame;
        auto ret = depthFrameReader->AcquireLatestFrame( &depthFrame );
        if ( ret != S_OK ){
            return;
        }

        // データを取得する
        ERROR_CHECK( depthFrame->CopyFrameDataToArray( depthBuffer.size(), &depthBuffer[0] ) );
    }

    // ボディインデックスフレームの更新
    void updateBodyIndexFrame()
    {
        // フレームを取得する
        ComPtr<IBodyIndexFrame> bodyIndexFrame;
        auto ret = bodyIndexFrameReader->AcquireLatestFrame( &bodyIndexFrame );
        if ( ret == S_OK ){
            // データを取得する
            ERROR_CHECK( bodyIndexFrame->CopyFrameDataToArray( bodyIndexBuffer.size(), &bodyIndexBuffer[0] ) );
        }
    }

    void draw()
    {
        cv::Mat colorImage( colorHeight, colorWidth, CV_8UC4, &colorBuffer[0] );

        std::vector<DepthSpacePoint> depthSpace( colorWidth * colorHeight );
        coordinateMapper->MapColorFrameToDepthSpace( depthBuffer.size(), &depthBuffer[0], depthSpace.size(), &depthSpace[0] );

        // Depth
        if ( showState == 0 ) {
            for ( int i = 0; i < colorWidth * colorHeight; ++i ){
                int depthX = (int)depthSpace[i].X;
                int depthY = (int)depthSpace[i].Y;
                if ( (depthX < 0) || (512 <= depthX) || (depthY < 0) || (424 <= depthY) ){
                    continue;
                }

                int depthIndex = (depthY * depthWidth) + depthX;
                int depth = ~(uchar)((depthBuffer[depthIndex] * 255) / 8000);

                // Depthで上書きする
                int colorIndex = i * 4;
                colorImage.data[colorIndex + 0] = depth;
                colorImage.data[colorIndex + 1] = depth;
                colorImage.data[colorIndex + 2] = depth;
            }
        }
        // BodyIndex
        else if ( showState == 1 ){
            for ( int i = 0; i < colorWidth * colorHeight; ++i ){
                int depthX = (int)depthSpace[i].X;
                int depthY = (int)depthSpace[i].Y;
                if ( (depthX < 0) || (512 <= depthX) || (depthY < 0) || (424 <= depthY) ){
                    continue;
                }

                int depthIndex = (depthY * depthWidth) + depthX;
                int bodyIndex = bodyIndexBuffer[depthIndex];

                // 人を検出した位置だけ色を消す
                if ( bodyIndex != 255 ){
                    int colorIndex = i * 4;
                    colorImage.data[colorIndex + 0] = 255;
                    colorImage.data[colorIndex + 1] = 255;
                    colorImage.data[colorIndex + 2] = 255;
                }
            }
        }
        // BodyIndex(背景除去)
        else {
            // 表示用の画像データ
            cv::Mat showImage = cv::Mat::zeros( colorHeight, colorWidth, CV_8UC4 );

            for ( int i = 0; i < colorWidth * colorHeight; ++i ){
                int depthX = (int)depthSpace[i].X;
                int depthY = (int)depthSpace[i].Y;
                if ( (depthX < 0) || (512 <= depthX) || (depthY < 0) || (424 <= depthY) ){
                    continue;
                }

                int depthIndex = (depthY * depthWidth) + depthX;
                int bodyIndex = bodyIndexBuffer[depthIndex];

                // 人を検出した位置だけ色を付ける
                if ( bodyIndex != 255 ){
                    int colorIndex = i * 4;
                    showImage.data[colorIndex + 0] = colorImage.data[colorIndex + 0];
                    showImage.data[colorIndex + 1] = colorImage.data[colorIndex + 1];
                    showImage.data[colorIndex + 2] = colorImage.data[colorIndex + 2];
                }
            }

            // 表示用の画像を差し換える
            colorImage = showImage;
        }

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
