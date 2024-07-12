//
// Created by lishi on 7/12/2024.
//

#ifndef TESTANDROIDCPP_GLTEXTUREINTERFACE_H
#define TESTANDROIDCPP_GLTEXTUREINTERFACE_H

#include <GLES3/gl32.h>
#ifdef __cplusplus
extern "C"{
#endif
    void startTestThread();
    void stopTestThread();
    GLuint getTexture(int id);
    void startService();
    void stopService();
#ifdef __cplusplus
};
#endif
#endif //TESTANDROIDCPP_GLTEXTUREINTERFACE_H
