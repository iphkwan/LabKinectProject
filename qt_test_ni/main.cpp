#include <QtGui>
#include <QtCore>
#include "copenni.cpp"
#include "cskeletonitem.cpp"
#include "ckinectreader.cpp"

using namespace xn;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    COpenNI openni;
    if(!openni.Initial())
        return 1;//返回1表示不正常返回

    QGraphicsScene scene;
    QGraphicsView view(&scene);
    view.setWindowTitle("Test");
    view.resize(650, 540);//view的尺寸比图片的输出尺寸稍微大一点
    view.show();

    CKinectReader kinect_reader(openni, scene);
    kinect_reader.Start();

    return app.exec();
}
