/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <cassert>
#include <pthread.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <signal.h>
#include <sys/time.h>
#include <math.h>
#include <unistd.h>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QFontDatabase>
#include <QFont>
#include "MiracastAppCommon.hpp"
#include "MiracastGraphicsDelegate.hpp"
#include "EssosEGLInternal.hpp"
#include "MiracastAppLogging.hpp"

namespace MiracastApp{
namespace Graphics{

#define RED_SIZE (8)
#define GREEN_SIZE (8)
#define BLUE_SIZE (8)
#define ALPHA_SIZE (8)
#define DEPTH_SIZE (24)

#define BG_IMAGE_PATH "/package/bg.jpg"
#define SPINNER_IMAGE_PATH "/package/spinner.png"

#define ESS_RENDER_THREAD_NAME "EssRenderTh"
#define kESSRunLoopPeriod 16

#define SKY_BG_PURPLE_COLOR 0xFF1D0E1F
#define BLACK_COLOR 0xFF000000

#define BACKGROUND_SCREEN_COLOR SKY_BG_PURPLE_COLOR

#define BG_COLOR_ALPHA_F ((BACKGROUND_SCREEN_COLOR >> 24) / 255.0)
#define BG_COLOR_RED_F (((BACKGROUND_SCREEN_COLOR >> 16) & 0xFF) / 255.0)
#define BG_COLOR_GREEN_F (((BACKGROUND_SCREEN_COLOR >> 8) & 0xFF) / 255.0)
#define BG_COLOR_BLUE_F ((BACKGROUND_SCREEN_COLOR & 0xFF) / 255.0)

#define VIRTUAL_DISPLAY_WIDTH 1920
#define VIRTUAL_DISPLAY_HEIGHT 1080

#define BUTTON_WIDTH 200
#define BUTTON_HEIGHT 50
#define BUTTON_Y_OFFSET 50

#define TITLE_WIDTH 800
#define TITLE_HEIGHT 100

#define DESCRIPTION_X_OFFSET 120
#define DESCRIPTION_Y_OFFSET 20

#define DESCRIPTION_WIDTH (TITLE_WIDTH + (DESCRIPTION_X_OFFSET * 2))
#define DESCRIPTION_HEIGHT 100

#define ENTIRE_TITLE_WIDTH (DESCRIPTION_WIDTH)
#define ENTIRE_TITLE_HEIGHT (TITLE_HEIGHT + DESCRIPTION_HEIGHT + DESCRIPTION_Y_OFFSET + BUTTON_HEIGHT + BUTTON_Y_OFFSET)

#define TITLE_START_X ((VIRTUAL_DISPLAY_WIDTH - ENTIRE_TITLE_WIDTH) >> 1)
#define TITLE_START_Y ((VIRTUAL_DISPLAY_HEIGHT - ENTIRE_TITLE_HEIGHT) >> 1)

#define DESCRIPTION_START_X (TITLE_START_X - DESCRIPTION_X_OFFSET)
#define DESCRIPTION_START_Y (TITLE_START_Y + TITLE_HEIGHT + DESCRIPTION_Y_OFFSET)

#define BUTTON_START_X (TITLE_START_X + ((TITLE_WIDTH - BUTTON_WIDTH) >> 1))
#define BUTTON_START_Y (DESCRIPTION_START_Y + DESCRIPTION_HEIGHT + BUTTON_Y_OFFSET)

NativeDisplayType display;
EssosRenderThread * EssosRenderThread::_mEssRenderTh = nullptr;
MiracastGraphicsDelegate* MiracastGraphicsDelegate::mInstance = nullptr;
pthread_mutex_t MiracastGraphicsDelegate::_mRenderMutex;

static GLuint program;
static GLuint textTexture = 0;
static GLuint okButtonTexture = 0;
static GLuint bgTexture = 0;
static GLuint spinnerTexture = 0;
int bgW = 0, bgH = 0;
int spinnerAngle = 0;
std::chrono::steady_clock::time_point lastSpinnerUpdate = std::chrono::steady_clock::now();
int windowWidth = 1920;
int windowHeight = 1080;

static std::map<std::string, std::map<std::string, std::string>> localizedStrings = {
    {"en_US", {
        {"title", "Screen Mirroring to Your TV"},
        {"description", "Find and select screen mirroring on your phone, tablet or computer. Then\n select [%s] to view your device's screen on your TV."},
        {"connecting", "Connecting to "},
        {"mirroring", "Mirroring to "},
        {"error_info", "Let's try that again"},
        {"error_description", "Something went wrong and [%s] couldn't be mirrored to this TV.\nPlease try again from your device.\n Error: %s"},
        {"ok", "OK"}
    }},
    {"es_US", {
        {"title", "Duplicación de pantalla en tu televisor"},
        {"description", "Busca y selecciona la función de duplicación de pantalla en tu teléfono, tableta o computadora. Luego\n Seleccione [%s] para ver la pantalla de su dispositivo en su televisor."},
        {"connecting", "Conectando a "},
        {"mirroring", "Duplicando a "},
        {"error_info", "Intentémoslo de nuevo"},
        {"error_description", "Algo salió mal y [%s] no se pudo duplicar en este televisor.\nInténtalo de nuevo desde tu dispositivo.\nCódigo: %s"},
        {"ok", "Aceptar"}
    }}
};

#if 0
static GLuint spinner_loadTexture(const char* filename)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 4); // Load PNG with 4 channels (RGBA)
    if (!image) {
        std::cerr << "Failed to load texture: " << filename << std::endl;
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(image);
    return texture;
}

static void spinner_drawSpinner(GLuint texture, float angle)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);

    glPushMatrix();
    glTranslatef(0.5f, 0.5f, 0.0f); // Move to the center of the screen
    glRotatef(angle, 0.0f, 0.0f, 1.0f); // Rotate around the Z-axis
    glTranslatef(-0.5f, -0.5f, 0.0f); // Move back to the original position

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-0.5f, -0.5f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f( 0.5f, -0.5f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f( 0.5f,  0.5f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-0.5f,  0.5f);
    glEnd();

    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
}

statin int spinner_main()
{
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Spinner Example", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Load the spinner texture
    GLuint spinnerTexture = loadTexture("spinner.png");
    if (spinnerTexture == 0) {
        glfwTerminate();
        return -1;
    }

    float angle = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        // Update the spinner angle
        angle += 2.0f; // Rotate by 2 degrees per frame
        if (angle >= 360.0f) {
            angle -= 360.0f;
        }

        // Draw the spinner
        drawSpinner(spinnerTexture, angle);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteTextures(1, &spinnerTexture);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
#endif

static void drawSpinnerWheel(void)
{
    static float spinnerAngle = 0.0f;
    auto now = std::chrono::steady_clock::now();

    MIRACASTLOG_TRACE("Entering ...");
    //if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSpinnerUpdate).count() > 50) //20fps {
    //if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSpinnerUpdate).count() > 100) //10fps {
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSpinnerUpdate).count() > 20) { // 50fps
        //spinnerAngle += 10.0f; // Rotate 10 degrees every frame
        spinnerAngle += 20.0f; // Rotate 10 degrees every frame
        if (spinnerAngle >= 360.0f) spinnerAngle -= 360.0f;
        lastSpinnerUpdate = now;
    }

    if (!spinnerTexture) return;

    glBindTexture(GL_TEXTURE_2D, spinnerTexture);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int size = 128;
    float cx = windowWidth / 2;
    float cy = windowHeight / 2 + 150;

    float radians = spinnerAngle * 3.14159f / 180.0f;
    float cosA = cos(radians);
    float sinA = sin(radians);

    GLfloat vertices[] = {
        cx + (-size/2)*cosA - (-size/2)*sinA, cy + (-size/2)*sinA + (-size/2)*cosA,
        cx + ( size/2)*cosA - (-size/2)*sinA, cy + ( size/2)*sinA + (-size/2)*cosA,
        cx + (-size/2)*cosA - ( size/2)*sinA, cy + (-size/2)*sinA + ( size/2)*cosA,
        cx + ( size/2)*cosA - ( size/2)*sinA, cy + ( size/2)*sinA + ( size/2)*cosA
    };

    GLfloat texCoords[] = {
        0.0f, 1.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 0.0f
    };

    GLint posAttr = glGetAttribLocation(program, "a_position");
    GLint texAttr = glGetAttribLocation(program, "a_texCoord");

    glEnableVertexAttribArray(posAttr);
    glVertexAttribPointer(posAttr, 2, GL_FLOAT, GL_FALSE, 0, vertices);

    glEnableVertexAttribArray(texAttr);
    glVertexAttribPointer(texAttr, 2, GL_FLOAT, GL_FALSE, 0, texCoords);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(posAttr);
    glDisableVertexAttribArray(texAttr);
    glDisable(GL_BLEND);
    MIRACASTLOG_TRACE("Exiting ...");
}

static QImage prepareQImageByText(const QString& text, int w, int h, bool isButton, uint8_t fontSize, Qt::GlobalColor bgcolor, Qt::GlobalColor penColor)
{
    MIRACASTLOG_TRACE("Entering ...");
    MIRACASTLOG_VERBOSE("text[%s] w=%d h=%d isButton=%d fontSize=%d bgcolor=%x penColor=%x", text.toStdString().c_str(), w, h, isButton, fontSize, bgcolor, penColor);

    QImage img(w, h, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    
    int fontId = QFontDatabase::addApplicationFont("/usr/share/fonts/ttf/LiberationSans-Regular.ttf");
    if (fontId != -1) {
        MIRACASTLOG_VERBOSE("Font LiberationSans loaded.fontId=%d\n", fontId);
    }
    else if ((fontId = QFontDatabase::addApplicationFont("/usr/share/fonts/Cinecav_Mono.ttf")) != -1)
    {
        MIRACASTLOG_VERBOSE("Font load failed, using fallback.fontId=%d\n", fontId);
    }
    else
    {
        MIRACASTLOG_VERBOSE("Fonts LiberationSans and Cinecav_Mono load failed.fontId =%d\n", fontId);
    }
    QString family = QFontDatabase::applicationFontFamilies(0).at(0);
    QFont font(family, fontSize);

    QPainter p(&img);
    p.setPen(penColor);
    p.setRenderHint(QPainter::Antialiasing);
    p.setFont(font);

    // Determine drawing area
    QRect rect = img.rect();

    if (!isButton) {
        p.fillRect(rect, bgcolor);
    }
    else{
        p.setBrush(bgcolor);
        p.setPen(penColor);
        int radius = 12;
        p.drawRoundedRect(rect, radius, radius);
    }

    p.drawText(img.rect(), Qt::AlignCenter | Qt::TextWordWrap, text);
    p.end();
    MIRACASTLOG_TRACE("Exiting ...");
    return img;
}

static GLuint prepareTextureFromQImage(const QImage &img, int &w, int &h)
{
    MIRACASTLOG_TRACE("Entering ...");
    w = img.width();
    h = img.height();
    MIRACASTLOG_VERBOSE("w=%d h=%d", w, h);
    QImage glImg = img.convertToFormat(QImage::Format_RGBA8888).mirrored();
    GLuint tex;
    glGenTextures(1, &tex);
    MIRACASTLOG_VERBOSE("tex=%d", tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, glImg.bits());
    MIRACASTLOG_TRACE("Exiting ...");
    return tex;
}

static QImage loadImage(const QString& path)
{
    MIRACASTLOG_TRACE("Entering ...");
    QImage img;
    img.load(path);
    if (img.isNull())
    {
        MIRACASTLOG_ERROR("Failed to load image from path: %s", path.toStdString().c_str());
    }
    else{
        MIRACASTLOG_VERBOSE("img.width()=%d img.height()=%d", img.width(), img.height());
    }
    MIRACASTLOG_TRACE("Exiting ...");
    return img;
}

static void loadBackgroundTexture()
{
    MIRACASTLOG_TRACE("Entering ...");
    MIRACASTLOG_VERBOSE("windowWidth=%d windowHeight=%d", windowWidth, windowHeight);
    QImage bg = loadImage(BG_IMAGE_PATH);
    if (bg.isNull())
    {
        MIRACASTLOG_ERROR("Failed to load background image");
        return;
    }
    MIRACASTLOG_VERBOSE("bg.width()=%d bg.height()=%d", bg.width(), bg.height());
    bg = bg.scaled(windowWidth, windowHeight, Qt::IgnoreAspectRatio);
    bgTexture = prepareTextureFromQImage(bg, bgW, bgH);
    MIRACASTLOG_VERBOSE("bgTexture=%d bgW=%d bgH=%d", bgTexture, bgW, bgH);
    MIRACASTLOG_TRACE("Exiting ...");
}

static void loadSpinnerTexture()
{
    MIRACASTLOG_TRACE("Entering ...");
    QImage spinner = loadImage(SPINNER_IMAGE_PATH);
    if (spinner.isNull())
    {
        MIRACASTLOG_ERROR("Failed to load spinner image");
        return;
    }
    //spinner = spinner.scaled(128, 128, Qt::IgnoreAspectRatio);
    spinnerTexture = prepareTextureFromQImage(spinner, bgW, bgH);
    MIRACASTLOG_VERBOSE("spinnerTexture=%d bgW=%d bgH=%d", spinnerTexture, bgW, bgH);
    MIRACASTLOG_TRACE("Exiting ...");
}

static void drawTexture(GLuint texture, int texWidth, int texHeight, int centerX, int centerY)
{
    MIRACASTLOG_TRACE("Entering ...");
    glBindTexture(GL_TEXTURE_2D, texture);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if 0
    int halfW = texWidth / 2;
    int halfH = texHeight / 2;

    GLfloat vertices[] = {
        (GLfloat)(centerX - halfW), (GLfloat)(centerY - halfH),
        (GLfloat)(centerX + halfW), (GLfloat)(centerY - halfH),
        (GLfloat)(centerX - halfW), (GLfloat)(centerY + halfH),
        (GLfloat)(centerX + halfW), (GLfloat)(centerY + halfH)
    };
#else
    GLfloat vertices[] = {
        (GLfloat)(centerX), (GLfloat)(centerY),
        (GLfloat)(centerX+texWidth), (GLfloat)(centerY),
        (GLfloat)(centerX), (GLfloat)(centerY+texHeight),
        (GLfloat)(centerX+texWidth), (GLfloat)(centerY+texHeight)
    };
#endif

    GLfloat texCoords[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
    };

    GLint posAttr = glGetAttribLocation(program, "a_position");
    GLint texAttr = glGetAttribLocation(program, "a_texCoord");

    glEnableVertexAttribArray(posAttr);
    glVertexAttribPointer(posAttr, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(texAttr);
    glVertexAttribPointer(texAttr, 2, GL_FLOAT, GL_FALSE, 0, texCoords);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(posAttr);
    glDisableVertexAttribArray(texAttr);

    MIRACASTLOG_TRACE("Exiting ...");
}

static void drawBackground(void)
{
    MIRACASTLOG_TRACE("Entering ...");
    if (!bgTexture)
    {
        MIRACASTLOG_VERBOSE("Loading background texture");
        loadBackgroundTexture();
    }
    if (bgTexture) {
        drawTexture(bgTexture, bgW, bgH, 0, 0);
        MIRACASTLOG_VERBOSE("drawBackground: bgTexture=%d bgW=%d bgH=%d", bgTexture, bgW, bgH);
    }
    MIRACASTLOG_TRACE("Exiting ...");
}

EssosRenderThread::EssosRenderThread()
    : mRunning(true)
{
    MIRACASTLOG_TRACE("Entering ...");
	mEssCtx = nullptr;
    if (pthread_mutex_init(&mRunningMutex, NULL) != 0) {
        MIRACASTLOG_VERBOSE("mutex init has failed\n");
    }
    EssosRenderThread::_mEssRenderTh = this;
    sigact.sa_handler= EssosRenderThread::signalHandler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags= SA_RESETHAND;
    sigaction(SIGINT, &sigact, NULL);
    sigaction(SIGSEGV, &sigact, NULL);

    if(pthread_create(&EssRenderT_Id, nullptr, &EssosRenderThread::run, nullptr )){
        MIRACASTLOG_VERBOSE("Failure to create Thread name\n");
    }
    else{
        if(pthread_setname_np(EssRenderT_Id, ESS_RENDER_THREAD_NAME )){
            MIRACASTLOG_VERBOSE("Failure to set Thread name\n");
        }
    }
    MIRACASTLOG_TRACE("Exiting ...");
}

void EssosRenderThread::setRunning(bool running)
{
    MIRACASTLOG_TRACE("Entering ...");
    pthread_mutex_lock(&mRunningMutex);
    mRunning = running;
    pthread_mutex_unlock(&mRunningMutex);
    MIRACASTLOG_TRACE("Exiting ...");
}

void* EssosRenderThread::run(void *arg)
{
    MiracastAppScreenState  LastAppScreenState = APPSCREEN_STATE_DEFAULT,
                            currentAppScreenState = APPSCREEN_STATE_DEFAULT;

    bool error = _mEssRenderTh->BuildEssosContext();
    if(!error){
        if( !EssContextStart(_mEssRenderTh->mEssCtx)){
            error = true;
        }
        if(!error)
        {
            error = _mEssRenderTh->InitializeEGL();
            if(error)
            {
                MIRACASTLOG_ERROR("Unable to start rendering");
                _mEssRenderTh->setRunning(false);
            }
        }
    }

    struct timespec tspec;
    long long delay = 0, curr_time = 0, diff_time = 0, ess_evloop_last_ts = 0;
    std::string ttsVoiceMsg = "";
    while(_mEssRenderTh->isRunning())
    {
        currentAppScreenState = MiracastGraphicsDelegate::getInstance()->getAppScreenState();

        if ((LastAppScreenState != currentAppScreenState) ||
            (APPSCREEN_STATE_CONNECTING == currentAppScreenState))
        {
            ttsVoiceMsg.clear();
            MIRACASTLOG_VERBOSE(">>>> MiracastAppScreenState changed from %d to %d", LastAppScreenState, currentAppScreenState);
            LastAppScreenState = currentAppScreenState;
            switch (currentAppScreenState)
            {
                case APPSCREEN_STATE_WELCOME:
                case APPSCREEN_STATE_STOPPED:
                {
                    std::string welcomePageHeader = MiracastGraphicsDelegate::getInstance()->getWelcomePageHeader();
                    std::string welcomePageDescription = MiracastGraphicsDelegate::getInstance()->getWelcomePageDescription();

                    MIRACASTLOG_VERBOSE("Welcome screen");
                    drawBackground();
                    _mEssRenderTh->displayMessage(  welcomePageHeader, welcomePageDescription,"");
                    ttsVoiceMsg = welcomePageHeader + "\n\n" + welcomePageDescription;
                }
                break;
                case APPSCREEN_STATE_CONNECTING:
                {
                    std::string connectingPageHeader = MiracastGraphicsDelegate::getInstance()->getConnectingPageHeader();
                    MIRACASTLOG_VERBOSE("Connecting screen");
                    drawBackground();
                    _mEssRenderTh->displayMessage(  connectingPageHeader , "", "");
                    ttsVoiceMsg = connectingPageHeader;

                    drawSpinnerWheel();
                }
                break;
                case APPSCREEN_STATE_MIRRORING:
                {
                    std::string mirroringPageHeader = MiracastGraphicsDelegate::getInstance()->getMirroringPageHeader();
                    MIRACASTLOG_VERBOSE("Mirroring screen");
                    drawBackground();
                    _mEssRenderTh->displayMessage(  mirroringPageHeader, "", "");
                    ttsVoiceMsg = mirroringPageHeader;
                }
                break;
                case APPSCREEN_STATE_ERROR:
                {
                    std::string errorPageHeader = MiracastGraphicsDelegate::getInstance()->getErrorPageHeader();
                    std::string errorPageDescription = MiracastGraphicsDelegate::getInstance()->getErrorPageDescription();

                    MIRACASTLOG_VERBOSE("Error screen");
                    drawBackground();
                    _mEssRenderTh->displayMessage( errorPageHeader, errorPageDescription, MiracastGraphicsDelegate::getInstance()->getButtonText());
                    ttsVoiceMsg = errorPageHeader + "\n\n" + errorPageDescription;
                }
                break;
                case APPSCREEN_STATE_CONNECTED:
                {
                    MIRACASTLOG_VERBOSE("Connected screen");
                    _mEssRenderTh->fillColor(0.0, 0.0, 0.0, 0.0, false);
                }
                break;
                default:
                    break;
            }
            LastAppScreenState = currentAppScreenState;
            eglSwapBuffers(_mEssRenderTh->mDisplay, _mEssRenderTh->mSurface);
            EssContextUpdateDisplay( _mEssRenderTh->mEssCtx );
            if (!ttsVoiceMsg.empty())
            {
                MiracastGraphicsDelegate::getInstance()->updateTTSVoiceCommand(ttsVoiceMsg);
            }
        }
        EssContextRunEventLoopOnce(_mEssRenderTh->mEssCtx);
        clock_gettime(CLOCK_MONOTONIC,&tspec);
        curr_time = tspec.tv_sec * 1000 + tspec.tv_nsec / 1e6;
        diff_time = curr_time - ess_evloop_last_ts;
        delay = (long long)kESSRunLoopPeriod - diff_time;
        if(delay > 0 && delay <= kESSRunLoopPeriod )
        {
            usleep(delay*1000);
        }
        ess_evloop_last_ts = curr_time;
    }
    eglMakeCurrent(_mEssRenderTh->mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(_mEssRenderTh->mDisplay, _mEssRenderTh->mContext);
    eglDestroySurface(_mEssRenderTh->mDisplay, _mEssRenderTh->mSurface);
	_mEssRenderTh->DestroyNativeWindow();
    EssContextStop(_mEssRenderTh->mEssCtx);
    _mEssRenderTh->mContext = 0;
    MIRACASTLOG_TRACE("Exiting ...");
    return nullptr;
}

void EssosRenderThread::signalHandler(int signum)
{
    MIRACASTLOG_TRACE("Entering ...");
    MIRACASTLOG_VERBOSE("signalHandler: signum %d\n",signum);
    _mEssRenderTh->setRunning(false);
    MIRACASTLOG_TRACE("Exiting ...");
}

static EssTerminateListener terminateListener =
{
    //terminated
    [](void* /*data*/) {
        _Exit(-1);
    }
};

void EssosRenderThread::OnKeyPressed(unsigned int keyCode)
{
	MIRACASTLOG_VERBOSE(" <com.miracastapp.graphics> OnKeyPressed() key_code: %02X translated code: ?\n", keyCode);
    if (( 0x1C == keyCode) && ( APPSCREEN_STATE_ERROR == MiracastGraphicsDelegate::getInstance()->getAppScreenState())) {
        MIRACASTLOG_INFO("OK button pressed");
        MiracastGraphicsDelegate::getInstance()->setAppScreenState(APPSCREEN_STATE_WELCOME, "", "");
    }
}

void EssosRenderThread::OnKeyReleased(unsigned int key){
	MIRACASTLOG_VERBOSE(" <com.miracastapp.graphics> OnKeyReleased() key_code: %02X translated code: ?\n", key);
}

void EssosRenderThread::OnKeyRepeat(unsigned int key)
{
	MIRACASTLOG_VERBOSE(" <com.miracastapp.graphics> OnKeyRepeat() key_code: %02X translated code: ?\n", key);
}

EssKeyListener EssosRenderThread::keyListener =
{
  // keyPressed
  [](void* data, unsigned int key) { reinterpret_cast<EssosRenderThread*>(data)->OnKeyPressed(key); },
  // keyReleased
  [](void* data, unsigned int key) { reinterpret_cast<EssosRenderThread*>(data)->OnKeyReleased(key); },
  // keyRepeat
  [](void* data, unsigned int key) { reinterpret_cast<EssosRenderThread*>(data)->OnKeyRepeat(key); }
};

//this callback is invoked with the new width and height values
void displaySize(void *userData, int width, int height )
{
	EssCtx *ctx = (EssCtx*)userData;
	EssContextResizeWindow( ctx, width, height );
}

static EssSettingsListener settingsListener=
{
   displaySize
};

bool EssosRenderThread::BuildEssosContext()
{
    MIRACASTLOG_TRACE("Entering ...");
	mEssCtx = EssContextCreate();
	bool error = false;
	if(!mEssCtx) {
		MIRACASTLOG_ERROR("<com.miracastapp.graphics> Failed to create Essos context\n");
        MIRACASTLOG_TRACE("Exiting ...");
	    return false;
	}
	else {
		if(!EssContextSetUseWayland(mEssCtx, true)){
			error = true;
		}
		if (!EssContextSetSettingsListener(mEssCtx, mEssCtx, &settingsListener))
		{
			error = true;
		}
		if ( !EssContextSetTerminateListener(mEssCtx, 0, &terminateListener) ) {
    		error = true;
  		}
		if ( !EssContextSetKeyListener(mEssCtx, 0, &keyListener) ) {
                error = true;
        }
		if ( !EssContextInit(mEssCtx) )
    	{            	
			error = true;
        } 
		
		if (!EssContextGetEGLDisplayType(mEssCtx, &display))
		{
			error = true;
		}
		mDisplay = eglGetDisplay((NativeDisplayType)display);
        if (mDisplay == EGL_NO_DISPLAY)
        {
            MIRACASTLOG_ERROR("Unable to get EGL display 0x%x", eglGetError());
            error = true;
        }
		assert(mDisplay != EGL_NO_DISPLAY);

		if ( !EssContextGetDisplaySize( mEssCtx, &gDisplayWidth, &gDisplayHeight ) )
		{
			error= true;
		}
		if ( !EssContextSetInitialWindowSize( mEssCtx, gDisplayWidth, gDisplayHeight) ){
			error = true;
		}
		MIRACASTLOG_VERBOSE("<com.miracastapp.graphics> display %dx%d\n", gDisplayWidth, gDisplayHeight);

		if ( !EssContextCreateNativeWindow(mEssCtx, gDisplayWidth, gDisplayHeight, &mNativewindow) ) {
			error = true;
		}
	}	
        if ( error )
        {
            const char *detail = EssContextGetLastErrorDetail(mEssCtx);
           MIRACASTLOG_VERBOSE("<com.miracastapp.graphics>BuildEssosContext(): Essos error: '%s'\n",detail);
        }	
    MIRACASTLOG_TRACE("Exiting ...");
	return error;
}

void EssosRenderThread::DestroyNativeWindow()
{
    MIRACASTLOG_TRACE("Entering ...");
    if (0 != mNativewindow)
    {
        if ( !EssContextDestroyNativeWindow(mEssCtx, mNativewindow) ) {
            const char *detail = EssContextGetLastErrorDetail(mEssCtx);
            MIRACASTLOG_VERBOSE("<com.miracastapp.graphics> DestroyNativeWindow() Essos error: '%s'\n", detail);
        }
        mNativewindow = 0;
    }
    MIRACASTLOG_TRACE("Exiting ...");
}

bool EssosRenderThread::InitializeEGL()
{
    MIRACASTLOG_TRACE("Entering ...");
	EGLint major, minor;
	if (eglInitialize(mDisplay, &major, &minor) != EGL_TRUE)
	{
        MIRACASTLOG_ERROR("Unable to Initialize EGL API 0x%x",eglGetError());
        MIRACASTLOG_TRACE("Exiting ...");
		return true;
	}
    if(eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
        MIRACASTLOG_ERROR("Unable to bind EGL API 0x%x",eglGetError());
        MIRACASTLOG_TRACE("Exiting ...");
        return true;
    }
	/*
	 * Get number of available configurations
	 */
	EGLint configCount;
	MIRACASTLOG_VERBOSE("Vendor: %s\n",eglQueryString(mDisplay, EGL_VENDOR));
    MIRACASTLOG_VERBOSE("version: %d.%d\n", major, minor);

	if (eglGetConfigs(mDisplay, nullptr, 0, &configCount))
	{
		EGLConfig eglConfigs[configCount];

		EGLint attributes[] = {
			EGL_RED_SIZE, RED_SIZE,
			EGL_GREEN_SIZE, GREEN_SIZE,
			EGL_BLUE_SIZE, BLUE_SIZE,
			EGL_ALPHA_SIZE, 8,
			EGL_DEPTH_SIZE, DEPTH_SIZE,
			EGL_STENCIL_SIZE, 0,
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL_BUFFER_SIZE, EGL_DONT_CARE,
			EGL_NONE};
	
		MIRACASTLOG_VERBOSE("Configs: %d\n", configCount);
		/*
		 * Get a list of configurations that meet or exceed our requirements
		 */
		if (eglChooseConfig(mDisplay, attributes, eglConfigs, configCount, &configCount))
		{	/*
			 * Choose a suitable configuration
			 */
			int index = 0;
			while (index < configCount)
			{
				EGLint redSize, greenSize, blueSize, alphaSize, depthSize;
				eglGetConfigAttrib(mDisplay, eglConfigs[index], EGL_RED_SIZE, &redSize);
				eglGetConfigAttrib(mDisplay, eglConfigs[index], EGL_GREEN_SIZE, &greenSize);
				eglGetConfigAttrib(mDisplay, eglConfigs[index], EGL_BLUE_SIZE, &blueSize);
				eglGetConfigAttrib(mDisplay, eglConfigs[index], EGL_ALPHA_SIZE, &alphaSize);
				eglGetConfigAttrib(mDisplay, eglConfigs[index], EGL_DEPTH_SIZE, &depthSize);
				MIRACASTLOG_VERBOSE("depthSize = %d\n",depthSize);

				if ((redSize == RED_SIZE) && (greenSize == GREEN_SIZE) && (blueSize == BLUE_SIZE) && (alphaSize == ALPHA_SIZE) && (depthSize >= DEPTH_SIZE))
				{
					break;
				}

				index++;
			}
			if (index < configCount)
			{
				mConfig = eglConfigs[index];
				EGLint attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2 /* ES2 */, EGL_NONE};
				MIRACASTLOG_VERBOSE(" Config choosen: %d\n", index);

				/*
				 * Create an EGL context
				 */
				mContext = eglCreateContext(mDisplay, mConfig, EGL_NO_CONTEXT, attributes);
				MIRACASTLOG_VERBOSE("Context created");
			}
		}
	}
	
	MIRACASTLOG_VERBOSE("Extentions: %s\n", eglQueryString(mDisplay, EGL_EXTENSIONS));
   	/*
	 * Create a window surface
	 */
	mSurface = eglCreateWindowSurface(
		mDisplay,
		mConfig,
		mNativewindow,
		nullptr);

	if (mSurface == EGL_NO_SURFACE)
        MIRACASTLOG_ERROR("Unable to create EGL window surface 0x%x", eglGetError());
    else
        MIRACASTLOG_VERBOSE("EGL window surface created");

    assert(EGL_NO_SURFACE != mSurface);

	EGLint surfaceType(0);
	eglQuerySurface(mDisplay, mSurface, EGL_WIDTH, &gDisplayWidth);
	eglQuerySurface(mDisplay, mSurface, EGL_HEIGHT, &gDisplayHeight);
	eglGetConfigAttrib(mDisplay, mConfig, EGL_SURFACE_TYPE, &surfaceType);
	MIRACASTLOG_VERBOSE("EGL window surface is %dx%d\n", gDisplayWidth, gDisplayHeight);
    if(eglMakeCurrent(mDisplay, mSurface, mSurface, mContext) != EGL_TRUE) {
        MIRACASTLOG_ERROR(" Unable to make EGL context current 0x%x", eglGetError());
        MIRACASTLOG_TRACE("Exiting ...");
        return true;
    }

	eglSwapInterval(mDisplay, 1);

    InitializeQtRendering();

    MIRACASTLOG_TRACE("Exiting ...");
    return false;
}

void EssosRenderThread::InitializeQtRendering(void)
{
    int argc = 1;
    char* argv[] = { (char*)"MiracastApp", nullptr };

    MIRACASTLOG_TRACE("Entering ...");

    //qputenv("QT_QPA_FONTDIR", "/usr/share/fonts");
    //QGuiApplication app(argc, argv);

    const char* vertShaderSrc = R"(
        attribute vec2 a_position;
        attribute vec2 a_texCoord;
        varying vec2 v_texCoord;
        void main() {
            gl_Position = vec4(2.0 * a_position.x / 1920.0 - 1.0,
                                1.0 - 2.0 * a_position.y / 1080.0, 0.0, 1.0);
            v_texCoord = a_texCoord;
        }
    )";

    const char* fragShaderSrc = R"(
        precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D u_texture;
        void main() {
            gl_FragColor = texture2D(u_texture, v_texCoord);
        }
    )";

    auto compileShader = [](GLenum type, const char* src) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        return shader;
    };

    GLuint vert = compileShader(GL_VERTEX_SHADER, vertShaderSrc);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragShaderSrc);
    program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "u_texture"), 0);

    MIRACASTLOG_TRACE("Exiting ...");
}

void EssosRenderThread::displayMessage(const std::string& headerText, const std::string& bodyText, const std::string& buttonText)
{
    int texWidth, texHeight;
    MIRACASTLOG_TRACE("Entering ...");
    MIRACASTLOG_VERBOSE("headerText[%s] bodyText[%s] buttonText[%s]", headerText.c_str(), bodyText.c_str(), buttonText.c_str());
    if (!headerText.empty())
    {
        if (textTexture) glDeleteTextures(1, &textTexture);
        QImage img1 = prepareQImageByText(QString::fromStdString(headerText), TITLE_WIDTH, TITLE_HEIGHT, false, 38, Qt::transparent, Qt::white);
        textTexture = prepareTextureFromQImage(img1, texWidth, texHeight);
        drawTexture(textTexture, texWidth, texHeight, TITLE_START_X, TITLE_START_Y);
    }

    if (!bodyText.empty())
    {
        if (textTexture) glDeleteTextures(1, &textTexture);
        QImage img2 = prepareQImageByText(QString::fromStdString(bodyText), DESCRIPTION_WIDTH, DESCRIPTION_HEIGHT, false, 24, Qt::transparent, Qt::white);
        textTexture = prepareTextureFromQImage(img2, texWidth, texHeight);
        drawTexture(textTexture, texWidth, texHeight, DESCRIPTION_START_X, DESCRIPTION_START_Y);
    }

    if (!buttonText.empty())
    {
        if (textTexture) glDeleteTextures(1, &textTexture);
        QImage img3 = prepareQImageByText(QString::fromStdString(buttonText), BUTTON_WIDTH, BUTTON_HEIGHT, true, 24, Qt::white, Qt::black);
        textTexture = prepareTextureFromQImage(img3, texWidth, texHeight);
        drawTexture(textTexture, texWidth, texHeight, BUTTON_START_X, BUTTON_START_Y);
    }

    MIRACASTLOG_TRACE("Exiting ...");
}

MiracastGraphicsDelegate::MiracastGraphicsDelegate()
{
    const char *language = getenv("LANG");
    const char *friendlyName = getenv("DEVICE_FRIENDLYNAME");

    MIRACASTLOG_TRACE("Entering ...");

    mCurrentAppScreenState = APPSCREEN_STATE_DEFAULT;
	if (pthread_mutex_init(&_mRenderMutex, NULL) != 0) {
    	MIRACASTLOG_ERROR("<com.miracastapp.graphics> mutex init failed\n");
	}

    _mRDKTextToSpeech = RDKTextToSpeech::getInstance();
    if (!_mRDKTextToSpeech)
    {
        MIRACASTLOG_ERROR("Failed to create RDKTextToSpeech instance");
    }

    mLanguageCode = "en_US";
    if (language)
    {
        MIRACASTLOG_INFO("Current language is %s", language);
        setLanguageCode(language);
    }

    if (friendlyName)
    {
        MIRACASTLOG_INFO("Current friendly name is %s", friendlyName);
        setFriendlyName(friendlyName);
    }

    mButtonText = localizedStrings[mLanguageCode]["ok"];
    
    MIRACASTLOG_TRACE("Exiting ...");
}
MiracastGraphicsDelegate::~MiracastGraphicsDelegate() {
    MIRACASTLOG_TRACE("Entering ...");
    if ( _mRDKTextToSpeech )
    {
        RDKTextToSpeech::destroyInstance();
        _mRDKTextToSpeech = nullptr;
    }
    MIRACASTLOG_TRACE("Exiting ...");
}

MiracastGraphicsDelegate* MiracastGraphicsDelegate::getInstance()
{
    if(!mInstance) {
		mInstance = new MiracastGraphicsDelegate;
	}
    return mInstance;
}

void MiracastGraphicsDelegate::destroyInstance()
{
    MIRACASTLOG_TRACE("Entering ...");
    if(mInstance != NULL){
		delete mInstance;
		mInstance = NULL;
	}
    MIRACASTLOG_TRACE("Exiting ...");
}

void MiracastGraphicsDelegate::updateTTSVoiceCommand(const std::string &voiceMsg)
{
    MIRACASTLOG_TRACE("Entering ...");
    if (_mRDKTextToSpeech)
    {
        _mRDKTextToSpeech->speak(voiceMsg);
    }
    MIRACASTLOG_TRACE("Exiting ...");
}

void MiracastGraphicsDelegate::setAppScreenState(MiracastAppScreenState state, const std::string &deviceName, const std::string &errorCode)
{
    std::string result = "";
    switch (state)
    {
        case APPSCREEN_STATE_WELCOME:
        case APPSCREEN_STATE_STOPPED:
        {
            result = localizedStrings[mLanguageCode]["description"];
            size_t pos = result.find("%s");
            if (pos != std::string::npos) {
                result.replace(pos, 2, mFriendlyName);
            }
            mWelcomePageHeader = localizedStrings[mLanguageCode]["title"];
            mWelcomePageDescription = result;
        }
        break;
        case APPSCREEN_STATE_CONNECTING:
        {
            mConnectingPageHeader = localizedStrings[mLanguageCode]["connecting"] + deviceName + "...";
        }
        break;
        case APPSCREEN_STATE_MIRRORING:
        {
            mMirroringPageHeader = localizedStrings[mLanguageCode]["mirroring"] + deviceName + "...";
        }
        break;
        case APPSCREEN_STATE_ERROR:
        {
            result = localizedStrings[mLanguageCode]["error_description"];
            // Replace the first %s with FriendlyName
            size_t pos = result.find("%s");
            if (pos != std::string::npos) {
                result.replace(pos, 2, mFriendlyName);
            }
            // Replace the second %s with ErrorCode
            pos = result.find("%s");
            if (pos != std::string::npos) {
                result.replace(pos, 2, errorCode);
            }
            mErrorPageHeader = localizedStrings[mLanguageCode]["error_info"];
            mErrorPageDescription = result;
            mButtonText = localizedStrings[mLanguageCode]["ok"];
        }
        break;
        case APPSCREEN_STATE_CONNECTED:
        {

        }
        break;
        default:
            break;
    }
    MIRACASTLOG_INFO("App screen state changed [%d] -> [%d]", mCurrentAppScreenState, state);
    mCurrentAppScreenState = state;
}

bool MiracastGraphicsDelegate::initialize() 
{
    bool error = false;
    MIRACASTLOG_TRACE("Entering ...");
    pthread_mutex_lock(&_mRenderMutex);
    if ( !_mRenderThread )
    {
        _mRenderThread = new EssosRenderThread();
    }
	pthread_mutex_unlock(&_mRenderMutex);
	MIRACASTLOG_TRACE("Exiting ...");
    return !error;
}

void MiracastGraphicsDelegate::teardown() {
    MIRACASTLOG_TRACE("Entering ...");
    pthread_mutex_lock(&_mRenderMutex);
    if(_mRenderThread)
    {
        _mRenderThread->setRunning(false);
        pthread_join(_mRenderThread->EssRenderT_Id, nullptr);
    }
	pthread_mutex_unlock(&_mRenderMutex);
	pthread_mutex_destroy(&_mRenderMutex);
	MIRACASTLOG_VERBOSE("<com.miracastapp.graphics> teardown() completed\n");
    MIRACASTLOG_TRACE("Exiting ...");
}

void EssosRenderThread::fillColor(float alpha, float red, float green, float blue, bool swapBuffers )
{
    MIRACASTLOG_TRACE("Entering ...");

    // Set the clear color (RGBA)
    glClearColor(red, green, blue, alpha);

    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT);

    if (swapBuffers)
    {
        // Swap buffers to display the color
        if (eglSwapBuffers(mDisplay, mSurface) != EGL_TRUE) {
            MIRACASTLOG_ERROR("eglSwapBuffers failed with error: 0x%x", eglGetError());
        }
    }

    MIRACASTLOG_TRACE("Exiting ...");
}

}
}