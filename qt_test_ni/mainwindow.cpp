#ifndef COPENNI_CLASS
#define COPENNI_CLASS

#include <XnCppWrapper.h>
#include <QtGui/QtGui>
#include <iostream>

using namespace xn;
using namespace std;

class COpenNI
{
public:
    ~COpenNI() {
        context.Release();//释放空间
    }
    bool Initial() {
        //初始化
        status = context.Init();
        if(CheckError("Context initial failed!")) {
            return false;
        }
        context.SetGlobalMirror(true);//设置镜像
        xmode.nXRes = 640;
        xmode.nYRes = 480;
        xmode.nFPS = 30;
        //产生颜色node
        status = image_generator.Create(context);
        if(CheckError("Create image generator  error!")) {
            return false;
        }
        //设置颜色图片输出模式
        status = image_generator.SetMapOutputMode(xmode);
        if(CheckError("SetMapOutputMdoe error!")) {
            return false;
        }
        //产生深度node
        status = depth_generator.Create(context);
        if(CheckError("Create depth generator  error!")) {
            return false;
        }
        //设置深度图片输出模式
        status = depth_generator.SetMapOutputMode(xmode);
        if(CheckError("SetMapOutputMdoe error!")) {
            return false;
        }
        //产生手势node
        status = gesture_generator.Create(context);
        if(CheckError("Create gesture generator error!")) {
            return false;
        }
        /*添加手势识别的种类*/
        gesture_generator.AddGesture("Wave", NULL);
        gesture_generator.AddGesture("click", NULL);
        gesture_generator.AddGesture("RaiseHand", NULL);
        gesture_generator.AddGesture("MovingHand", NULL);
        //产生人体node
        status = user_generator.Create(context);
        if(CheckError("Create gesturen generator error!")) {
            return false;
        }
        //视角校正
        status = depth_generator.GetAlternativeViewPointCap().SetViewPoint(image_generator);
        if(CheckError("Can't set the alternative view point on depth generator!")) {
            return false;
        }
        //设置有人进入视野的回调函数
        XnCallbackHandle new_user_handle;
        user_generator.RegisterUserCallbacks(CBNewUser, NULL, NULL, new_user_handle);
        user_generator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);//设定使用所有关节（共15个）
        //设置骨骼校正完成的回调函数
        XnCallbackHandle calibration_complete;
        user_generator.GetSkeletonCap().RegisterToCalibrationComplete(CBCalibrationComplete, NULL, calibration_complete);
        return true;
    }

    bool Start() {
        status = context.StartGeneratingAll();
        if(CheckError("Start generating error!")) {
            return false;
        }
        return true;
    }

    bool UpdateData() {
        status = context.WaitNoneUpdateAll();
        if(CheckError("Update date error!")) {
            return false;
        }
        //获取数据
        image_generator.GetMetaData(image_metadata);
        depth_generator.GetMetaData(depth_metadata);

        return true;
    }
    //得到色彩图像的node
    ImageGenerator& getImageGenerator() {
        return image_generator;
    }
    //得到深度图像的node
    DepthGenerator& getDepthGenerator() {
        return depth_generator;
    }
    //得到人体的node
    UserGenerator& getUserGenerator() {
        return user_generator;
    }

public:
    DepthMetaData depth_metadata;
    ImageMetaData image_metadata;
    GestureGenerator gesture_generator;//外部要对其进行回调函数的设置，因此将它设为public类型

private:
    //该函数返回真代表出现了错误，返回假代表正确
    bool CheckError(const char* error) {
        if(status != XN_STATUS_OK ) {
            QMessageBox::critical(NULL, error, xnGetStatusString(status));
            cerr << error << ": " << xnGetStatusString( status ) << endl;
            return true;
        }
        return false;
    }
    //有人进入视野时的回调函数
    static void XN_CALLBACK_TYPE CBNewUser(UserGenerator &generator, XnUserID user, void *p_cookie) {
        //得到skeleton的capability,并调用RequestCalibration函数设置对新检测到的人进行骨骼校正
        generator.GetSkeletonCap().RequestCalibration(user, true);
    }
    //完成骨骼校正的回调函数
    static void XN_CALLBACK_TYPE CBCalibrationComplete(SkeletonCapability &skeleton,
                                                       XnUserID user, XnCalibrationStatus calibration_error, void *p_cookie) {
        if(calibration_error == XN_CALIBRATION_STATUS_OK) {
            skeleton.StartTracking(user);//骨骼校正完成后就开始进行人体跟踪了
        }
        else {
            UserGenerator *p_user = (UserGenerator*)p_cookie;
            skeleton.RequestCalibration(user, true);//骨骼校正失败时重新设置对人体骨骼继续进行校正
        }
    }

private:
    XnStatus    status;
    Context     context;
    DepthGenerator  depth_generator;
    ImageGenerator  image_generator;
    UserGenerator user_generator;
    XnMapOutputMode xmode;
};

#endif
