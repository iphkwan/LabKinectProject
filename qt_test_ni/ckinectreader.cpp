#include <QtGui>
#include <QDebug>
#include <XnCppWrapper.h>
#include "copenni.cpp"  //要包含cpp文件，不能直接包含类
#include "cskeletonitem.cpp"
#include <iostream>

using namespace std;

class CKinectReader: public QObject
{
public:
    //构造函数,用构造函数中的变量给类的私有成员赋值
    CKinectReader(COpenNI &openni, QGraphicsScene &scene) : openni(openni), scene(scene) {
    }
    ~CKinectReader() {
        scene.removeItem(image_item);
        scene.removeItem(depth_item);
        delete [] p_depth_argb;
    }
    bool Start(int interval = 33) {
        openni.Start();//因为在调用CKinectReader这个类的之前会初始化好的,所以这里直接调用Start了
        image_item = scene.addPixmap(QPixmap());
        image_item->setZValue(1);
        depth_item = scene.addPixmap(QPixmap());
        depth_item->setZValue(2);
        openni.UpdateData();
        p_depth_argb = new uchar[4*openni.depth_metadata.XRes()*openni.depth_metadata.YRes()];
        startTimer(interval);//这里是继承QObject类，因此可以调用该函数
        return true;
    }
private:
    COpenNI &openni;    //定义引用同时没有初始化,因为在构造函数的时候用冒号来初始化
    QGraphicsScene &scene;
    QGraphicsPixmapItem *image_item;
    QGraphicsPixmapItem *depth_item;
    uchar *p_depth_argb;
    vector<CSkeletonItem*> skeletons;//CSkeletonItem类的使用在此处得到了体现

private:
    void timerEvent(QTimerEvent *) {

        openni.UpdateData();
        //这里使用const，是因为右边的函数返回的值就是const类型的
        const XnDepthPixel *p_depth_pixpel = openni.depth_metadata.Data();
        unsigned int size = openni.depth_metadata.XRes()*openni.depth_metadata.YRes();

        //找深度最大值点
        XnDepthPixel max_depth = *p_depth_pixpel;
        for(unsigned int i = 1; i < size; ++i)
            if(p_depth_pixpel[i] > max_depth )
                max_depth = p_depth_pixpel[i];

        //将深度图像格式归一化到0~255
        int idx = 0;
        for(unsigned int i = 1; i < size; ++i) {
            //一定要使用1.0f相乘，转换成float类型，否则该工程的结果会有错误,因为这个要么是0，要么是1，0的概率要大很多
            float fscale = 1.0f*(*p_depth_pixpel)/max_depth;
            if((*p_depth_pixpel) != 0) {
                p_depth_argb[idx++] = 255*(1-fscale);    //蓝色分量
                p_depth_argb[idx++] = 0; //绿色分量
                p_depth_argb[idx++] = 255*fscale;   //红色分量，越远越红
                p_depth_argb[idx++] = 255*(1-fscale); //距离越近，越不透明
            }
            else {
                p_depth_argb[idx++] = 0;
                p_depth_argb[idx++] = 0;
                p_depth_argb[idx++] = 0;
                p_depth_argb[idx++] = 255;
            }
            ++p_depth_pixpel;//此处的++p_depth_pixpel和p_depth_pixpel++是一样的
        }
        //往item中设置图像色彩数据
        image_item->setPixmap(QPixmap::fromImage(
                              QImage(openni.image_metadata.Data(), openni.image_metadata.XRes(), openni.image_metadata.YRes(),
                              QImage::Format_RGB888)));
        //往item中设置深度数据
        depth_item->setPixmap(QPixmap::fromImage(
                              QImage(p_depth_argb, openni.depth_metadata.XRes(), openni.depth_metadata.YRes()
                              , QImage::Format_ARGB32)));
        //读取骨骼信息
        UserGenerator &user_generator = openni.getUserGenerator();
        XnUInt16 users_num = user_generator.GetNumberOfUsers();//得到视野中人体的个数
        if(users_num > 0) {
            XnUserID *user_id = new XnUserID[users_num];//开辟users_num个XnUserID类型的内存空间,XnUserID其实就是一个XnUInt32类型
            user_generator.GetUsers(user_id, users_num);//将获取到的userid放入user_id指向的内存中
            unsigned int counter = 0;
            const SkeletonCapability &skeleton_capability = user_generator.GetSkeletonCap();//获取骨骼的capability
            for(int i = 0; i < users_num; i++) {
                if(skeleton_capability.IsTracking(user_id[i])) {
                    ++counter;
                    if(counter > skeletons.size()) { //跟踪中人体的数目大于视野中人体的数量时
                        CSkeletonItem *p_skeleton = new CSkeletonItem(user_id[i], openni);//重新创建一个骨架对象，并加入到骨架vector中
                        scene.addItem(p_skeleton);//在场景中显示该骨架
                        p_skeleton->setZValue(10);
                        skeletons.push_back(p_skeleton);
                    }
                    else
                        skeletons[counter-1]->user_id = user_id[i];
                    //更新对应人体的骨架信息
                    skeletons[counter-1]->UpdateSkeleton();
                   //调用该函数后boundingRect()函数就会一直在更新，所以paint()函数也在不断变化
                    skeletons[counter-1]->setVisible(true);
                }
            }
            //将其他没有使用的item设置为不显示
            for(unsigned int i = counter; i < skeletons.size(); ++i) {
                skeletons[i]->setVisible(false);
            }
            delete [] user_id;
        }
    }
};
