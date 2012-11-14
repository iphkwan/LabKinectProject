#ifndef CSKELETONITEM_CLASS
#define CSKELETONITEM_CLASS

#include <QtGui>
#include <XnCppWrapper.h>
#include "copenni.cpp"

class CSkeletonItem : public QGraphicsItem
{
public:
    /*构造函数*/
    CSkeletonItem(XnUserID &user_id, COpenNI& openni) : QGraphicsItem(), user_id(user_id),
                    openni(openni) {
        /*创建关节相连的二维表
        connections[i]表示第i条线(2个节点之间表示一条线),connections[i][0]和connections[i][1]分别表示
        第i条线的2个端点*/
        //头部和身体的2条线
        {
            connections[0][0] = 0;
            connections[0][1] = 1;
            connections[1][0] = 1;
            connections[1][1] = 2;
        }
        //左手的3条线
        {
            connections[2][0] = 1;
            connections[2][1] = 3;
            connections[3][0] = 3;
            connections[3][1] = 4;
            connections[4][0] = 4;
            connections[4][1] = 5;
        }
        //右手的3条线
        {
            connections[5][0] = 1;
            connections[5][1] = 6;
            connections[6][0] = 6;
            connections[6][1] = 7;
            connections[7][0] = 7;
            connections[7][1] = 8;
        }
        //左腿的3条线
        {
            connections[8][0] = 2;
            connections[8][1] = 9;
            connections[9][0] = 9;
            connections[9][1] = 10;
            connections[10][0] = 10;
            connections[10][1] = 11;
        }
        //右腿的3条线
        {
            connections[11][0] = 2;
            connections[11][1] = 12;
            connections[12][0] = 12;
            connections[12][1] = 13;
            connections[13][0] = 13;
            connections[13][1] = 14;
        }

    }
    /*更新skeleton里面的数据,分别获得15个节点的世界坐标，并转换成投影坐标*/
    void UpdateSkeleton() {
        XnPoint3D joints_realworld[15];
        joints_realworld[0] = getSkeletonPos(XN_SKEL_HEAD);
        joints_realworld[1] = getSkeletonPos(XN_SKEL_NECK);
        joints_realworld[2] = getSkeletonPos(XN_SKEL_TORSO);
        joints_realworld[3] = getSkeletonPos(XN_SKEL_LEFT_SHOULDER);
        joints_realworld[4] = getSkeletonPos(XN_SKEL_LEFT_ELBOW);
        joints_realworld[5] = getSkeletonPos(XN_SKEL_LEFT_HAND);
        joints_realworld[6] = getSkeletonPos(XN_SKEL_RIGHT_SHOULDER);
        joints_realworld[7] = getSkeletonPos(XN_SKEL_RIGHT_ELBOW);
        joints_realworld[8] = getSkeletonPos(XN_SKEL_RIGHT_HAND);
        joints_realworld[9] = getSkeletonPos(XN_SKEL_LEFT_HIP);
        joints_realworld[10] = getSkeletonPos(XN_SKEL_LEFT_KNEE);
        joints_realworld[11] = getSkeletonPos(XN_SKEL_LEFT_FOOT);
        joints_realworld[12] = getSkeletonPos(XN_SKEL_RIGHT_HIP);
        joints_realworld[13] = getSkeletonPos(XN_SKEL_RIGHT_KNEE);
        joints_realworld[14] = getSkeletonPos(XN_SKEL_RIGHT_FOOT);
        //将世界坐标系转换成投影坐标系，一定要使用深度信息的节点
        openni.getDepthGenerator().ConvertRealWorldToProjective(15, joints_realworld, joints_project);
    }
public:
    COpenNI& openni;
    XnUserID& user_id;//每个CSkeletonItem对应一个人体
    XnPoint3D joints_project[15];//15个关节点的坐标
    int connections[14][2];
 //   int connections[15][2];

private:
    XnPoint3D getSkeletonPos(XnSkeletonJoint joint_name) {
        XnSkeletonJointPosition pos;//关节点的坐标
        //得到指定关节名称的节点的坐标,保存在pos中
        openni.getUserGenerator().GetSkeletonCap().GetSkeletonJointPosition(user_id, joint_name, pos);
        return xnCreatePoint3D(pos.position.X, pos.position.Y, pos.position.Z);//以3维坐标的形式返回节点的坐标
    }
    //boudintRect函数的重写
    QRectF boundingRect() const {
        QRectF rect(joints_project[0].X, joints_project[0].Y, 0, 0);//定义一个矩形外围框，其长和宽都为0
        for(int i = 1; i < 15; i++) {
            //下面的代码是找出能够围住15个节点的最小矩形框
            //rect.left()等返回的是一个实数
            if(joints_project[i].X < rect.left()) { //小于矩形框左边点的横坐标时
                rect.setLeft(joints_project[i].X);
            }
            if(joints_project[i].X > rect.right()) {
                rect.setRight(joints_project[i].X);
            }
            if(joints_project[i].Y < rect.top()) {
                rect.setTop(joints_project[i].Y);
            }
            if(joints_project[i].Y > rect.bottom()) {
                rect.setBottom(joints_project[i].Y);
            }
        }
        return rect;
    }
    //重绘函数的重写
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) { //固定的参数形式
        //后面要画骨骼直接的连线，首先需要设置画笔
        QPen pen(QColor::fromRgb(0, 0, 255));//设置蓝色的画笔
        pen.setWidth(3);
        painter->setPen(pen);
        //画骨骼的线,总共是14条线
        for(unsigned int i = 0; i < 14; i++) {
            XnPoint3D &p1 = joints_project[connections[i][0]];
            XnPoint3D &p2 = joints_project[connections[i][1]];
            painter->drawLine(p1.X, p1.Y, p2.X, p2.Y);
        }
        painter->setPen(QPen(Qt::yellow, 3));
        //每个节点处画个小圆圈
        for(unsigned int  i = 0; i < 15; i++ ) {
            painter->drawEllipse(QPoint(joints_project[i].X, joints_project[i].Y), 5, 5);
        }
    }
};

#endif
