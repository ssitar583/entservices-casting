// Updated version of Miracast UI with working state transitions, proper OK button rendering, and debug logs.

#include <essos.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QFontDatabase>
#include <QFont>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <string>
#include <iostream>
#include <unistd.h>
#include <map>

enum class AppScreenState {
    Stopped,
    PreConnect,
    Connecting,
    Connected,
    Error
};

EssCtx* essosCtx = nullptr;
EGLDisplay eglDisplay;
EGLSurface eglSurface;
EGLContext eglContext;
GLuint program;

int windowWidth = 1920;
int windowHeight = 1080;

AppScreenState currentState = AppScreenState::Stopped;
//AppScreenState currentState = AppScreenState::Error;
//AppScreenState prevState = AppScreenState::Stopped;
AppScreenState lastRenderedState = AppScreenState::Stopped;
std::string renderText = "Screen Mirroring\nReady to connect";
std::string deviceName = "";
std::string errorCode = "";

GLuint textTexture = 0;
GLuint okButtonTexture = 0;
int texWidth = 0, texHeight = 0;
std::mutex renderMutex;

std::map<std::string, std::string> errorMap = {
    {"ENT-32101", "P2P Connect failure"},
    {"ENT-32207", "Internal Failure"}
};

QImage renderTextImage(const QString& text, int w, int h) {
    printf("%d:%s: img w=%d , h=%d\n", __LINE__, __func__, w, h);
    QImage img(w, h, QImage::Format_ARGB32);
    img.fill(Qt::white);
    
    QFontDatabase::addApplicationFont("/usr/share/fonts/Cinecav_Mono.ttf");
    QString family = QFontDatabase::applicationFontFamilies(0).at(0);
    QFont font(family, 28);

    QPainter p(&img);
    p.setPen(Qt::black);
    p.setFont(font);
    p.drawText(img.rect(), Qt::AlignCenter | Qt::TextWordWrap, text);
    p.end();
    return img;
}

GLuint createTextureFromQImage(const QImage &img, int &w, int &h) {
    w = img.width();
    h = img.height();
    QImage glImg = img.convertToFormat(QImage::Format_RGBA8888).mirrored();
    GLuint tex;
    glGenTextures(1, &tex);
    printf("%d:%s: tex=%d\n", __LINE__, __func__, tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, glImg.bits());
    return tex;
}

void drawTexturedQuad(GLuint texture, int texWidth, int texHeight, int centerX, int centerY) {
    glBindTexture(GL_TEXTURE_2D, texture);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int halfW = texWidth / 2;
    int halfH = texHeight / 2;

    GLfloat vertices[] = {
        (GLfloat)(centerX - halfW), (GLfloat)(centerY - halfH),
        (GLfloat)(centerX + halfW), (GLfloat)(centerY - halfH),
        (GLfloat)(centerX - halfW), (GLfloat)(centerY + halfH),
        (GLfloat)(centerX + halfW), (GLfloat)(centerY + halfH)
    };

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
}

void drawOkButton() {
    if (currentState != AppScreenState::Error) return;
    QImage okImg = renderTextImage("OK", 120, 60);
    if (okButtonTexture) glDeleteTextures(1, &okButtonTexture);
    int w, h;
    okButtonTexture = createTextureFromQImage(okImg, w, h);
    drawTexturedQuad(okButtonTexture, w, h, windowWidth - w - 60, windowHeight - h - 40);
}

void swapbuffer() {
    printf("%d:%s: windowWidth=%d,windowHeight=%d\n", __LINE__ , __func__, windowWidth, windowHeight);
    printf("%d:%s: texWidth=%d,texHeight=%d\n", __LINE__ , __func__, texWidth, texHeight);
    drawTexturedQuad(textTexture, texWidth, texHeight, windowWidth / 2, windowHeight / 2);
    drawOkButton();
    eglSwapBuffers(eglDisplay, eglSurface);
    printf("%d:%s end ...\n", __LINE__, __func__);
}

void updateTexture() {
    printf("%d:%s: renderText=%s\n", __LINE__, __func__, renderText.c_str());
    printf("%d:%s: textTexture=%d \n", __LINE__, __func__, textTexture);
    if (textTexture) glDeleteTextures(1, &textTexture);
    printf("%d:%s: deleted textTexture \n", __LINE__, __func__);
    QImage img = renderTextImage(QString::fromStdString(renderText), 800, 256);
    textTexture = createTextureFromQImage(img, texWidth, texHeight);
    printf("%d:%s: created textTexture =%d \n", __LINE__, __func__, textTexture);
    #if 0 //NOt working
    swapbuffer();
    #endif
}

void updateRenderText() {
    switch (currentState) {
        case AppScreenState::Stopped:
            renderText = "Screen Mirroring\nReady to connect";
            break;
        case AppScreenState::PreConnect:
            renderText = "Preparing connection...";
            break;
        case AppScreenState::Connecting:
            renderText = "Connecting to device: " + deviceName;
            break;
        case AppScreenState::Connected:
            renderText = "Connected to " + deviceName;
            break;
        case AppScreenState::Error:
            renderText = deviceName + " failed to connect\nCode: " + errorCode +
                         "\nReason: " + errorMap[errorCode];
            break;
    }
    //prevState = currentState;
    printf("[State] %d => %s\n", static_cast<int>(currentState), renderText.c_str());
    updateTexture();
}


void handleKeyPress(int keyCode) {
    printf("%d:%s: keyCode=%d\n", __LINE__, __func__, keyCode);
    if (currentState == AppScreenState::Error && keyCode == 134) {
        printf("[Key] OK button pressed\n");
        currentState = AppScreenState::Stopped;
        updateRenderText();
    }
}

void simulateEventThread() {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10s);
    currentState = AppScreenState::PreConnect;
    //updateRenderText();
    std::this_thread::sleep_for(10s);
    currentState = AppScreenState::Connecting;
    deviceName = "Pixel 7";
    //updateRenderText();
    std::this_thread::sleep_for(10s);
    currentState = AppScreenState::Connected;
    //updateRenderText();
    std::this_thread::sleep_for(10s);
    currentState = AppScreenState::Error;
    errorCode = "ENT-32101";
    //updateRenderText();
}

void keyPressed(void*, unsigned int keyCode) {
    handleKeyPress(keyCode);
    swapbuffer(); // force redraw on key
}

static void keyReleased(void*, unsigned int) {}

static EssKeyListener keyListener = {
    keyPressed,
    keyReleased
};

int renderLoop() {
    while (true) {
        if (lastRenderedState != currentState) {
            printf("[RenderLoop] State change detected. Updating texture.\n");
            updateRenderText();
            lastRenderedState = currentState;
            printf("%d:%s: state changed. start render the updated screen\n", __LINE__, __func__);
            #if 1
            glViewport(0, 0, windowWidth, windowHeight);
            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);
            swapbuffer();
            EssContextUpdateDisplay( essosCtx );
            EssContextRunEventLoopOnce( essosCtx );
            usleep(16666);
            #endif
            printf("%d:%s: End render the updated screen\n", __LINE__, __func__);
            return true;
        }
    }
}

int main(int argc, char** argv) {
    qputenv("QT_QPA_FONTDIR", "/usr/share/fonts");
    QGuiApplication app(argc, argv);

    essosCtx = EssContextCreate();
    EssContextInit(essosCtx);
    EssContextSetKeyListener(essosCtx, nullptr, &keyListener);
    EssContextStart(essosCtx);

    NativeDisplayType display;
    EssContextGetEGLDisplayType(essosCtx, &display);
    eglDisplay = eglGetDisplay(display);
    eglInitialize(eglDisplay, nullptr, nullptr);

    EGLConfig config;
    EGLint numConfigs;
    EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8, EGL_NONE
    };
    eglChooseConfig(eglDisplay, configAttribs, &config, 1, &numConfigs);

    EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    eglContext = eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT, ctxAttribs);

    EssContextGetDisplaySize(essosCtx, &windowWidth, &windowHeight);
    NativeWindowType nativeWindow;
    EssContextCreateNativeWindow(essosCtx, windowWidth, windowHeight, &nativeWindow);
    eglSurface = eglCreateWindowSurface(eglDisplay, config, nativeWindow, nullptr);
    eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);

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

    //using namespace std::chrono_literals;
    //std::this_thread::sleep_for(10s);
    // Welcome screen 
    updateRenderText();
    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    swapbuffer();
    EssContextUpdateDisplay( essosCtx );
    EssContextRunEventLoopOnce( essosCtx );
    usleep(16666);
    //swapbuffer();
    //usleep(16666);

    //std::thread eventThread(simulateEventThread);
    
    #if 0
    while ( simulateEventThread()) {
    //while (renderLoop()) {

    //}
    }
    #endif
    
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10s);
    currentState = AppScreenState::PreConnect;
    renderLoop();
    //updateRenderText();
    std::this_thread::sleep_for(10s);
    currentState = AppScreenState::Connecting;
    deviceName = "Pixel 7";
    renderLoop();
    //updateRenderText();
    std::this_thread::sleep_for(10s);
    currentState = AppScreenState::Connected;
    renderLoop();
    //updateRenderText();
    std::this_thread::sleep_for(10s);
    currentState = AppScreenState::Error;
    errorCode = "ENT-32101";
    renderLoop();
    std::this_thread::sleep_for(10s);
    while(1);

    //eventThread.join();
    return 0;
}
